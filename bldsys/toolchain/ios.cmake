# As per upstream CMake recommendation in https://gitlab.kitware.com/cmake/cmake/-/issues/27661
# to properly build for iOS we need a toolchain file.
# Toolchain file shall define:
# - Target system name to iOS
# - Export the $ENV{VULKAN_SDK} variable to `CMAKE_FIND_ROOT_PATH`
# iOS/setup-env.sh will take care of setting proper environment.

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_OSX_DEPLOYMENT_TARGET 16.3)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
list(APPEND CMAKE_FIND_ROOT_PATH "$ENV{VULKAN_SDK}")
