#include "Force/Renderer/RenderGraph.h"
#include "Force/Renderer/Renderer.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"

namespace Force
{
    // -------------------------------------------------------------------------
    // PassBuilder
    // -------------------------------------------------------------------------
    PassBuilder::PassBuilder(RenderGraph& graph, u32 passIndex)
        : m_Graph(graph), m_PassIndex(passIndex)
    {
    }

    PassBuilder& PassBuilder::AddColorOutput(const std::string& name, const RGTextureDesc& desc)
    {
        RGHandle handle = m_Graph.GetOrCreateHandle(name, desc);
        m_Graph.m_Resources[handle].IsWritten = true;
        m_Graph.m_Passes[m_PassIndex].ColorOutputs.push_back(handle);
        return *this;
    }

    PassBuilder& PassBuilder::AddColorOutput(RGHandle handle)
    {
        FORCE_CORE_ASSERT(handle < m_Graph.m_Resources.size(), "Invalid RGHandle");
        m_Graph.m_Resources[handle].IsWritten = true;
        m_Graph.m_Passes[m_PassIndex].ColorOutputs.push_back(handle);
        return *this;
    }

    PassBuilder& PassBuilder::SetDepthOutput(const std::string& name, const RGTextureDesc& desc)
    {
        RGHandle handle = m_Graph.GetOrCreateHandle(name, desc);
        m_Graph.m_Resources[handle].IsWritten = true;
        m_Graph.m_Passes[m_PassIndex].DepthOutput = handle;
        return *this;
    }

    PassBuilder& PassBuilder::SetDepthOutput(RGHandle handle)
    {
        FORCE_CORE_ASSERT(handle < m_Graph.m_Resources.size(), "Invalid RGHandle");
        m_Graph.m_Resources[handle].IsWritten = true;
        m_Graph.m_Passes[m_PassIndex].DepthOutput = handle;
        return *this;
    }

    PassBuilder& PassBuilder::AddTextureInput(RGHandle handle)
    {
        FORCE_CORE_ASSERT(handle < m_Graph.m_Resources.size(), "Invalid RGHandle");
        m_Graph.m_Passes[m_PassIndex].TextureInputs.push_back(handle);
        return *this;
    }

    void PassBuilder::Execute(RGPassExecuteFn fn)
    {
        m_Graph.m_Passes[m_PassIndex].ExecuteFn = std::move(fn);
    }

    // -------------------------------------------------------------------------
    // RenderGraph
    // -------------------------------------------------------------------------
    RGHandle RenderGraph::DeclareTexture(const std::string& name, const RGTextureDesc& desc)
    {
        auto it = m_NameToHandle.find(name);
        if (it != m_NameToHandle.end())
        {
            return it->second;
        }

        RGHandle handle = static_cast<RGHandle>(m_Resources.size());
        RGResource resource;
        resource.Name       = name;
        resource.Desc       = desc;
        resource.IsImported = false;
        m_Resources.push_back(resource);
        m_NameToHandle[name] = handle;
        return handle;
    }

    RGHandle RenderGraph::ImportTexture(const std::string& name, const Ref<Texture2D>& texture)
    {
        auto it = m_NameToHandle.find(name);
        if (it != m_NameToHandle.end())
        {
            m_Resources[it->second].Texture = texture;
            m_Resources[it->second].CurrentLayout = texture->GetCurrentLayout();
            return it->second;
        }

        RGHandle handle = static_cast<RGHandle>(m_Resources.size());
        RGResource resource;
        resource.Name          = name;
        resource.Texture       = texture;
        resource.CurrentLayout = texture->GetCurrentLayout();
        resource.IsImported    = true;
        m_Resources.push_back(resource);
        m_NameToHandle[name] = handle;
        return handle;
    }

    PassBuilder RenderGraph::AddPass(const std::string& name)
    {
        u32 passIndex = static_cast<u32>(m_Passes.size());
        RGPass pass;
        pass.Name = name;
        m_Passes.push_back(pass);
        return PassBuilder(*this, passIndex);
    }

    RGHandle RenderGraph::GetOrCreateHandle(const std::string& name, const RGTextureDesc& desc)
    {
        auto it = m_NameToHandle.find(name);
        if (it != m_NameToHandle.end())
        {
            return it->second;
        }
        return DeclareTexture(name, desc);
    }

