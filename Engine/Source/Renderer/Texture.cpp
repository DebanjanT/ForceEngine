#include "Force/Renderer/Texture.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Renderer/VulkanBuffer.h"
#include "Force/Renderer/VulkanCommandBuffer.h"
#include "Force/Core/Logger.h"
#include <algorithm>

namespace Force
{
    // -------------------------------------------------------------------------
    // Texture2D
    // -------------------------------------------------------------------------
    Texture2D::~Texture2D()
    {
        Destroy();
    }

    void Texture2D::Init(const TextureDesc& desc, const SamplerDesc& samplerDesc)
    {
        m_Width     = desc.Width;
        m_Height    = desc.Height;
        m_MipLevels = desc.MipLevels;
        m_Format    = desc.Format;
        m_Aspect    = desc.Aspect;

        CreateImage(desc.Width, desc.Height, desc.MipLevels, desc.ArrayLayers,
                    desc.Format, desc.Usage, desc.Samples);
        CreateImageView(desc.Format, desc.Aspect, desc.MipLevels);
        CreateSampler(samplerDesc);
    }

    void Texture2D::Upload(const void* data, u64 dataSize)
    {
        // Staging buffer → GPU image
        VulkanBuffer staging;
        staging.Create(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        staging.SetData(data, dataSize);

        VkCommandBuffer cmd = VulkanCommandBuffer::BeginSingleTimeCommands();

        // Undefined → TransferDst
        {
            VkImageMemoryBarrier2 barrier{};
            barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image               = m_Image;
            barrier.subresourceRange    = {m_Aspect, 0, m_MipLevels, 0, 1};
            barrier.srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            barrier.srcAccessMask       = 0;
            barrier.dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;

            VkDependencyInfo dep{};
            dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dep.imageMemoryBarrierCount = 1;
            dep.pImageMemoryBarriers    = &barrier;
            vkCmdPipelineBarrier2(cmd, &dep);
        }

        // Copy buffer → image
        VkBufferImageCopy region{};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = m_Aspect;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {0, 0, 0};
        region.imageExtent                     = {m_Width, m_Height, 1};

        vkCmdCopyBufferToImage(cmd, staging.GetBuffer(), m_Image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // TransferDst → ShaderReadOnlyOptimal
        {
            VkImageMemoryBarrier2 barrier{};
            barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image               = m_Image;
            barrier.subresourceRange    = {m_Aspect, 0, m_MipLevels, 0, 1};
            barrier.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.dstStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            barrier.dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT;

            VkDependencyInfo dep{};
            dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dep.imageMemoryBarrierCount = 1;
            dep.pImageMemoryBarriers    = &barrier;
            vkCmdPipelineBarrier2(cmd, &dep);
        }

        VulkanCommandBuffer::EndSingleTimeCommands(cmd);
        m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        staging.Destroy();
    }

    void Texture2D::Destroy()
    {
        VkDevice      device    = VulkanDevice::Get().GetDevice();
        VmaAllocator  allocator = VulkanDevice::Get().GetAllocator();

        if (m_Sampler   != VK_NULL_HANDLE) { vkDestroySampler(device, m_Sampler, nullptr);    m_Sampler   = VK_NULL_HANDLE; }
        if (m_ImageView != VK_NULL_HANDLE) { vkDestroyImageView(device, m_ImageView, nullptr); m_ImageView = VK_NULL_HANDLE; }
        if (m_Image     != VK_NULL_HANDLE) { vmaDestroyImage(allocator, m_Image, m_Allocation); m_Image = VK_NULL_HANDLE; m_Allocation = VK_NULL_HANDLE; }
    }

    void Texture2D::TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout)
    {
        if (m_CurrentLayout == newLayout) return;

        VkImageMemoryBarrier2 barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.oldLayout           = m_CurrentLayout;
        barrier.newLayout           = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = m_Image;
        barrier.subresourceRange    = {m_Aspect, 0, m_MipLevels, 0, 1};

        // Source access based on current layout
        switch (m_CurrentLayout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcStageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            barrier.srcAccessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.srcStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.srcStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        default:
            barrier.srcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;
        }

        // Destination access based on new layout
        switch (newLayout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.dstStageMask  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            barrier.dstAccessMask = 0;
            break;
        default:
            barrier.dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;
        }

        VkDependencyInfo dep{};
        dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers    = &barrier;
        vkCmdPipelineBarrier2(cmd, &dep);

        m_CurrentLayout = newLayout;
    }

    VkDescriptorImageInfo Texture2D::GetDescriptorInfo() const
    {
        VkDescriptorImageInfo info{};
        info.sampler     = m_Sampler;
        info.imageView   = m_ImageView;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    }

    // -------------------------------------------------------------------------
    // Factory
    // -------------------------------------------------------------------------
    Ref<Texture2D> Texture2D::Create(const TextureDesc& desc, const SamplerDesc& samplerDesc)
    {
        auto tex = CreateRef<Texture2D>();
        tex->Init(desc, samplerDesc);
        return tex;
    }

    Ref<Texture2D> Texture2D::CreateFromData(u32 width, u32 height, VkFormat format,
                                              const void* data, u64 dataSize)
    {
        TextureDesc desc{};
        desc.Width  = width;
        desc.Height = height;
        desc.Format = format;
        desc.Usage  = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        auto tex = CreateRef<Texture2D>();
        tex->Init(desc);
        tex->Upload(data, dataSize);
        return tex;
    }

    Ref<Texture2D> Texture2D::CreateWhite()
    {
        const u8 data[4] = {255, 255, 255, 255};
        return CreateFromData(1, 1, VK_FORMAT_R8G8B8A8_UNORM, data, 4);
    }

    Ref<Texture2D> Texture2D::CreateBlack()
    {
        const u8 data[4] = {0, 0, 0, 255};
        return CreateFromData(1, 1, VK_FORMAT_R8G8B8A8_UNORM, data, 4);
    }

    Ref<Texture2D> Texture2D::CreateNormal()
    {
        // Flat normal map: tangent-space (0,0,1) → RGBA8 (128,128,255,255)
        const u8 data[4] = {128, 128, 255, 255};
        return CreateFromData(1, 1, VK_FORMAT_R8G8B8A8_UNORM, data, 4);
    }

    Ref<Texture2D> Texture2D::CreateColorAttachment(u32 width, u32 height, VkFormat format)
    {
        TextureDesc desc{};
        desc.Width  = width;
        desc.Height = height;
        desc.Format = format;
        desc.Usage  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        return Create(desc);
    }

    Ref<Texture2D> Texture2D::CreateDepthAttachment(u32 width, u32 height, VkFormat format)
    {
        TextureDesc desc{};
        desc.Width  = width;
        desc.Height = height;
        desc.Format = format;
        desc.Usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        desc.Aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

        return Create(desc);
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------
    void Texture2D::CreateImage(u32 width, u32 height, u32 mipLevels, u32 arrayLayers,
                                 VkFormat format, VkImageUsageFlags usage,
                                 VkSampleCountFlagBits samples)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent        = {width, height, 1};
        imageInfo.mipLevels     = mipLevels;
        imageInfo.arrayLayers   = arrayLayers;
        imageInfo.format        = format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = usage;
        imageInfo.samples       = samples;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkResult result = vmaCreateImage(VulkanDevice::Get().GetAllocator(),
                                          &imageInfo, &allocInfo,
                                          &m_Image, &m_Allocation, nullptr);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan image!");
        m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void Texture2D::CreateImageView(VkFormat format, VkImageAspectFlags aspect, u32 mipLevels)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = m_Image;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = format;
        viewInfo.subresourceRange.aspectMask     = aspect;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        VkResult result = vkCreateImageView(VulkanDevice::Get().GetDevice(),
                                             &viewInfo, nullptr, &m_ImageView);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create image view!");
    }

    void Texture2D::CreateSampler(const SamplerDesc& desc)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(VulkanDevice::Get().GetPhysicalDevice(), &props);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter               = desc.MagFilter;
        samplerInfo.minFilter               = desc.MinFilter;
        samplerInfo.addressModeU            = desc.AddressModeU;
        samplerInfo.addressModeV            = desc.AddressModeV;
        samplerInfo.addressModeW            = desc.AddressModeW;
        samplerInfo.anisotropyEnable        = desc.EnableAnisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy           = desc.EnableAnisotropy
                                                ? std::min(desc.MaxAnisotropy, props.limits.maxSamplerAnisotropy)
                                                : 1.0f;
        samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode              = desc.MipmapMode;
        samplerInfo.mipLodBias              = 0.0f;
        samplerInfo.minLod                  = desc.MinLod;
        samplerInfo.maxLod                  = desc.MaxLod;

        VkResult result = vkCreateSampler(VulkanDevice::Get().GetDevice(),
                                           &samplerInfo, nullptr, &m_Sampler);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create sampler!");
    }

