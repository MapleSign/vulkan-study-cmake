set(ENGINE_FILES 
    Camera.h
    Mesh.h
    Model.h
    Scene.h
    Vertex.h
    Texture.h
    Light.h

    Camera.cpp
    Mesh.cpp
    Model.cpp
    Scene.cpp
    Vertex.cpp
    Light.cpp
    
    main.cpp
)

set(VULKAN_FRAMEWORK_FILES 
    ./Vulkan/VulkanApplication.h
    ./Vulkan/VulkanBuffer.h
    ./Vulkan/VulkanCommandBuffer.h
    ./Vulkan/VulkanCommandPool.h
    ./Vulkan/VulkanCommon.h
    ./Vulkan/VulkanDescriptorPool.h
    ./Vulkan/VulkanDescriptorSet.h
    ./Vulkan/VulkanDescriptorSetLayout.h
    ./Vulkan/VulkanDevice.h
    ./Vulkan/VulkanFence.h
    ./Vulkan/VulkanFrameBuffer.h
    ./Vulkan/VulkanGraphicsPipeline.h
    ./Vulkan/VulkanImage.h
    ./Vulkan/VulkanImageView.h
    ./Vulkan/VulkanInclude.h
    ./Vulkan/VulkanInstance.h
    ./Vulkan/VulkanPhysicalDevice.h
    ./Vulkan/VulkanPipeline.h
    ./Vulkan/VulkanPipelineLayout.h
    ./Vulkan/VulkanQueue.h
    ./Vulkan/VulkanRayTracingPipeline.h
    ./Vulkan/VulkanRenderPass.h
    ./Vulkan/VulkanRenderTarget.h
    ./Vulkan/VulkanSemaphore.h
    ./Vulkan/VulkanShaderModule.h
    ./Vulkan/VulkanSwapChain.h
    ./Vulkan/VulkanTexture.h

    ./Vulkan/VulkanApplication.cpp
    ./Vulkan/VulkanBuffer.cpp
    ./Vulkan/VulkanCommandBuffer.cpp
    ./Vulkan/VulkanCommandPool.cpp
    ./Vulkan/VulkanCommon.cpp
    ./Vulkan/VulkanDescriptorPool.cpp
    ./Vulkan/VulkanDescriptorSet.cpp
    ./Vulkan/VulkanDescriptorSetLayout.cpp
    ./Vulkan/VulkanDevice.cpp
    ./Vulkan/VulkanFence.cpp
    ./Vulkan/VulkanFrameBuffer.cpp
    ./Vulkan/VulkanGraphicsPipeline.cpp
    ./Vulkan/VulkanImage.cpp
    ./Vulkan/VulkanImageView.cpp
    ./Vulkan/VulkanInstance.cpp
    ./Vulkan/VulkanPhysicalDevice.cpp
    ./Vulkan/VulkanPipeline.cpp
    ./Vulkan/VulkanPipelineLayout.cpp
    ./Vulkan/VulkanQueue.cpp
    ./Vulkan/VulkanRayTracingPipeline.cpp
    ./Vulkan/VulkanRenderPass.cpp
    ./Vulkan/VulkanRenderTarget.cpp
    ./Vulkan/VulkanSemaphore.cpp
    ./Vulkan/VulkanShaderModule.cpp
    ./Vulkan/VulkanSwapChain.cpp
    ./Vulkan/VulkanTexture.cpp
)

set(RENDERING_FILES 
    ./Vulkan/Rendering/VulkanRayTracingBuilder.h
    ./Vulkan/Rendering/VulkanGraphicsBuilder.h
    ./Vulkan/Rendering/VulkanRenderContext.h
    ./Vulkan/Rendering/VulkanRenderFrame.h
    ./Vulkan/Rendering/VulkanRenderPipeline.h
    ./Vulkan/Rendering/VulkanResource.h
    ./Vulkan/Rendering/VulkanSubpass.h

    ./Vulkan/Rendering/VulkanRayTracingBuilder.cpp
    ./Vulkan/Rendering/VulkanGraphicsBuilder.cpp
    ./Vulkan/Rendering/VulkanRenderContext.cpp
    ./Vulkan/Rendering/VulkanRenderFrame.cpp
    ./Vulkan/Rendering/VulkanRenderPipeline.cpp
    ./Vulkan/Rendering/VulkanResource.cpp
    ./Vulkan/Rendering/VulkanSubpass.cpp
)

set(SUBPASSES_FILES
    ./Vulkan/Rendering/Subpasses/GlobalSubpass.h
    ./Vulkan/Rendering/Subpasses/LightingSubpass.h
    ./Vulkan/Rendering/Subpasses/SkyboxSubpass.h
    ./Vulkan/Rendering/Subpasses/SSAOSubpass.h
    
    ./Vulkan/Rendering/Subpasses/GlobalSubpass.cpp
    ./Vulkan/Rendering/Subpasses/LightingSubpass.cpp
    ./Vulkan/Rendering/Subpasses/SkyboxSubpass.cpp
    ./Vulkan/Rendering/Subpasses/SSAOSubpass.cpp
)

set(PLATFORM_FILES 
    ./Platform/GlfwWindow.h

    ./Platform/GlfwWindow.cpp
)

set(GUI_FILES 
    ./GUI/GUI.h

    ./GUI/GUI.cpp
)

set(COMPONENT_FILES 
    ./Component/TransformComponent.h

    ./Component/TransformComponent.cpp
)

set(GLTF_FILES
    ./GLTF/GLTFHelper.h
    
    ./GLTF/GLTFHelper.cpp
)

source_group("\\" FILES ${ENGINE_FILES})
source_group("platform\\" FILES ${PLATFORM_FILES})
source_group("gui\\" FILES ${GUI_FILES})
source_group("component\\" FILES ${COMPONENT_FILES})
source_group("GLTF\\" FILES ${GLTF_FILES})
source_group("vulkan\\" FILES ${VULKAN_FRAMEWORK_FILES})
source_group("vulkan\\rendering\\" FILES ${RENDERING_FILES})
source_group("vulkan\\rendering\\subpasses" FILES ${SUBPASSES_FILES})

# Add source to this project's executable.
add_executable(vulkan_study 
    ${ENGINE_FILES}
    ${VULKAN_FRAMEWORK_FILES}
    ${RENDERING_FILES}
    ${SUBPASSES_FILES}
    ${PLATFORM_FILES}
    ${GUI_FILES}
    ${COMPONENT_FILES}
    ${GLTF_FILES}
)

# set_property(TARGET vulkan_study PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkan_study>")
set_property(TARGET vulkan_study PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

target_include_directories(vulkan_study PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}" 
    "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan"
)
target_link_libraries(vulkan_study PUBLIC vma glm imgui stb volk tinygltf)

target_link_libraries(vulkan_study PUBLIC Vulkan::Vulkan glfw)

add_dependencies(vulkan_study Shaders)
