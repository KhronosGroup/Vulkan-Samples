get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
get_filename_component(CATEGORY_NAME ${PARENT_DIR} NAME)

add_library( octomath STATIC
        octomap/octomap/src/math/Vector3.cpp
        octomap/octomap/src/math/Quaternion.cpp
        octomap/octomap/src/math/Pose6D.cpp
)
target_include_directories(octomath PUBLIC ${CMAKE_CURRENT_LIST_DIR}/octomap/octomap/include)

add_library(octomap SHARED
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

add_library(dynamicEDT3d SHARED octomap/dynamicEDT3D/src/dynamicEDT3D.cpp)
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
        LIBS
            dynamicEDT3d
)