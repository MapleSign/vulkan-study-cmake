# CMakeList.txt : CMake project for vulkan_guide, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.21)

project ("vulkan_study_cmake")

#set MSVC multi-threaded compilation
if (MSVC)
    add_definitions("/MP")
endif()

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

add_subdirectory(third_party)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory(src)


find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

# find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_INCLUDE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.glsl"
    )

set(GLSL_SOURCE_FILES 
    "${PROJECT_SOURCE_DIR}/shaders/shader.frag"
    "${PROJECT_SOURCE_DIR}/shaders/shader.vert"

    "${PROJECT_SOURCE_DIR}/shaders/passthrough.vert"
    "${PROJECT_SOURCE_DIR}/shaders/post.frag"

    "${PROJECT_SOURCE_DIR}/shaders/shadow.vert"

    "${PROJECT_SOURCE_DIR}/shaders/dirShadow.geom"
    "${PROJECT_SOURCE_DIR}/shaders/dirShadow.frag"

    "${PROJECT_SOURCE_DIR}/shaders/pointShadow.geom"
    "${PROJECT_SOURCE_DIR}/shaders/pointShadow.frag"
    
    "${PROJECT_SOURCE_DIR}/shaders/skybox.vert"
    "${PROJECT_SOURCE_DIR}/shaders/skybox.frag"

    "${PROJECT_SOURCE_DIR}/shaders/raytrace.rgen"
    "${PROJECT_SOURCE_DIR}/shaders/raytrace.rmiss"
    "${PROJECT_SOURCE_DIR}/shaders/raytrace.rchit"
    "${PROJECT_SOURCE_DIR}/shaders/raytraceShadow.rmiss"
    "${PROJECT_SOURCE_DIR}/shaders/raytrace.rahit"
)

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/spv/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    ##execute glslang command to compile that specific shader
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.3 -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )
