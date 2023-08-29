# Copyright 2022-2023 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Configure Vulkan Guide Antora tree with transformed markup files.
# Branch selection may come later. For now it is the current branch.

function(gatherAntoraAssets)
    set(DIRS_TO_SEARCH
            app
            assets
            components
            docs
            framework
            samples
            scripts
            shaders
            tests
    )
    set(PAGES_DIR_SEARCH)
    set(IMAGES_DIR_SEARCH)
    foreach (DIR ${DIRS_TO_SEARCH})
        list(APPEND PAGES_DIR_SEARCH ${CMAKE_SOURCE_DIR}/${DIR}/*.adoc)
        list(APPEND IMAGES_DIR_SEARCH ${CMAKE_SOURCE_DIR}/${DIR}/*.jpg ${CMAKE_SOURCE_DIR}/${DIR}/*.png ${CMAKE_SOURCE_DIR}/${DIR}/*.gif)
    endforeach ()
    file(GLOB PAGES ${CMAKE_SOURCE_DIR}/*.adoc)
    file(GLOB IMAGES ${CMAKE_SOURCE_DIR}/*.jpg ${CMAKE_SOURCE_DIR}/*.png ${CMAKE_SOURCE_DIR}/*.gif)
    file(GLOB_RECURSE PAGES_R ${PAGES_DIR_SEARCH})
    file(GLOB_RECURSE IMAGES_R ${IMAGES_DIR_SEARCH})

    foreach(page ${PAGES})
        file(COPY ${page} DESTINATION ${CMAKE_CURRENT_LIST_DIR}/modules/ROOT/pages)
    endforeach ()

    foreach(image ${IMAGES})
        file(COPY ${image} DESTINATION ${CMAKE_CURRENT_LIST_DIR}/modules/ROOT/images)
    endforeach ()

    foreach (page ${PAGES_R})
        file(RELATIVE_PATH relpage ${CMAKE_SOURCE_DIR} ${page})
        get_filename_component(directory ${relpage} DIRECTORY)
        file(COPY ${page} DESTINATION ${CMAKE_CURRENT_LIST_DIR}/modules/ROOT/pages/${directory})
    endforeach ()

    foreach(image ${IMAGES_R})
        file(RELATIVE_PATH relpage ${CMAKE_SOURCE_DIR} ${image})
        get_filename_component(directory ${relpage} DIRECTORY)
        file(COPY ${image} DESTINATION ${CMAKE_CURRENT_LIST_DIR}/modules/ROOT/images/${directory})
    endforeach ()
endfunction()

if(VKB_GENERATE_ANTORA_SITE)
    gatherAntoraAssets()
endif()