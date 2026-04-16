#include "Force/Renderer/Renderer.h"
#include "Force/Core/Application.h"
#include "Force/Core/Logger.h"

namespace Force
{
    Scope<VulkanContext> Renderer::s_Context = nullptr;
    Scope<VulkanDevice> Renderer::s_Device = nullptr;
    Scope<VulkanSwapchain> Renderer::s_Swapchain = nullptr;
    
    std::vector<VkCommandBuffer> Renderer::s_CommandBuffers;
    std::vector<VkSemaphore> Renderer::s_ImageAvailableSemaphores;
    std::vector<VkSemaphore> Renderer::s_RenderFinishedSemaphores;
    std::vector<VkFence> Renderer::s_InFlightFences;
    
    u32 Renderer::s_CurrentFrame = 0;
    u32 Renderer::s_MaxFramesInFlight = 2;
    bool Renderer::s_FramebufferResized = false;
    
    glm::vec4 Renderer::s_ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    
    void Renderer::Init(const RendererConfig& config)
    {
        FORCE_CORE_INFO("Initializing Renderer...");
        
        s_MaxFramesInFlight = config.MaxFramesInFlight;
        
        s_Context = CreateScope<VulkanContext>();
        s_Context->Init(config);
        
        // Create surface
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
        
        FORCE_CORE_INFO("Renderer initialized successfully");
    }
    
    void Renderer::Shutdown()
    {
        FORCE_CORE_INFO("Shutting down Renderer...");
        
        s_Device->WaitIdle();
        
        DestroySyncObjects();
        
        s_Swapchain->Shutdown();
        s_Swapchain.reset();
        
        s_Device->Shutdown();
        s_Device.reset();
        
        s_Context->Shutdown();
        s_Context.reset();
    }
    
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
        
        // Reset and begin command buffer
        VkCommandBuffer cmd = s_CommandBuffers[s_CurrentFrame];
        vkResetCommandBuffer(cmd, 0);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        // Begin render pass with clear
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = s_Swapchain->GetRenderPass();
        renderPassInfo.framebuffer = s_Swapchain->GetFramebuffer(s_Swapchain->GetCurrentImageIndex());
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = s_Swapchain->GetExtent();
        
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{s_ClearColor.r, s_ClearColor.g, s_ClearColor.b, s_ClearColor.a}};
        clearValues[1].depthStencil = {1.0f, 0};
        
        renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        // Set viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<f32>(s_Swapchain->GetExtent().width);
        viewport.height = static_cast<f32>(s_Swapchain->GetExtent().height);
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
        
        // End render pass and command buffer
        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);
        
        // Submit command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        VkSemaphore waitSemaphores[] = {s_ImageAvailableSemaphores[s_CurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        
        VkSemaphore signalSemaphores[] = {s_RenderFinishedSemaphores[s_CurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        VkResult result = vkQueueSubmit(s_Device->GetGraphicsQueue(), 1, &submitInfo, s_InFlightFences[s_CurrentFrame]);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to submit draw command buffer!");
        
        // Present
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
        if (width == 0 || height == 0)
        {
            return;
        }
        
        s_Device->WaitIdle();
        s_Swapchain->Recreate(width, height);
    }
    
    void Renderer::SetClearColor(const glm::vec4& color)
    {
        s_ClearColor = color;
    }
    
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
            VkResult result1 = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &s_ImageAvailableSemaphores[i]);
            VkResult result2 = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &s_RenderFinishedSemaphores[i]);
            VkResult result3 = vkCreateFence(device, &fenceInfo, nullptr, &s_InFlightFences[i]);
            
            FORCE_CORE_ASSERT(result1 == VK_SUCCESS && result2 == VK_SUCCESS && result3 == VK_SUCCESS,
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
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = s_Device->GetCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<u32>(s_CommandBuffers.size());
        
        VkResult result = vkAllocateCommandBuffers(s_Device->GetDevice(), &allocInfo, s_CommandBuffers.data());
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffers!");
    }
}
