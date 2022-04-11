#include "components/vfs/android.hpp"

#include <android/asset_manager.h>

namespace vfs
{
RootFileSystem &instance(void *context)
{
	static vfs::RootFileSystem fs;

	static bool first_time = true;
	if (first_time)
	{
		auto *app = reinterpret_cast<android_app *>(context);

		fs.mount("/temp", std::make_shared<vfs::AndroidTempFileSystem>(app));

#ifdef VKB_BUNDLE_ASSETS
		fs.mount("/", std::make_shared<vfs::AndroidAAssetManager>(app));
		fs.mount("/scenes", std::make_shared<vfs::AndroidAAssetManager>(app, "scenes"));
		fs.mount("/textures", std::make_shared<vfs::AndroidAAssetManager>(app, "textures"));
		fs.mount("/fonts", std::make_shared<vfs::AndroidAAssetManager>(app, "fonts"));

#else
		fs.mount("/", std::make_shared<vfs::AndroidFileSystem>(app));
		fs.mount("/scenes", std::make_shared<vfs::AndroidFileSystem>(app, "/assets/scenes"));
		fs.mount("/textures", std::make_shared<vfs::AndroidFileSystem>(app, "/assets/textures"));
		fs.mount("/fonts", std::make_shared<vfs::AndroidFileSystem>(app, "/assets/fonts"));
#endif

		first_time = false;
	}

	return fs;
}

std::string get_external_file_dir(android_app *app)
{
	std::string path = "/";

	if (!app)
	{
		return path;
	}

	JNIEnv *env;
	app->activity->vm->AttachCurrentThread(&env, NULL);

	// Create reference frame
	if (env->PushLocalFrame(16) >= 0)
	{
		jclass     activity_class         = env->GetObjectClass(app->activity->clazz);
		jmethodID  get_external_files_dir = env->GetMethodID(activity_class, "getExternalFilesDir", "()Ljava/io/File;");
		jobject    file                   = env->CallObjectMethod(app->activity->clazz, get_external_files_dir);
		jthrowable error                  = env->ExceptionOccurred();
		if (error != nullptr || file == nullptr)
		{
			// ! exception handling?

			return "/";
		}

		if (!file)
		{
			// ! exception handling?

			return "/";
		}

		jmethodID get_canonical_path = env->GetMethodID(env->GetObjectClass(file), "getCanonicalPath", "()Ljava/lang/String;");
		jstring   jstr               = (jstring) env->CallObjectMethod(file, get_canonical_path);
		error                        = env->ExceptionOccurred();
		if (error != nullptr || file == nullptr)
		{
			// ! exception handling?

			return "/";
		}

		const char *c_path = env->GetStringUTFChars(jstr, NULL);
		path               = std::string{c_path};
		env->ReleaseStringUTFChars(jstr, c_path);
	}

	app->activity->vm->DetachCurrentThread();

	return path;
}

std::string get_external_cache_dir(android_app *app)
{
	std::string path = "/";

	if (!app)
	{
		return path;
	}

	JNIEnv *env;
	app->activity->vm->AttachCurrentThread(&env, NULL);

	// Create reference frame
	if (env->PushLocalFrame(16) >= 0)
	{
		jclass     activity_class         = env->GetObjectClass(app->activity->clazz);
		jmethodID  get_external_files_dir = env->GetMethodID(activity_class, "getExternalCacheDir", "()Ljava/io/File;");
		jobject    file                   = env->CallObjectMethod(app->activity->clazz, get_external_files_dir);
		jthrowable error                  = env->ExceptionOccurred();
		if (error != nullptr || file == nullptr)
		{
			// ! exception handling?

			return "/";
		}

		if (!file)
		{
			// ! exception handling?

			return "/";
		}

		jmethodID get_canonical_path = env->GetMethodID(env->GetObjectClass(file), "getCanonicalPath", "()Ljava/lang/String;");
		jstring   jstr               = (jstring) env->CallObjectMethod(file, get_canonical_path);
		error                        = env->ExceptionOccurred();
		if (error != nullptr || file == nullptr)
		{
			// ! exception handling?

			return "/";
		}

		const char *c_path = env->GetStringUTFChars(jstr, NULL);
		path               = std::string{c_path};
		env->ReleaseStringUTFChars(jstr, c_path);
	}

	app->activity->vm->DetachCurrentThread();

	return path;
}

AndroidTempFileSystem::AndroidTempFileSystem(android_app *app, const std::string &sub_path) :
    UnixFileSystem{get_external_cache_dir(app)}
{
}

AndroidExternalFileSystem::AndroidExternalFileSystem(android_app *app, const std::string &sub_path) :
    UnixFileSystem{get_external_file_dir(app)}
{
}
}        // namespace vfs