find_package(Vulkan REQUIRED)

option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_INSTALL OFF)
option(BUILD_SHARED_LIBS ON)
add_subdirectory(glfw)

add_definitions(-DGLM_FORCE_RADIANS)
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
add_library(glm INTERFACE)
add_library(vma INTERFACE)
add_library(stb INTERFACE)
add_library(tinygltf INTERFACE)

# header only lib
target_include_directories(vma INTERFACE vma)
target_include_directories(glm INTERFACE glm)
target_include_directories(stb INTERFACE stb)
target_include_directories(tinygltf INTERFACE tinygltf)
target_sources(tinygltf INTERFACE tinygltf/tiny_gltf.h tinygltf/json.hpp)

# imgui
add_definitions(-DVK_NO_PROTOTYPES)
add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui imgui/backends)

target_sources(imgui PRIVATE 
    imgui/imgui.h
    imgui/imgui.cpp

    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp

    imgui/backends/imgui_impl_vulkan.cpp
    imgui/backends/imgui_impl_glfw.cpp
)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)

# volk
add_library(volk STATIC)
target_link_libraries(volk PUBLIC Vulkan::Vulkan)
target_include_directories(volk PUBLIC volk)
target_sources(volk PRIVATE 
    volk/volk.c
)