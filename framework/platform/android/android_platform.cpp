/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "android_platform.h"

#include <chrono>
#include <unistd.h>
#include <unordered_map>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <imgui.h>
#include <jni.h>
#include <spdlog/sinks/android_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

#include "apps.h"
#include "common/logging.h"
#include "common/strings.h"
#include "platform/android/android_window.h"
#include "platform/input_events.h"

extern "C"
{
	JNIEXPORT void JNICALL
	    Java_com_khronos_vulkan_1samples_SampleLauncherActivity_initFilePath(JNIEnv *env, jobject thiz, jstring external_dir, jstring temp_dir)
	{
		const char *external_dir_cstr = env->GetStringUTFChars(external_dir, 0);
		vkb::Platform::set_external_storage_directory(std::string(external_dir_cstr) + "/");
		env->ReleaseStringUTFChars(external_dir, external_dir_cstr);

		const char *temp_dir_cstr = env->GetStringUTFChars(temp_dir, 0);
		vkb::Platform::set_temp_directory(std::string(temp_dir_cstr) + "/");
		env->ReleaseStringUTFChars(temp_dir, temp_dir_cstr);
	}

	JNIEXPORT jobjectArray JNICALL
	    Java_com_khronos_vulkan_1samples_SampleLauncherActivity_getSamples(JNIEnv *env, jobject thiz)
	{
		auto sample_list = apps::get_samples();

		jclass       c             = env->FindClass("com/khronos/vulkan_samples/model/Sample");
		jmethodID    constructor   = env->GetMethodID(c, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");
		jobjectArray j_sample_list = env->NewObjectArray(sample_list.size(), c, 0);

		for (int sample_index = 0; sample_index < sample_list.size(); sample_index++)
		{
			const apps::SampleInfo *sample_info = reinterpret_cast<apps::SampleInfo *>(sample_list[sample_index]);

			jstring id       = env->NewStringUTF(sample_info->id.c_str());
			jstring category = env->NewStringUTF(sample_info->category.c_str());
			jstring author   = env->NewStringUTF(sample_info->author.c_str());
			jstring name     = env->NewStringUTF(sample_info->name.c_str());
			jstring desc     = env->NewStringUTF(sample_info->description.c_str());

			jobjectArray j_tag_list = env->NewObjectArray(sample_info->tags.size(), env->FindClass("java/lang/String"), env->NewStringUTF(""));
			for (int tag_index = 0; tag_index < sample_info->tags.size(); ++tag_index)
			{
				env->SetObjectArrayElement(j_tag_list, tag_index, env->NewStringUTF(sample_info->tags[tag_index].c_str()));
			}

			env->SetObjectArrayElement(j_sample_list, sample_index, env->NewObject(c, constructor, id, category, author, name, desc, j_tag_list));
		}

		return j_sample_list;
	}

	JNIEXPORT void JNICALL
	    Java_com_khronos_vulkan_1samples_SampleLauncherActivity_sendArgumentsToPlatform(JNIEnv *env, jobject thiz, jobjectArray arg_strings)
	{
		std::vector<std::string> args;

		for (int i = 0; i < env->GetArrayLength(arg_strings); i++)
		{
			jstring arg_string = (jstring) (env->GetObjectArrayElement(arg_strings, i));

			const char *arg = env->GetStringUTFChars(arg_string, 0);

			args.push_back(std::string(arg));

			env->ReleaseStringUTFChars(arg_string, arg);
		}

		vkb::Platform::set_arguments(args);
	}
}

namespace vkb
{
namespace
{
inline std::tm thread_safe_time(const std::time_t time)
{
	std::tm                     result;
	std::mutex                  mtx;
	std::lock_guard<std::mutex> lock(mtx);
	result = *std::localtime(&time);
	return result;
}

inline KeyCode translate_key_code(int key)
{
	static const std::unordered_map<int, KeyCode> key_lookup =
	    {
	        {AKEYCODE_SPACE, KeyCode::Space},
	        {AKEYCODE_APOSTROPHE, KeyCode::Apostrophe},
	        {AKEYCODE_COMMA, KeyCode::Comma},
	        {AKEYCODE_MINUS, KeyCode::Minus},
	        {AKEYCODE_PERIOD, KeyCode::Period},
	        {AKEYCODE_SLASH, KeyCode::Slash},
	        {AKEYCODE_0, KeyCode::_0},
	        {AKEYCODE_1, KeyCode::_1},
	        {AKEYCODE_2, KeyCode::_2},
	        {AKEYCODE_3, KeyCode::_3},
	        {AKEYCODE_4, KeyCode::_4},
	        {AKEYCODE_5, KeyCode::_5},
	        {AKEYCODE_6, KeyCode::_6},
	        {AKEYCODE_7, KeyCode::_7},
	        {AKEYCODE_8, KeyCode::_8},
	        {AKEYCODE_9, KeyCode::_9},
	        {AKEYCODE_SEMICOLON, KeyCode::Semicolon},
	        {AKEYCODE_EQUALS, KeyCode::Equal},
	        {AKEYCODE_A, KeyCode::A},
	        {AKEYCODE_B, KeyCode::B},
	        {AKEYCODE_C, KeyCode::C},
	        {AKEYCODE_D, KeyCode::D},
	        {AKEYCODE_E, KeyCode::E},
	        {AKEYCODE_F, KeyCode::F},
	        {AKEYCODE_G, KeyCode::G},
	        {AKEYCODE_H, KeyCode::H},
	        {AKEYCODE_I, KeyCode::I},
	        {AKEYCODE_J, KeyCode::J},
	        {AKEYCODE_K, KeyCode::K},
	        {AKEYCODE_L, KeyCode::L},
	        {AKEYCODE_M, KeyCode::M},
	        {AKEYCODE_N, KeyCode::N},
	        {AKEYCODE_O, KeyCode::O},
	        {AKEYCODE_P, KeyCode::P},
	        {AKEYCODE_Q, KeyCode::Q},
	        {AKEYCODE_R, KeyCode::R},
	        {AKEYCODE_S, KeyCode::S},
	        {AKEYCODE_T, KeyCode::T},
	        {AKEYCODE_U, KeyCode::U},
	        {AKEYCODE_V, KeyCode::V},
	        {AKEYCODE_W, KeyCode::W},
	        {AKEYCODE_X, KeyCode::X},
	        {AKEYCODE_Y, KeyCode::Y},
	        {AKEYCODE_Z, KeyCode::Z},
	        {AKEYCODE_LEFT_BRACKET, KeyCode::LeftBracket},
	        {AKEYCODE_BACKSLASH, KeyCode::Backslash},
	        {AKEYCODE_RIGHT_BRACKET, KeyCode::RightBracket},
	        {AKEYCODE_ESCAPE, KeyCode::Escape},
	        {AKEYCODE_BACK, KeyCode::Back},
	        {AKEYCODE_ENTER, KeyCode::Enter},
	        {AKEYCODE_TAB, KeyCode::Tab},
	        {AKEYCODE_DEL, KeyCode::Backspace},
	        {AKEYCODE_INSERT, KeyCode::Insert},
	        {AKEYCODE_DEL, KeyCode::DelKey},
	        {AKEYCODE_SYSTEM_NAVIGATION_RIGHT, KeyCode::Right},
	        {AKEYCODE_SYSTEM_NAVIGATION_LEFT, KeyCode::Left},
	        {AKEYCODE_SYSTEM_NAVIGATION_DOWN, KeyCode::Down},
	        {AKEYCODE_SYSTEM_NAVIGATION_UP, KeyCode::Up},
	        {AKEYCODE_PAGE_UP, KeyCode::PageUp},
	        {AKEYCODE_PAGE_DOWN, KeyCode::PageDown},
	        {AKEYCODE_HOME, KeyCode::Home},
	        {AKEYCODE_CAPS_LOCK, KeyCode::CapsLock},
	        {AKEYCODE_SCROLL_LOCK, KeyCode::ScrollLock},
	        {AKEYCODE_NUM_LOCK, KeyCode::NumLock},
	        {AKEYCODE_BREAK, KeyCode::Pause},
	        {AKEYCODE_F1, KeyCode::F1},
	        {AKEYCODE_F2, KeyCode::F2},
	        {AKEYCODE_F3, KeyCode::F3},
	        {AKEYCODE_F4, KeyCode::F4},
	        {AKEYCODE_F5, KeyCode::F5},
	        {AKEYCODE_F6, KeyCode::F6},
	        {AKEYCODE_F7, KeyCode::F7},
	        {AKEYCODE_F8, KeyCode::F8},
	        {AKEYCODE_F9, KeyCode::F9},
	        {AKEYCODE_F10, KeyCode::F10},
	        {AKEYCODE_F11, KeyCode::F11},
	        {AKEYCODE_F12, KeyCode::F12},
	        {AKEYCODE_NUMPAD_0, KeyCode::KP_0},
	        {AKEYCODE_NUMPAD_1, KeyCode::KP_1},
	        {AKEYCODE_NUMPAD_2, KeyCode::KP_2},
	        {AKEYCODE_NUMPAD_3, KeyCode::KP_3},
	        {AKEYCODE_NUMPAD_4, KeyCode::KP_4},
	        {AKEYCODE_NUMPAD_5, KeyCode::KP_5},
	        {AKEYCODE_NUMPAD_6, KeyCode::KP_6},
	        {AKEYCODE_NUMPAD_7, KeyCode::KP_7},
	        {AKEYCODE_NUMPAD_8, KeyCode::KP_8},
	        {AKEYCODE_NUMPAD_9, KeyCode::KP_9},
	        {AKEYCODE_NUMPAD_DOT, KeyCode::KP_Decimal},
	        {AKEYCODE_NUMPAD_DIVIDE, KeyCode::KP_Divide},
	        {AKEYCODE_NUMPAD_MULTIPLY, KeyCode::KP_Multiply},
	        {AKEYCODE_NUMPAD_SUBTRACT, KeyCode::KP_Subtract},
	        {AKEYCODE_NUMPAD_ADD, KeyCode::KP_Add},
	        {AKEYCODE_NUMPAD_ENTER, KeyCode::KP_Enter},
	        {AKEYCODE_NUMPAD_EQUALS, KeyCode::KP_Equal},
	        {AKEYCODE_SHIFT_LEFT, KeyCode::LeftShift},
	        {AKEYCODE_CTRL_LEFT, KeyCode::LeftControl},
	        {AKEYCODE_ALT_LEFT, KeyCode::LeftAlt},
	        {AKEYCODE_SHIFT_RIGHT, KeyCode::RightShift},
	        {AKEYCODE_CTRL_RIGHT, KeyCode::RightControl},
	        {AKEYCODE_ALT_RIGHT, KeyCode::RightAlt}};

	auto key_it = key_lookup.find(key);

	if (key_it == key_lookup.end())
	{
		return KeyCode::Unknown;
	}

	return key_it->second;
}

inline KeyAction translate_key_action(int action)
{
	if (action == AKEY_STATE_DOWN)
	{
		return KeyAction::Down;
	}
	else if (action == AKEY_STATE_UP)
	{
		return KeyAction::Up;
	}

	return KeyAction::Unknown;
}

inline MouseButton translate_mouse_button(int button)
{
	if (button < 3)
	{
		return static_cast<MouseButton>(button);
	}

	return MouseButton::Unknown;
}

inline MouseAction translate_mouse_action(int action)
{
	if (action == AMOTION_EVENT_ACTION_DOWN)
	{
		return MouseAction::Down;
	}
	else if (action == AMOTION_EVENT_ACTION_UP)
	{
		return MouseAction::Up;
	}
	else if (action == AMOTION_EVENT_ACTION_MOVE)
	{
		return MouseAction::Move;
	}

	return MouseAction::Unknown;
}

inline TouchAction translate_touch_action(int action)
{
	action &= AMOTION_EVENT_ACTION_MASK;
	if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN)
	{
		return TouchAction::Down;
	}
	else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP)
	{
		return TouchAction::Up;
	}
	else if (action == AMOTION_EVENT_ACTION_CANCEL)
	{
		return TouchAction::Cancel;
	}
	else if (action == AMOTION_EVENT_ACTION_MOVE)
	{
		return TouchAction::Move;
	}

	return TouchAction::Unknown;
}

void on_content_rect_changed(GameActivity *activity, const ARect *rect)
{
	LOGI("ContentRectChanged: {:p}\n", static_cast<void *>(activity));
	struct android_app *app = reinterpret_cast<struct android_app *>(activity->instance);
	auto                cmd = APP_CMD_CONTENT_RECT_CHANGED;

	app->contentRect = *rect;

	if (write(app->msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd))
	{
		LOGE("Failure writing android_app cmd: {}\n", strerror(errno));
	}
}

void on_app_cmd(android_app *app, int32_t cmd)
{
	auto platform = reinterpret_cast<AndroidPlatform *>(app->userData);
	assert(platform && "Platform is not valid");

	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
		{
			platform->resize(ANativeWindow_getWidth(app->window),
			                 ANativeWindow_getHeight(app->window));
			platform->set_surface_ready();
			break;
		}
		case APP_CMD_CONTENT_RECT_CHANGED:
		{
			// Get the new size
			auto width  = app->contentRect.right - app->contentRect.left;
			auto height = app->contentRect.bottom - app->contentRect.top;
			platform->resize(width, height);
			break;
		}
		case APP_CMD_GAINED_FOCUS:
		{
			platform->set_focus(true);
			break;
		}
		case APP_CMD_LOST_FOCUS:
		{
			platform->set_focus(false);
			break;
		}
	}
}

bool key_event_filter(const GameActivityKeyEvent *event)
{
	if (event->source == AINPUT_SOURCE_KEYBOARD)
	{
		return true;
	}
	return false;
}

bool motion_event_filter(const GameActivityMotionEvent *event)
{
	if ((event->source == AINPUT_SOURCE_MOUSE) ||
	    (event->source == AINPUT_SOURCE_TOUCHSCREEN))
	{
		return true;
	}
	return false;
}
}        // namespace

namespace fs
{
void create_directory(const std::string &path)
{
	if (!is_directory(path))
	{
		mkdir(path.c_str(), 0777);
	}
}
}        // namespace fs

AndroidPlatform::AndroidPlatform(android_app *app) :
    app{app}
{
}

ExitCode AndroidPlatform::initialize(const std::vector<Plugin *> &plugins)
{
	android_app_set_key_event_filter(app, key_event_filter);
	android_app_set_motion_event_filter(app, motion_event_filter);

	app->onAppCmd                                  = on_app_cmd;
	app->activity->callbacks->onContentRectChanged = on_content_rect_changed;
	app->userData                                  = this;

	auto code = Platform::initialize(plugins);
	if (code != ExitCode::Success)
	{
		return code;
	}

	// Wait until the android window is loaded before allowing the app to continue
	LOGI("Waiting on window surface to be ready");
	do
	{
		if (!process_android_events(app))
		{
			// Android requested for the app to close
			LOGI("Android app has been destroyed by the OS");
			return ExitCode::Close;
		}
	} while (!surface_ready);

	return ExitCode::Success;
}

