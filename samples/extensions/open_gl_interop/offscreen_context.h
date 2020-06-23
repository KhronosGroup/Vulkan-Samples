#pragma once

#include <memory>

#include <glad/glad.h>

#include "common/vk_common.h"

constexpr uint32_t SHARED_TEXTURE_DIMENSION = 512;

#ifdef WIN32
constexpr const char *HOST_MEMORY_EXTENSION_NAME    = VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME;
constexpr const char *HOST_SEMAPHORE_EXTENSION_NAME = VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME;
#	define glImportSemaphore glImportSemaphoreWin32HandleEXT
#	define glImportMemory glImportMemoryWin32HandleEXT
#	define GL_HANDLE_TYPE GL_HANDLE_TYPE_OPAQUE_WIN32_EXT
#	define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
#	define VK_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
#else
constexpr const char *HOST_MEMORY_EXTENSION_NAME    = VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME;
constexpr const char *HOST_SEMAPHORE_EXTENSION_NAME = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
#	define glImportSemaphore glImportSemaphoreFdEXT
#	define glImportMemory glImportMemoryFdEXT
#	define GL_HANDLE_TYPE GL_HANDLE_TYPE_OPAQUE_FD_EXT
#	define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
#	define VK_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
// Android
#	include <EGL/egl.h>
#	include <EGL/eglext.h>

struct ContextData
{
	EGLConfig  config{nullptr};
	EGLSurface surface{EGL_NO_SURFACE};
	EGLContext context{EGL_NO_CONTEXT};
	EGLDisplay display{EGL_NO_DISPLAY};
};
#else
// Desktop
#	include <GLFW/glfw3.h>
#	include <GLFW/glfw3native.h>

struct ContextData
{
	GLFWwindow *window;
};
#endif

class OffscreenContext
{
  public:
	OffscreenContext();

	~OffscreenContext();

	// Shared
	GLuint build_program(const char *vertex_shader_source, const char *fragment_shader_source);

  private:
	// Platform specific
	void init_context();

	void destroy_context();

	std::string get_shader_header();

	ContextData data;

	GLuint load_shader(const char *shader_source, GLenum shader_type);
};