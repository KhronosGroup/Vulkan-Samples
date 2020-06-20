/* Copyright (c) 2019-2020, Bradley Austin Davis
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "open_gl_interop.h"

#if defined(USE_GLFW)

#	include "common/vk_common.h"
#	include "gltf_loader.h"
#	include "gui.h"
#	include "platform/filesystem.h"
#	include "platform/platform.h"
#	include "rendering/subpasses/forward_subpass.h"

#	if defined(VK_USE_PLATFORM_ANDROID_KHR)
#		include "platform/android/android_platform.h"
#	endif

VKBP_DISABLE_WARNINGS()
#	include <glad/glad.h>
#	if defined(VK_USE_PLATFORM_ANDROID_KHR)
#		include <ctime>
#		include <EGL/egl.h>
#		include <EGL/eglext.h>
#	else
#		include <GLFW/glfw3.h>
#		include <GLFW/glfw3native.h>
#	endif
VKBP_ENABLE_WARNINGS()

constexpr uint32_t SHARED_TEXTURE_DIMENSION = 512;

#	if !defined(WIN32)
constexpr const char *HOST_MEMORY_EXTENSION_NAME    = VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME;
constexpr const char *HOST_SEMAPHORE_EXTENSION_NAME = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
#		define glImportSemaphore glImportSemaphoreFdEXT
#		define glImportMemory glImportMemoryFdEXT
#		define GL_HANDLE_TYPE GL_HANDLE_TYPE_OPAQUE_FD_EXT
#		define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
#		define VK_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
#	else
constexpr const char *HOST_MEMORY_EXTENSION_NAME    = VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME;
constexpr const char *HOST_SEMAPHORE_EXTENSION_NAME = VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME;
#		define glImportSemaphore glImportSemaphoreWin32HandleEXT
#		define glImportMemory glImportMemoryWin32HandleEXT
#		define GL_HANDLE_TYPE GL_HANDLE_TYPE_OPAQUE_WIN32_EXT
#		define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
#		define VK_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
#	endif

#	if defined(VK_USE_PLATFORM_ANDROID_KHR)
#		define OPENGL_SHADER_HEADER "#version 320 es"
#	else
#		define OPENGL_SHADER_HEADER "#version 450 core"
#	endif
constexpr const char *OPENG_VERTEX_SHADER =
    OPENGL_SHADER_HEADER
    R"SHADER(
const vec4 VERTICES[] = vec4[](
    vec4(-1.0, -1.0, 0.0, 1.0), 
    vec4( 1.0, -1.0, 0.0, 1.0),    
    vec4(-1.0,  1.0, 0.0, 1.0),
    vec4( 1.0,  1.0, 0.0, 1.0)
);   
void main() { gl_Position = VERTICES[gl_VertexID]; }
)SHADER";

// Derived from Shadertoy Vornoi noise shader by Inigo Quilez
// https://www.shadertoy.com/view/Xd23Dh
constexpr const char *OPENGL_FRAGMENT_SHADER =
    OPENGL_SHADER_HEADER
    R"SHADER(
const vec4 iMouse = vec4(0.0); 
layout(location = 0) out vec4 outColor;
layout(location = 0) uniform vec3 iResolution;
layout(location = 1) uniform float iTime;
vec3 hash3( vec2 p )
{
    vec3 q = vec3( dot(p,vec2(127.1,311.7)), 
                   dot(p,vec2(269.5,183.3)), 
                   dot(p,vec2(419.2,371.9)) );
    return fract(sin(q)*43758.5453);
}
float iqnoise( in vec2 x, float u, float v )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
        
    float k = 1.0+63.0*pow(1.0-v,4.0);
    
    float va = 0.0;
    float wt = 0.0;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        vec2 g = vec2( float(i),float(j) );
        vec3 o = hash3( p + g )*vec3(u,u,1.0);
        vec2 r = g - f + o.xy;
        float d = dot(r,r);
        float ww = pow( 1.0-smoothstep(0.0,1.414,sqrt(d)), k );
        va += o.z*ww;
        wt += ww;
    }
    
    return va/wt;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xx;
    vec2 p = 0.5 - 0.5*sin( iTime*vec2(1.01,1.71) );
    
    if( iMouse.w>0.001 ) p = vec2(0.0,1.0) + vec2(1.0,-1.0)*iMouse.xy/iResolution.xy;
    
    p = p*p*(3.0-2.0*p);
    p = p*p*(3.0-2.0*p);
    p = p*p*(3.0-2.0*p);
    
    float f = iqnoise( 24.0*uv, p.x, p.y );
    
    fragColor = vec4( f, f, f, 1.0 );
}
void main() { mainImage(outColor, gl_FragCoord.xy); }
)SHADER";

class OpenGLWindow
{
	static void debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
	{
		switch (severity)
		{
			case GL_DEBUG_SEVERITY_HIGH:
				LOGE("OpenGL: {}", message);
				break;
			case GL_DEBUG_SEVERITY_MEDIUM:
				LOGW("OpenGL: {}", message);
				break;
			case GL_DEBUG_SEVERITY_LOW:
				LOGI("OpenGL: {}", message);
				break;
			default:
			case GL_DEBUG_SEVERITY_NOTIFICATION:
				LOGD("OpenGL: {}", message);
		}
	}

	static GLuint loadShader(const char *shaderSource, GLenum shaderType)
	{
		GLuint shader = glCreateShader(shaderType);
		int    sizes  = static_cast<int>(strlen(shaderSource));
		glShaderSource(shader, 1, &shaderSource, &sizes);
		glCompileShader(shader);

		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> errorLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
			std::string strError;
			strError.insert(strError.end(), errorLog.begin(), errorLog.end());

			// Provide the infolog in whatever manor you deem best.
			// Exit with failure.
			glDeleteShader(shader);        // Don't leak the shader.
			LOGE("OpenGL: Shader compilation failed", strError.c_str());
		}
		return shader;
	}

	static GLuint buildProgram(const char *vertexShaderSource, const char *fragmentShaderSource)
	{
		GLuint program = glCreateProgram();
		GLuint vs      = loadShader(vertexShaderSource, GL_VERTEX_SHADER);
		GLuint fs      = loadShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		glDeleteShader(vs);
		glDeleteShader(fs);
		return program;
	}

  public:
	void create(const OpenGLInterop::ShareHandles &handles, uint64_t memorySize)
	{
#	if defined(VK_USE_PLATFORM_ANDROID_KHR)
		EGLint eglMajVers{0}, eglMinVers{0};
		eglDisp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		eglInitialize(eglDisp, &eglMajVers, &eglMinVers);
		LOGD("EGL init with version %d.%d", eglMajVers, eglMinVers);

		constexpr EGLint confAttr[] = {
		    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		    EGL_RED_SIZE, 8,
		    EGL_GREEN_SIZE, 8,
		    EGL_BLUE_SIZE, 8,
		    EGL_ALPHA_SIZE, 8,
		    EGL_NONE};

		EGLint numConfigs;
		eglChooseConfig(eglDisp, confAttr, &eglConf, 1, &numConfigs);

		// Create a EGL context
		constexpr EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

		eglCtx = eglCreateContext(eglDisp, eglConf, EGL_NO_CONTEXT, ctxAttr);

		// Create an offscreen pbuffer surface, and then make it current
		constexpr EGLint surfaceAttr[] = {EGL_WIDTH, 10, EGL_HEIGHT, 10, EGL_NONE};
		eglSurface                     = eglCreatePbufferSurface(eglDisp, eglConf, surfaceAttr);
		eglMakeCurrent(eglDisp, eglSurface, eglSurface, eglCtx);
		gladLoadGLES2Loader((GLADloadproc) &eglGetProcAddress);
#	else
		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		window = glfwCreateWindow(SHARED_TEXTURE_DIMENSION, SHARED_TEXTURE_DIMENSION, "OpenGL Window", nullptr, nullptr);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwMakeContextCurrent(window);
		gladLoadGL();
#	endif

		glDebugMessageCallback(debugMessageCallback, NULL);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		program = buildProgram(OPENG_VERTEX_SHADER, OPENGL_FRAGMENT_SHADER);
		timer.start();

		glDisable(GL_DEPTH_TEST);

		// Create the texture for the FBO color attachment.
		// This only reserves the ID, it doesn't allocate memory
		glGenTextures(1, &color);
		glBindTexture(GL_TEXTURE_2D, color);

		// Create the GL identifiers

		// semaphores
		glGenSemaphoresEXT(1, &glReady);
		glGenSemaphoresEXT(1, &glComplete);
		// memory
		glCreateMemoryObjectsEXT(1, &mem);

		// Platform specific import.
		glImportSemaphore(glReady, GL_HANDLE_TYPE, handles.glReady);
		glImportSemaphore(glComplete, GL_HANDLE_TYPE, handles.glComplete);
		glImportMemory(mem, memorySize, GL_HANDLE_TYPE, handles.memory);

		// Use the imported memory as backing for the OpenGL texture.  The internalFormat, dimensions
		// and mip count should match the ones used by Vulkan to create the image and determine it's memory
		// allocation.
		glTextureStorageMem2DEXT(color, 1, GL_RGBA8, SHARED_TEXTURE_DIMENSION, SHARED_TEXTURE_DIMENSION, mem, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// The remaining initialization code is all standard OpenGL
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color, 0);

		glUseProgram(program);
		glProgramUniform3f(program, 0, (float) SHARED_TEXTURE_DIMENSION, (float) SHARED_TEXTURE_DIMENSION, 0.0f);

		glViewport(0, 0, SHARED_TEXTURE_DIMENSION, SHARED_TEXTURE_DIMENSION);
	}

	void render()
	{
		float time = (float) timer.elapsed();
		// The GL shader animates the image, so provide the time as input
		glProgramUniform1f(program, 1, time);

		// Wait (on the GPU side) for the Vulkan semaphore to be signaled
		GLenum srcLayout = GL_LAYOUT_COLOR_ATTACHMENT_EXT;
		glWaitSemaphoreEXT(glReady, 0, nullptr, 1, &color, &srcLayout);

		// Draw to the framebuffer
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Once drawing is complete, signal the Vulkan semaphore indicating
		// it can continue with it's render
		GLenum dstLayout = GL_LAYOUT_SHADER_READ_ONLY_EXT;
		glSignalSemaphoreEXT(glComplete, 0, nullptr, 1, &color, &dstLayout);

		// When using synchronization across multiple GL context, or in this case
		// across OpenGL and another API, it's critical that an operation on a
		// synchronization object that will be waited on in another context or API
		// is flushed to the GL server.
		//
		// Failure to flush the operation can cause the GL driver to sit and wait for
		// sufficient additional commands in the buffer before it flushes automatically
		// but depending on how the waits and signals are structured, this may never
		// occur.
		glFlush();
	}

	void destroy()
	{
		glFinish();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);
		glDeleteFramebuffers(1, &fbo);
		glDeleteTextures(1, &color);
		glDeleteSemaphoresEXT(1, &glReady);
		glDeleteSemaphoresEXT(1, &glComplete);
		glDeleteVertexArrays(1, &vao);
		glDeleteProgram(program);
		glFlush();
		glFinish();
#	if defined(VK_USE_PLATFORM_ANDROID_KHR)
		eglDestroySurface(eglDisp, eglSurface);
		eglSurface = nullptr;
		eglDestroyContext(eglDisp, eglCtx);
		eglCtx = nullptr;
#	else
		glfwDestroyWindow(window);
		window = nullptr;
#	endif
	}

  private:
#	if defined(VK_USE_PLATFORM_ANDROID_KHR)
	EGLConfig  eglConf{nullptr};
	EGLSurface eglSurface{EGL_NO_SURFACE};
	EGLContext eglCtx{EGL_NO_CONTEXT};
	EGLDisplay eglDisp{EGL_NO_DISPLAY};
#	else
	GLFWwindow *window{nullptr};
#	endif
	GLuint     glReady{0}, glComplete{0};
	GLuint     color{0};
	GLuint     fbo{0};
	GLuint     vao{0};
	GLuint     program{0};
	GLuint     mem{0};
	vkb::Timer timer;
};

OpenGLInterop::OpenGLInterop()
{
	zoom  = -2.5f;
	title = "Interoperability with OpenGL";
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_instance_extension(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
	add_instance_extension(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);

	add_device_extension(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
	add_device_extension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);

	add_device_extension(HOST_SEMAPHORE_EXTENSION_NAME);
	add_device_extension(HOST_MEMORY_EXTENSION_NAME);
}

OpenGLInterop::~OpenGLInterop()
{
	if (glWindow)
	{
		glWindow->destroy();
		glWindow.reset();
	}

	vertex_buffer.reset();
	index_buffer.reset();
	uniform_buffer_vs.reset();

	if (device)
	{
		device->wait_idle();
		auto deviceHandle = device->get_handle();
		vkDestroySemaphore(deviceHandle, sharedSemaphores.glReady, nullptr);
		vkDestroySemaphore(deviceHandle, sharedSemaphores.glComplete, nullptr);
		vkDestroyImage(deviceHandle, sharedTexture.image, nullptr);
		vkDestroySampler(deviceHandle, sharedTexture.sampler, nullptr);
		vkDestroyImageView(deviceHandle, sharedTexture.view, nullptr);
		vkFreeMemory(deviceHandle, sharedTexture.memory, nullptr);
		vkDestroyPipeline(deviceHandle, pipeline, nullptr);
		vkDestroyPipelineLayout(deviceHandle, pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(deviceHandle, descriptor_set_layout, nullptr);
	}
}

void OpenGLInterop::prepare_shared_resources()
{
	auto deviceHandle = device->get_handle();

	{
		VkExportSemaphoreCreateInfo exportSemaphoreCreateInfo{VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO, nullptr, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE};
		VkSemaphoreCreateInfo       semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &exportSemaphoreCreateInfo};
		VK_CHECK(vkCreateSemaphore(deviceHandle, &semaphoreCreateInfo, nullptr, &sharedSemaphores.glComplete));
		VK_CHECK(vkCreateSemaphore(deviceHandle, &semaphoreCreateInfo, nullptr, &sharedSemaphores.glReady));

#	if WIN32
		VkSemaphoreGetWin32HandleInfoKHR semaphoreGetHandleInfo{
		    VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR, nullptr,
		    nullptr, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT};
		semaphoreGetHandleInfo.semaphore = sharedSemaphores.glReady;
		VK_CHECK(vkGetSemaphoreWin32HandleKHR(deviceHandle, &semaphoreGetHandleInfo, &shareHandles.glReady));
		semaphoreGetHandleInfo.semaphore = sharedSemaphores.glComplete;
		VK_CHECK(vkGetSemaphoreWin32HandleKHR(deviceHandle, &semaphoreGetHandleInfo, &shareHandles.glComplete));
#	else
		VkSemaphoreGetFdInfoKHR semaphoreGetFdInfo{
		    VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR, nullptr,
		    nullptr, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT};
		semaphoreGetFdInfo.semaphore = sharedSemaphores.glReady;
		VK_CHECK(vkGetSemaphoreFdKHR(deviceHandle, &semaphoreGetFdInfo, &shareHandles.glReady));
		semaphoreGetFdInfo.semaphore = sharedSemaphores.glComplete;
		VK_CHECK(vkGetSemaphoreFdKHR(deviceHandle, &semaphoreGetFdInfo, &shareHandles.glComplete));
#	endif
	}

	{
		VkImageCreateInfo imageCreateInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
		imageCreateInfo.mipLevels     = 1;
		imageCreateInfo.arrayLayers   = 1;
		imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.extent.depth  = 1;
		imageCreateInfo.extent.width  = SHARED_TEXTURE_DIMENSION;
		imageCreateInfo.extent.height = SHARED_TEXTURE_DIMENSION;
		imageCreateInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VK_CHECK(vkCreateImage(deviceHandle, &imageCreateInfo, nullptr, &sharedTexture.image));

		VkMemoryRequirements memReqs{};
		vkGetImageMemoryRequirements(device->get_handle(), sharedTexture.image, &memReqs);

		VkExportMemoryAllocateInfo exportAllocInfo{
		    VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO, nullptr,
		    VK_EXTERNAL_MEMORY_HANDLE_TYPE};
		VkMemoryAllocateInfo memAllocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &exportAllocInfo};

		memAllocInfo.allocationSize = sharedTexture.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex                               = device->get_memory_type(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK(vkAllocateMemory(deviceHandle, &memAllocInfo, nullptr, &sharedTexture.memory));
		VK_CHECK(vkBindImageMemory(deviceHandle, sharedTexture.image, sharedTexture.memory, 0));

#	if WIN32
		VkMemoryGetWin32HandleInfoKHR memoryFdInfo{VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR, nullptr,
		                                           sharedTexture.memory,
		                                           VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT};
		VK_CHECK(vkGetMemoryWin32HandleKHR(deviceHandle, &memoryFdInfo, &shareHandles.memory));
#	else
		VkMemoryGetFdInfoKHR memoryFdInfo{VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR, nullptr,
		                                  sharedTexture.memory,
		                                  VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT};
		VK_CHECK(vkGetMemoryFdKHR(deviceHandle, &memoryFdInfo, &shareHandles.memory));
#	endif

		// Create sampler
		VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		samplerCreateInfo.magFilter  = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter  = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.maxLod     = (float) 1;
		//samplerCreateInfo.maxAnisotropy = context.deviceFeatures.samplerAnisotropy ? context.deviceProperties.limits.maxSamplerAnisotropy : 1.0f;
		//samplerCreateInfo.anisotropyEnable = context.deviceFeatures.samplerAnisotropy;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		vkCreateSampler(deviceHandle, &samplerCreateInfo, nullptr, &sharedTexture.sampler);

		// Create image view
		VkImageViewCreateInfo viewCreateInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		viewCreateInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.image            = sharedTexture.image;
		viewCreateInfo.format           = VK_FORMAT_R8G8B8A8_UNORM;
		viewCreateInfo.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		vkCreateImageView(deviceHandle, &viewCreateInfo, nullptr, &sharedTexture.view);

		with_command_buffer([&](VkCommandBuffer image_command_buffer) {
			VkImageMemoryBarrier image_memory_barrier  = vkb::initializers::image_memory_barrier();
			image_memory_barrier.image                 = sharedTexture.image;
			image_memory_barrier.srcAccessMask         = 0;
			image_memory_barrier.dstAccessMask         = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			image_memory_barrier.oldLayout             = VK_IMAGE_LAYOUT_UNDEFINED;
			image_memory_barrier.newLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkImageSubresourceRange &subresource_range = image_memory_barrier.subresourceRange;
			subresource_range.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
			subresource_range.levelCount               = 1;
			subresource_range.layerCount               = 1;

			vkCmdPipelineBarrier(
			    image_command_buffer,
			    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			    0,
			    0, nullptr,
			    0, nullptr,
			    1, &image_memory_barrier);
		},
		                    sharedSemaphores.glReady);
	}
}

void OpenGLInterop::prepare_opengl_context()
{
	glWindow = std::make_unique<OpenGLWindow>();
	glWindow->create(shareHandles, sharedTexture.allocationSize);
}

void OpenGLInterop::draw()
{
	ApiVulkanSample::prepare_frame();

	glWindow->render();

	std::array<VkPipelineStageFlags, 2> waitStages{{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT}};
	std::array<VkSemaphore, 2>          waitSemaphores{{semaphores.acquired_image_ready, sharedSemaphores.glComplete}};

	std::array<VkSemaphore, 2> signalSemaphores{{semaphores.render_complete, sharedSemaphores.glReady}};
	// Command buffer to be sumitted to the queue
	submit_info.waitSemaphoreCount   = vkb::to_u32(waitSemaphores.size());
	submit_info.pWaitSemaphores      = waitSemaphores.data();
	submit_info.pWaitDstStageMask    = waitStages.data();
	submit_info.signalSemaphoreCount = vkb::to_u32(signalSemaphores.size());
	submit_info.pSignalSemaphores    = signalSemaphores.data();
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void OpenGLInterop::generate_quad()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	std::vector<VertexStructure> vertices =
	    {
	        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	        {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	    };

	// Setup indices
	std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
	index_count                   = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vkb::to_u32(vertices.size() * sizeof(VertexStructure));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                    vertex_buffer_size,
	                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                   index_buffer_size,
	                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                   VMA_MEMORY_USAGE_CPU_TO_GPU);

	index_buffer->update(indices.data(), index_buffer_size);
}

void OpenGLInterop::setup_descriptor_pool()
{
	// Example uses one ubo and one image sampler
	std::vector<VkDescriptorPoolSize> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        2);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void OpenGLInterop::setup_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings{
	    // Binding 0 : Vertex shader uniform buffer
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    // Binding 1 : Fragment shader image sampler
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        VK_SHADER_STAGE_FRAGMENT_BIT,
	        1),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        vkb::to_u32(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void OpenGLInterop::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer_vs);

	// Setup a descriptor image info for the current texture to be used as a combined image sampler
	VkDescriptorImageInfo image_descriptor;
	image_descriptor.imageView   = sharedTexture.view;                              // The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
	image_descriptor.sampler     = sharedTexture.sampler;                           // The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;        // The current layout of the image (Note: Should always fit the actual use, e.g. shader read)

	std::vector<VkWriteDescriptorSet> write_descriptor_sets =
	    {
	        // Binding 0 : Vertex shader uniform buffer
	        vkb::initializers::write_descriptor_set(
	            descriptor_set,
	            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	            0,
	            &buffer_descriptor),
	        // Binding 1 : Fragment shader texture sampler
	        //	Fragment shader: layout (binding = 1) uniform sampler2D samplerColor;
	        vkb::initializers::write_descriptor_set(
	            descriptor_set,
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,        // The descriptor set will use a combined image sampler (sampler and image could be split)
	            1,                                                // Shader binding point 1
	            &image_descriptor)                                // Pointer to the descriptor image for our texture
	    };

	vkUpdateDescriptorSets(get_device().get_handle(), vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

void OpenGLInterop::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_NONE,
	        VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        vkb::to_u32(dynamic_state_enables.size()),
	        0);

	// Load shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	shader_stages[0] = load_shader("texture_loading/texture.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("texture_loading/texture.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(VertexStructure), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexStructure, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexStructure, uv)),
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexStructure, normal)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = vkb::to_u32(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = vkb::to_u32(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layout,
	        render_pass,
	        0);

	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = vkb::to_u32(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms
void OpenGLInterop::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        sizeof(ubo_vs),
	                                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void OpenGLInterop::update_uniform_buffers()
{
	// Vertex shader
	ubo_vs.projection     = glm::perspective(glm::radians(60.0f), (float) width / (float) height, 0.001f, 256.0f);
	glm::mat4 view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

	ubo_vs.model = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo_vs.view_pos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

	uniform_buffer_vs->convert_and_update(ubo_vs);
}

bool OpenGLInterop::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	prepare_shared_resources();
	prepare_opengl_context();

	generate_quad();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

void OpenGLInterop::render(float)
{
	if (!prepared)
		return;
	draw();
}

void OpenGLInterop::view_changed()
{
	update_uniform_buffers();
}

void OpenGLInterop::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
	}
}

void OpenGLInterop::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		{
			VkImageMemoryBarrier image_memory_barrier = vkb::initializers::image_memory_barrier();
			image_memory_barrier.image                = sharedTexture.image;
			image_memory_barrier.srcAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			image_memory_barrier.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;
			image_memory_barrier.oldLayout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			image_memory_barrier.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkImageSubresourceRange &subresource_range = image_memory_barrier.subresourceRange;
			subresource_range.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
			subresource_range.levelCount               = 1;
			subresource_range.layerCount               = 1;
			vkCmdPipelineBarrier(
			    draw_cmd_buffers[i],
			    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			    0, nullptr, 0, nullptr, 1, &image_memory_barrier);
		}

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		{
			VkImageMemoryBarrier image_memory_barrier = vkb::initializers::image_memory_barrier();
			image_memory_barrier.image                = sharedTexture.image;
			image_memory_barrier.srcAccessMask        = VK_ACCESS_SHADER_READ_BIT;
			image_memory_barrier.dstAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			image_memory_barrier.oldLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_memory_barrier.newLayout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkImageSubresourceRange &subresource_range = image_memory_barrier.subresourceRange;
			subresource_range.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
			subresource_range.levelCount               = 1;
			subresource_range.layerCount               = 1;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			// Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
			// Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
			vkCmdPipelineBarrier(
			    draw_cmd_buffers[i],
			    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			    0,
			    0, nullptr,
			    0, nullptr,
			    1, &image_memory_barrier);
		}
		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

#else
#endif

std::unique_ptr<vkb::VulkanSample> create_open_gl_interop()
{
	return std::make_unique<OpenGLInterop>();
}