    // -------------------------------------------------------------------------
    // TextureLibrary
    // -------------------------------------------------------------------------
    TextureLibrary& TextureLibrary::Get()
    {
        static TextureLibrary instance;
        return instance;
    }

    void TextureLibrary::InitDefaults()
    {
        m_WhiteTexture  = Texture2D::CreateWhite();
        m_BlackTexture  = Texture2D::CreateBlack();
        m_NormalTexture = Texture2D::CreateNormal();
    }

    void TextureLibrary::Add(const std::string& name, const Ref<Texture2D>& texture)
    {
        m_Textures[name] = texture;
    }

    Ref<Texture2D> TextureLibrary::Load(const std::filesystem::path& path)
    {
        // TODO: Phase 4 — stb_image loading
        FORCE_CORE_WARN("Texture file loading not yet implemented: {}", path.string());
        return m_WhiteTexture;
    }

    Ref<Texture2D> TextureLibrary::Get(const std::string& name)
    {
        FORCE_CORE_ASSERT(Exists(name), "Texture not found: {}", name);
        return m_Textures.at(name);
    }

    bool TextureLibrary::Exists(const std::string& name) const
    {
        return m_Textures.find(name) != m_Textures.end();
    }

    void TextureLibrary::Clear()
    {
        m_Textures.clear();
    }
}