void AndroidPlatform::create_window(const Window::Properties &properties)
{
	// Android window uses native window size
	// Required so that the vulkan sample can create a VkSurface
	window = std::make_unique<AndroidWindow>(this, app->window, properties);
}

void AndroidPlatform::process_android_input_events(void)
{
	auto input_buf = android_app_swap_input_buffers(app);
	if (!input_buf)
	{
		return;
	}
	if (input_buf->motionEventsCount)
	{
		for (int idx = 0; idx < input_buf->motionEventsCount; idx++)
		{
			auto event = &input_buf->motionEvents[idx];
			assert((event->source == AINPUT_SOURCE_MOUSE ||
			        event->source == AINPUT_SOURCE_TOUCHSCREEN) &&
			       "Invalid motion event source");

			std::int32_t action = event->action;

			float x = GameActivityPointerAxes_getX(&event->pointers[0]);
			float y = GameActivityPointerAxes_getY(&event->pointers[0]);

			if (event->source == AINPUT_SOURCE_MOUSE)
			{
				input_event(MouseButtonInputEvent{
				    translate_mouse_button(0),
				    translate_mouse_action(action),
				    x, y});
			}
			else if (event->source == AINPUT_SOURCE_TOUCHSCREEN)
			{
				// Multiple pointers are not supported.
				size_t       pointer_count = event->pointerCount;
				std::int32_t pointer_id    = event->pointers[0].id;

				input_event(TouchInputEvent{
				    pointer_id,
				    pointer_count,
				    translate_touch_action(action),
				    x, y});
			}
		}
		android_app_clear_motion_events(input_buf);
	}

	if (input_buf->keyEventsCount)
	{
		for (int idx = 0; idx < input_buf->keyEventsCount; idx++)
		{
			auto event = &input_buf->keyEvents[idx];
			assert((event->source == AINPUT_SOURCE_KEYBOARD) &&
			       "Invalid key event source");
			input_event(KeyInputEvent{
			    translate_key_code(event->keyCode),
			    translate_key_action(event->action)});
		}
		android_app_clear_key_events(input_buf);
	}
}

