#include "GUI.h"

#include "../Platform/GlfwWindow.h"

GUI::GUI(const VulkanInstance& instance, const GlfwWindow& window, const VulkanDevice& device, const VulkanRenderPass& renderPass,
    uint32_t minImageCount, uint32_t imageCount, VkSampleCountFlagBits MSAASamples):
	device{device}, minImageCount{minImageCount}, imageCount{imageCount}, MSAASamples{MSAASamples}
{
    // Create DescriptorPool
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VK_CHECK(vkCreateDescriptorPool(device.getHandle(), &pool_info, nullptr, &imguiDescriptorPool));
    }

    // Setup Dear ImGui context
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui_ImplVulkan_LoadFunctions(
        [](const char* name, void* data) {
            return vkGetInstanceProcAddr(reinterpret_cast<VulkanInstance*>(data)->getHandle(), name);
        }, 
        const_cast<VulkanInstance*>(&instance)
    );

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = instance.getHandle();
    init_info.PhysicalDevice = device.getGPU().getHandle();
    init_info.Device = device.getHandle();
    init_info.QueueFamily = device.getGraphicsQueue().getFamilyIndex();
    init_info.Queue = device.getGraphicsQueue().getHandle();
    init_info.DescriptorPool = imguiDescriptorPool;
    init_info.MinImageCount = minImageCount;
    init_info.ImageCount = imageCount;
    init_info.MSAASamples = MSAASamples;
    ImGui_ImplVulkan_Init(&init_info, renderPass.getHandle());

    // Setup Dear ImGui font texture
    auto tmpCmd = device.getCommandPool().beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(tmpCmd->getHandle());
    device.getCommandPool().endSingleTimeCommands(*tmpCmd, device.getGraphicsQueue());
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window.getHandle(), true);
}

GUI::~GUI()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(device.getHandle(), imguiDescriptorPool, nullptr);
}

void GUI::newFrame() const
{
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
}

void GUI::render() const
{
    ImGui::Render();
}

void GUI::renderDrawData(const VkCommandBuffer& cmd) const
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
