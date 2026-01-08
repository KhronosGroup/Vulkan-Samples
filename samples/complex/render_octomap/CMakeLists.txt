# Copyright (c) 2025, Holochip Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
get_filename_component(CATEGORY_NAME ${PARENT_DIR} NAME)

# On Windows, build as STATIC to avoid DLL export issues
# On other platforms, build as SHARED for better memory efficiency
if(WIN32)
    set(OCTOMAP_LIBRARY_TYPE STATIC)
else()
    set(OCTOMAP_LIBRARY_TYPE SHARED)
endif()

add_library( octomath STATIC
        octomap/octomap/src/math/Vector3.cpp
        octomap/octomap/src/math/Quaternion.cpp
        octomap/octomap/src/math/Pose6D.cpp
)
set_target_properties(octomath PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(octomath PUBLIC ${CMAKE_CURRENT_LIST_DIR}/octomap/octomap/include)

add_library(octomap ${OCTOMAP_LIBRARY_TYPE}
        octomap/octomap/src/AbstractOcTree.cpp
        octomap/octomap/src/AbstractOccupancyOcTree.cpp
        octomap/octomap/src/Pointcloud.cpp
        octomap/octomap/src/ScanGraph.cpp
        octomap/octomap/src/CountingOcTree.cpp
        octomap/octomap/src/OcTree.cpp
        octomap/octomap/src/OcTreeNode.cpp
        octomap/octomap/src/OcTreeStamped.cpp
        octomap/octomap/src/ColorOcTree.cpp
)
target_link_libraries(octomap PUBLIC octomath)

add_library(dynamicEDT3d ${OCTOMAP_LIBRARY_TYPE} octomap/dynamicEDT3D/src/dynamicEDT3D.cpp)
target_include_directories(dynamicEDT3d PUBLIC octomap/dynamicEDT3D/include)
target_link_libraries(dynamicEDT3d PUBLIC octomap)

add_sample_with_tags(
        ID ${FOLDER_NAME}
        CATEGORY ${CATEGORY_NAME}
        AUTHOR "Holochip"
        NAME "Octmap rendering"
        DESCRIPTION "Demonstration of how to render an OctMap which was generated from ICP combining ARCore and ARKit SLAM maps"
        FILES
            ImGUIUtil.cpp
            Screens/MapView.cpp
        SHADER_FILES_GLSL
        "render_octomap/glsl/render.vert"
        "render_octomap/glsl/render.frag"
        "render_octomap/glsl/imgui.vert"
        "render_octomap/glsl/imgui.frag"
        "render_octomap/glsl/gltf.vert"
        "render_octomap/glsl/gltf.frag"
        "render_octomap/glsl/splat.vert"
        "render_octomap/glsl/splat.frag"
        LIBS
            dynamicEDT3d
)
