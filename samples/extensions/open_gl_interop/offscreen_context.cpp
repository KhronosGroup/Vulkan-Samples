
#include "offscreen_context.h"

#include <string>
#include <vector>

#include "common/logging.h"

void APIENTRY debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user_param)
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

OffscreenContext::OffscreenContext()
{
	init_context();

	glDebugMessageCallback(debug_message_callback, NULL);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

OffscreenContext::~OffscreenContext()
{
	destroy_context();
}

GLuint OffscreenContext::load_shader(const char *shader_source, GLenum shader_type)
{
	std::string source     = get_shader_header() + "\n" + std::string(shader_source);
	const char *source_ptr = source.c_str();
	const GLint size       = static_cast<GLint>(source.size());

	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, &source_ptr, &size);
	glCompileShader(shader);

	GLint is_compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

		// The maxLength includes the NULL character
		std::vector<GLchar> error_log(max_length);
		glGetShaderInfoLog(shader, max_length, &max_length, &error_log[0]);
		std::string str_error;
		str_error.insert(str_error.end(), error_log.begin(), error_log.end());

		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(shader);        // Don't leak the shader.
		LOGE("OpenGL: Shader compilation failed", str_error.c_str());
	}
	return shader;
}

GLuint OffscreenContext::build_program(const char *vertex_shader_source, const char *fragment_shader_source)
{
	GLuint program = glCreateProgram();
	GLuint vs      = load_shader(vertex_shader_source, GL_VERTEX_SHADER);
	GLuint fs      = load_shader(fragment_shader_source, GL_FRAGMENT_SHADER);
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glDeleteShader(vs);
	glDeleteShader(fs);
	return program;
}

#ifdef VK_USE_PLATFORM_ANDROID_KHR
void OffscreenContext::init_context()
{
	EGLint egl_maj_vers{0},
	    egl_min_vers{0};
	data.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(data.display, &egl_maj_vers, &egl_min_vers);

	constexpr EGLint conf_attr[] = {
	    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
	    EGL_NONE};

	EGLint num_configs;
	eglChooseConfig(data.display, conf_attr, &data.config, 1, &num_configs);

	// Create a EGL context
	constexpr EGLint ctx_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

	data.context = eglCreateContext(data.display, data.config, EGL_NO_CONTEXT, ctx_attr);
	if (data.context == EGL_NO_CONTEXT)
	{
		throw std::runtime_error{"Failed to create EGL context"};
	}

	// Create an offscreen pbuffer surface, and then make it current
	constexpr EGLint surface_attr[] = {EGL_WIDTH, 10, EGL_HEIGHT, 10, EGL_NONE};
	data.surface                    = eglCreatePbufferSurface(data.display, data.config, surface_attr);
	eglMakeCurrent(data.display, data.surface, data.surface, data.context);
	gladLoadGLES2Loader((GLADloadproc) &eglGetProcAddress);

	LOGD("EGL init with version {}.{}", egl_maj_vers, egl_min_vers);
}

void OffscreenContext::destroy_context()
{
	eglDestroySurface(data.display, data.surface);
	eglDestroyContext(data.display, data.context);
}

std::string OffscreenContext::get_shader_header()
{
	return "#version 320 es";
}
#else
void OffscreenContext::init_context()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	data.window = glfwCreateWindow(SHARED_TEXTURE_DIMENSION, SHARED_TEXTURE_DIMENSION, "OpenGL Window", nullptr, nullptr);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwMakeContextCurrent(data.window);
	gladLoadGL();
}

void OffscreenContext::destroy_context()
{
	glfwDestroyWindow(data.window);
}

std::string OffscreenContext::get_shader_header()
{
	return "#version 450 core";
}
#endif