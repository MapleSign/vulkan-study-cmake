find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_library(glm INTERFACE)
add_library(vma INTERFACE)

add_library(stb INTERFACE)

#both vma and glm and header only libs so we only need the include path
target_include_directories(vma INTERFACE vma)
target_include_directories(glm INTERFACE glm)

add_definitions(-DVK_NO_PROTOTYPES)
add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui glfw imgui/backends)

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

target_include_directories(stb INTERFACE stb)

add_library(volk STATIC)
target_link_libraries(volk PUBLIC Vulkan::Vulkan)
target_include_directories(volk PUBLIC volk)
target_sources(volk PRIVATE 
    volk/volk.c
)