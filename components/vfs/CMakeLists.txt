set(PROJECT_FILES
    src/root_file_system.cpp
    src/helpers.cpp
)

set(ANDROID_FILES
    src/android_aasset_manager.cpp
    src/android.cpp
)

set(UNIX_FILES
    src/unix.cpp
)

set(WINDOWS_FILES
    src/windows.cpp
)

if(ANDROID)
    list(APPEND PROJECT_FILES ${ANDROID_FILES})
    list(APPEND PROJECT_FILES ${UNIX_FILES})
elseif(WIN32)
    list(APPEND PROJECT_FILES ${WINDOWS_FILES})
else()
    list(APPEND PROJECT_FILES ${UNIX_FILES})
endif()

add_library(components_vfs STATIC ${PROJECT_FILES})
target_include_directories(components_vfs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (ANDROID)
    target_link_libraries(components_vfs PRIVATE log android native_app_glue)
endif()

if(${VKB_BUNDLE_ASSETS})
    target_compile_definitions(components_vfs PUBLIC VKB_BUNDLE_ASSETS)
endif()

add_library(Components::VFS ALIAS components_vfs)