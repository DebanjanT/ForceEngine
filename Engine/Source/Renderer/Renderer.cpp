#include "Force/Renderer/Renderer.h"
#include "Force/Core/Application.h"
#include "Force/Core/Logger.h"

namespace Force
{
    Scope<VulkanContext>   Renderer::s_Context   = nullptr;
    Scope<VulkanDevice>    Renderer::s_Device    = nullptr;
    Scope<VulkanSwapchain> Renderer::s_Swapchain = nullptr;

    std::vector<VkCommandBuffer> Renderer::s_CommandBuffers;
    std::vector<VkSemaphore>     Renderer::s_ImageAvailableSemaphores;
    std::vector<VkSemaphore>     Renderer::s_RenderFinishedSemaphores;
    std::vector<VkFence>         Renderer::s_InFlightFences;

    u32  Renderer::s_CurrentFrame       = 0;
    u32  Renderer::s_MaxFramesInFlight  = 2;
    bool Renderer::s_FramebufferResized = false;

    glm::vec4 Renderer::s_ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

    VkDescriptorPool                        Renderer::s_DescriptorPool             = VK_NULL_HANDLE;
    VkDescriptorSetLayout                   Renderer::s_CameraDescriptorSetLayout  = VK_NULL_HANDLE;
    std::vector<UniformBuffer>              Renderer::s_CameraUBOs;
    std::vector<VkDescriptorSet>            Renderer::s_CameraDescriptorSets;

    // -------------------------------------------------------------------------
    void Renderer::Init(const RendererConfig& config)
    {
        FORCE_CORE_INFO("Initializing Renderer...");

        s_MaxFramesInFlight = config.MaxFramesInFlight;

        s_Context = CreateScope<VulkanContext>();
        s_Context->Init(config);

        VkSurfaceKHR surface;
        Application::Get().GetWindow().CreateVulkanSurface(s_Context->GetInstance(), &surface);
        s_Context->SetSurface(surface);

        s_Device = CreateScope<VulkanDevice>();
        s_Device->Init(s_Context->GetInstance(), s_Context->GetSurface());

        s_Swapchain = CreateScope<VulkanSwapchain>();
        s_Swapchain->Init(
            Application::Get().GetWindow().GetWidth(),
            Application::Get().GetWindow().GetHeight(),
            config.VSync
        );

        CreateSyncObjects();
        CreateCommandBuffers();
        CreateDescriptorPool();
        CreateCameraResources();

        FORCE_CORE_INFO("Renderer initialized successfully");
    }

    void Renderer::Shutdown()
    {
        FORCE_CORE_INFO("Shutting down Renderer...");

        s_Device->WaitIdle();

        // Camera resources
        s_CameraUBOs.clear();
        if (s_CameraDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(s_Device->GetDevice(),
                                          s_CameraDescriptorSetLayout, nullptr);
            s_CameraDescriptorSetLayout = VK_NULL_HANDLE;
        }

        // Descriptor pool (frees all sets implicitly)
        if (s_DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(s_Device->GetDevice(), s_DescriptorPool, nullptr);
            s_DescriptorPool = VK_NULL_HANDLE;
        }

        DestroySyncObjects();

        s_Swapchain->Shutdown();
        s_Swapchain.reset();

        s_Device->Shutdown();
        s_Device.reset();

        s_Context->Shutdown();
        s_Context.reset();
    }