    void RenderGraph::Compile(u32 framebufferWidth, u32 framebufferHeight)
    {
        m_FramebufferWidth  = framebufferWidth;
        m_FramebufferHeight = framebufferHeight;

        CreateResources();

        // Simple linear execution order (no dependency sorting yet)
        m_ExecutionOrder.clear();
        for (u32 i = 0; i < static_cast<u32>(m_Passes.size()); ++i)
        {
            m_ExecutionOrder.push_back(i);
        }

        m_Compiled = true;
    }

    void RenderGraph::CreateResources()
    {
        for (auto& res : m_Resources)
        {
            if (res.IsImported)
                continue;

            if (res.Texture)
            {
                // Check if resize is needed
                if (res.Texture->GetWidth() == (res.Desc.Width == 0 ? m_FramebufferWidth : res.Desc.Width) &&
                    res.Texture->GetHeight() == (res.Desc.Height == 0 ? m_FramebufferHeight : res.Desc.Height))
                {
                    continue; // already correct size
                }
                res.Texture->Destroy();
                res.Texture.reset();
            }

            u32 w = res.Desc.Width  == 0 ? m_FramebufferWidth  : res.Desc.Width;
            u32 h = res.Desc.Height == 0 ? m_FramebufferHeight : res.Desc.Height;

            // Determine if depth format
            bool isDepth = (res.Desc.Format == VK_FORMAT_D32_SFLOAT ||
                            res.Desc.Format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                            res.Desc.Format == VK_FORMAT_D24_UNORM_S8_UINT ||
                            res.Desc.Format == VK_FORMAT_D16_UNORM ||
                            res.Desc.Format == VK_FORMAT_D16_UNORM_S8_UINT);

            if (isDepth)
            {
                res.Texture = Texture2D::CreateDepthAttachment(w, h, res.Desc.Format);
            }
            else
            {
                res.Texture = Texture2D::CreateColorAttachment(w, h, res.Desc.Format);
            }

            res.CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }

    void RenderGraph::Execute(VkCommandBuffer cmd)
    {
        FORCE_CORE_ASSERT(m_Compiled, "RenderGraph must be compiled before execution");

        for (u32 passIdx : m_ExecutionOrder)
        {
            RGPass& pass = m_Passes[passIdx];

            // Insert barriers to transition resources to required layouts
            InsertBarriers(cmd, pass);

            // Build dynamic rendering info
            std::vector<VkRenderingAttachmentInfo> colorAttachments;
            colorAttachments.reserve(pass.ColorOutputs.size());

            for (RGHandle handle : pass.ColorOutputs)
            {
                RGResource& res = m_Resources[handle];
                VkRenderingAttachmentInfo attachment{};
                attachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                attachment.imageView   = res.Texture->GetImageView();
                attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
                attachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
                colorAttachments.push_back(attachment);
            }

            VkRenderingAttachmentInfo depthAttachment{};
            bool hasDepth = pass.DepthOutput != RG_INVALID_HANDLE;
            if (hasDepth)
            {
                RGResource& res = m_Resources[pass.DepthOutput];
                depthAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                depthAttachment.imageView   = res.Texture->GetImageView();
                depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
                depthAttachment.clearValue.depthStencil = {1.0f, 0};
            }

            VkRenderingInfo renderingInfo{};
            renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
            renderingInfo.renderArea.offset    = {0, 0};
            renderingInfo.renderArea.extent    = {m_FramebufferWidth, m_FramebufferHeight};
            renderingInfo.layerCount           = 1;
            renderingInfo.colorAttachmentCount = static_cast<u32>(colorAttachments.size());
            renderingInfo.pColorAttachments    = colorAttachments.empty() ? nullptr : colorAttachments.data();
            renderingInfo.pDepthAttachment     = hasDepth ? &depthAttachment : nullptr;
            renderingInfo.pStencilAttachment   = nullptr;

            vkCmdBeginRendering(cmd, &renderingInfo);

            // Set viewport and scissor
            VkViewport viewport{};
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = static_cast<float>(m_FramebufferWidth);
            viewport.height   = static_cast<float>(m_FramebufferHeight);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = {m_FramebufferWidth, m_FramebufferHeight};
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            // Execute pass callback
            if (pass.ExecuteFn)
            {
                pass.ExecuteFn(cmd);
            }

            vkCmdEndRendering(cmd);

            // Update current layouts for written resources
            for (RGHandle handle : pass.ColorOutputs)
            {
                m_Resources[handle].CurrentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
            if (hasDepth)
            {
                m_Resources[pass.DepthOutput].CurrentLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
        }
    }

    void RenderGraph::InsertBarriers(VkCommandBuffer cmd, const RGPass& pass)
    {
        std::vector<VkImageMemoryBarrier2> barriers;

        // Transition texture inputs to SHADER_READ_ONLY_OPTIMAL
        for (RGHandle handle : pass.TextureInputs)
        {
            RGResource& res = m_Resources[handle];
            if (res.CurrentLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                VkImageMemoryBarrier2 barrier{};
                barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                barrier.srcStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | 
                                              VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                barrier.srcAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                                              VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                barrier.dstStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                barrier.dstAccessMask       = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
                barrier.oldLayout           = res.CurrentLayout;
                barrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image               = res.Texture->GetImage();
                VkFormat fmt = res.Texture->GetFormat();
                bool isDepth = (fmt == VK_FORMAT_D32_SFLOAT ||
                                fmt == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                                fmt == VK_FORMAT_D24_UNORM_S8_UINT ||
                                fmt == VK_FORMAT_D16_UNORM);
                barrier.subresourceRange.aspectMask     = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel   = 0;
                barrier.subresourceRange.levelCount     = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount     = 1;
                barriers.push_back(barrier);
                res.CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        // Transition color outputs to COLOR_ATTACHMENT_OPTIMAL
        for (RGHandle handle : pass.ColorOutputs)
        {
            RGResource& res = m_Resources[handle];
            if (res.CurrentLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                VkImageMemoryBarrier2 barrier{};
                barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                barrier.srcStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                                              VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                barrier.srcAccessMask       = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
                                              VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
                barrier.dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                barrier.dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                barrier.oldLayout           = res.CurrentLayout;
                barrier.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image               = res.Texture->GetImage();
                barrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                barriers.push_back(barrier);
                // Don't update layout here; it will be updated after pass execution
            }
        }

        // Transition depth output to DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        if (pass.DepthOutput != RG_INVALID_HANDLE)
        {
            RGResource& res = m_Resources[pass.DepthOutput];
            if (res.CurrentLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                VkImageMemoryBarrier2 barrier{};
                barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                barrier.srcStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                                              VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
                barrier.srcAccessMask       = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
                                              VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                barrier.dstStageMask        = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                              VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                barrier.dstAccessMask       = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                              VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                barrier.oldLayout           = res.CurrentLayout;
                barrier.newLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image               = res.Texture->GetImage();
                barrier.subresourceRange    = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
                barriers.push_back(barrier);
            }
        }

        if (!barriers.empty())
        {
            VkDependencyInfo depInfo{};
            depInfo.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depInfo.imageMemoryBarrierCount  = static_cast<u32>(barriers.size());
            depInfo.pImageMemoryBarriers     = barriers.data();
            vkCmdPipelineBarrier2(cmd, &depInfo);
        }
    }

    void RenderGraph::OnResize(u32 width, u32 height)
    {
        if (m_FramebufferWidth == width && m_FramebufferHeight == height)
            return;

        m_FramebufferWidth  = width;
        m_FramebufferHeight = height;

        // Recreate framebuffer-sized resources
        CreateResources();
    }

    Ref<Texture2D> RenderGraph::GetTexture(RGHandle handle) const
    {
        FORCE_CORE_ASSERT(handle < m_Resources.size(), "Invalid RGHandle");
        return m_Resources[handle].Texture;
    }

    Ref<Texture2D> RenderGraph::GetTexture(const std::string& name) const
    {
        auto it = m_NameToHandle.find(name);
        FORCE_CORE_ASSERT(it != m_NameToHandle.end(), "Texture not found in RenderGraph");
        return m_Resources[it->second].Texture;
    }

    void RenderGraph::Reset()
    {
        m_Passes.clear();
        m_ExecutionOrder.clear();
        m_Compiled = false;

        // Reset resource layouts to undefined (they'll be transitioned fresh next frame)
        for (auto& res : m_Resources)
        {
            res.CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            res.IsWritten = false;
        }
    }

    void RenderGraph::Destroy()
    {
        for (auto& res : m_Resources)
        {
            if (!res.IsImported && res.Texture)
            {
                res.Texture->Destroy();
                res.Texture.reset();
            }
        }
        m_Resources.clear();
        m_NameToHandle.clear();
        m_Passes.clear();
        m_ExecutionOrder.clear();
        m_Compiled = false;
    }
}
