set(ENGINE_FILES 
    Camera.cpp
    main.cpp
    Mesh.cpp
    Model.cpp
    Scene.cpp
    Vertex.cpp
)

set(VULKAN_FRAMEWORK_FILES 
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
    ./Vulkan/Rendering/VulkanApplication.cpp
    ./Vulkan/Rendering/VulkanBuffer.cpp
    ./Vulkan/Rendering/VulkanCommandBuffer.cpp
    ./Vulkan/Rendering/VulkanCommandPool.cpp
    ./Vulkan/Rendering/VulkanCommon.cpp
    ./Vulkan/Rendering/VulkanDescriptorPool.cpp
    ./Vulkan/Rendering/VulkanDescriptorSet.cpp
    ./Vulkan/Rendering/VulkanDescriptorSetLayout.cpp
    ./Vulkan/Rendering/VulkanDevice.cpp
    ./Vulkan/Rendering/VulkanFence.cpp
    ./Vulkan/Rendering/VulkanFrameBuffer.cpp
    ./Vulkan/Rendering/VulkanGraphicsPipeline.cpp
    ./Vulkan/Rendering/VulkanImage.cpp
    ./Vulkan/Rendering/VulkanImageView.cpp
    ./Vulkan/Rendering/VulkanInstance.cpp
    ./Vulkan/Rendering/VulkanPhysicalDevice.cpp
    ./Vulkan/Rendering/VulkanPipeline.cpp
    ./Vulkan/Rendering/VulkanPipelineLayout.cpp
    ./Vulkan/Rendering/VulkanQueue.cpp
    ./Vulkan/Rendering/VulkanRayTracingPipeline.cpp
    ./Vulkan/Rendering/VulkanRenderPass.cpp
    ./Vulkan/Rendering/VulkanRenderTarget.cpp
    ./Vulkan/Rendering/VulkanSemaphore.cpp
    ./Vulkan/Rendering/VulkanShaderModule.cpp
    ./Vulkan/Rendering/VulkanSwapChain.cpp
    ./Vulkan/Rendering/VulkanTexture.cpp
)

set(PLATFORM_FILES 
    ./Platform/GlfwWindow.cpp
)

set(GUI_FILES 
    ./GUI/GUI.cpp
)

set(COMPONENT_FILES 
    ./Component/TransformComponent.cpp
)

# Add source to this project's executable.
add_executable(vulkan_study 
    ${ENGINE_FILES}
    ${VULKAN_FRAMEWORK_FILES}
    ${RENDERING_FILES}
    ${PLATFORM_FILES}
    ${GUI_FILES}
    ${COMPONENT_FILES}
)

set_property(TARGET vulkan_study PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkan_study>")

target_include_directories(vulkan_study PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkan_study vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(vulkan_study Vulkan::Vulkan sdl2)

add_dependencies(vulkan_study Shaders)