    // -------------------------------------------------------------------------
    void Renderer::BeginFrame()
    {
        VkDevice device = s_Device->GetDevice();

        vkWaitForFences(device, 1, &s_InFlightFences[s_CurrentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = s_Swapchain->AcquireNextImage(s_ImageAvailableSemaphores[s_CurrentFrame]);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            OnWindowResize(Application::Get().GetWindow().GetWidth(),
                           Application::Get().GetWindow().GetHeight());
            return;
        }

        vkResetFences(device, 1, &s_InFlightFences[s_CurrentFrame]);

        VkCommandBuffer cmd = s_CommandBuffers[s_CurrentFrame];
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        // Begin swapchain render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = s_Swapchain->GetRenderPass();
        renderPassInfo.framebuffer       = s_Swapchain->GetFramebuffer(s_Swapchain->GetCurrentImageIndex());
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = s_Swapchain->GetExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color        = {{s_ClearColor.r, s_ClearColor.g, s_ClearColor.b, s_ClearColor.a}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
        renderPassInfo.pClearValues    = clearValues.data();

        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<f32>(s_Swapchain->GetExtent().width);
        viewport.height   = static_cast<f32>(s_Swapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = s_Swapchain->GetExtent();
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    void Renderer::EndFrame()
    {
        VkCommandBuffer cmd = s_CommandBuffers[s_CurrentFrame];

        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore          waitSemaphores[] = {s_ImageAvailableSemaphores[s_CurrentFrame]};
        VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphores;
        submitInfo.pWaitDstStageMask    = waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &cmd;

        VkSemaphore signalSemaphores[] = {s_RenderFinishedSemaphores[s_CurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;

        VkResult result = vkQueueSubmit(s_Device->GetGraphicsQueue(), 1,
                                         &submitInfo, s_InFlightFences[s_CurrentFrame]);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to submit draw command buffer!");

        result = s_Swapchain->Present(s_RenderFinishedSemaphores[s_CurrentFrame]);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || s_FramebufferResized)
        {
            s_FramebufferResized = false;
            OnWindowResize(Application::Get().GetWindow().GetWidth(),
                           Application::Get().GetWindow().GetHeight());
        }

        s_CurrentFrame = (s_CurrentFrame + 1) % s_MaxFramesInFlight;
    }

    void Renderer::OnWindowResize(u32 width, u32 height)
    {
        if (width == 0 || height == 0) return;
        s_Device->WaitIdle();
        s_Swapchain->Recreate(width, height);
    }

    void Renderer::SetClearColor(const glm::vec4& color)
    {
        s_ClearColor = color;
    }

    void Renderer::UpdateCamera(const CameraUBO& data)
    {
        s_CameraUBOs[s_CurrentFrame].SetData(&data, sizeof(CameraUBO));
    }

    // -------------------------------------------------------------------------
    // Private init helpers
    // -------------------------------------------------------------------------
    void Renderer::CreateSyncObjects()
    {
        s_ImageAvailableSemaphores.resize(s_MaxFramesInFlight);
        s_RenderFinishedSemaphores.resize(s_MaxFramesInFlight);
        s_InFlightFences.resize(s_MaxFramesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkDevice device = s_Device->GetDevice();

        for (u32 i = 0; i < s_MaxFramesInFlight; i++)
        {
            FORCE_CORE_ASSERT(
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &s_ImageAvailableSemaphores[i]) == VK_SUCCESS &&
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &s_RenderFinishedSemaphores[i]) == VK_SUCCESS &&
                vkCreateFence(device, &fenceInfo, nullptr, &s_InFlightFences[i])                  == VK_SUCCESS,
                "Failed to create synchronization objects!");
        }
    }

    void Renderer::DestroySyncObjects()
    {
        VkDevice device = s_Device->GetDevice();
        for (u32 i = 0; i < s_MaxFramesInFlight; i++)
        {
            vkDestroySemaphore(device, s_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, s_RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, s_InFlightFences[i], nullptr);
        }
    }

    void Renderer::CreateCommandBuffers()
    {
        s_CommandBuffers.resize(s_MaxFramesInFlight);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = s_Device->GetCommandPool();
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<u32>(s_CommandBuffers.size());

        VkResult result = vkAllocateCommandBuffers(s_Device->GetDevice(),
                                                    &allocInfo, s_CommandBuffers.data());
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffers!");
    }

    void Renderer::CreateDescriptorPool()
    {
        // Large general-purpose pool shared by all materials and systems
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 10000;
        poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 10000;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
        poolInfo.pPoolSizes    = poolSizes.data();
        poolInfo.maxSets       = 10000;

        VkResult result = vkCreateDescriptorPool(s_Device->GetDevice(),
                                                  &poolInfo, nullptr, &s_DescriptorPool);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor pool!");
    }

    void Renderer::CreateCameraResources()
    {
        VkDevice device = s_Device->GetDevice();

        // Descriptor set layout for set 0: camera UBO at binding 0
        VkDescriptorSetLayoutBinding binding{};
        binding.binding         = 0;
        binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings    = &binding;

        VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                                       &s_CameraDescriptorSetLayout);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create camera descriptor set layout!");

        // Per-frame UBOs + descriptor sets
        s_CameraUBOs.resize(s_MaxFramesInFlight);
        s_CameraDescriptorSets.resize(s_MaxFramesInFlight);

        std::vector<VkDescriptorSetLayout> layouts(s_MaxFramesInFlight, s_CameraDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = s_DescriptorPool;
        allocInfo.descriptorSetCount = s_MaxFramesInFlight;
        allocInfo.pSetLayouts        = layouts.data();

        result = vkAllocateDescriptorSets(device, &allocInfo, s_CameraDescriptorSets.data());
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate camera descriptor sets!");

        for (u32 i = 0; i < s_MaxFramesInFlight; i++)
        {
            s_CameraUBOs[i].Create(sizeof(CameraUBO));

            VkDescriptorBufferInfo bufferInfo = s_CameraUBOs[i].GetDescriptorInfo();

            VkWriteDescriptorSet write{};
            write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet          = s_CameraDescriptorSets[i];
            write.dstBinding      = 0;
            write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo     = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }
    }
}