void AndroidPlatform::terminate(ExitCode code)
{
	switch (code)
	{
		case ExitCode::Success:
		case ExitCode::Close:
			log_output.clear();
			break;
		case ExitCode::FatalError:
			send_notification(log_output);
			break;
		default:
			break;
	}

	while (process_android_events(app))
	{
		// Process events until app->destroyRequested is set
	}

	Platform::terminate(code);
}

void AndroidPlatform::send_notification(const std::string &message)
{
	JNIEnv *env;
	app->activity->vm->AttachCurrentThread(&env, NULL);
	jclass    cls         = env->GetObjectClass(app->activity->javaGameActivity);
	jmethodID fatal_error = env->GetMethodID(cls, "fatalError", "(Ljava/lang/String;)V");
	env->CallVoidMethod(app->activity->javaGameActivity, fatal_error, env->NewStringUTF(message.c_str()));
	app->activity->vm->DetachCurrentThread();
}

void AndroidPlatform::set_surface_ready()
{
	surface_ready = true;
}

GameActivity *AndroidPlatform::get_activity()
{
	return app->activity;
}

android_app *AndroidPlatform::get_android_app()
{
	return app;
}

std::vector<spdlog::sink_ptr> AndroidPlatform::get_platform_sinks()
{
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::android_sink_mt>(PROJECT_NAME));

	char        timestamp[80];
	std::time_t time = std::time(0);
	std::tm     now  = thread_safe_time(time);
	std::strftime(timestamp, 80, "%G-%m-%d_%H-%M-%S_log.txt", &now);
	log_output = vkb::fs::path::get(vkb::fs::path::Logs) + std::string(timestamp);

	sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_output, true));

	return sinks;
}
}        // namespace vkb
