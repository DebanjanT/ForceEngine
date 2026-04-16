#pragma once

#include "Force/Core/Base.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <filesystem>
#include <unordered_map>

namespace Force
{
    struct TextureDesc
    {
        u32 Width      = 1;
        u32 Height     = 1;
        u32 MipLevels  = 1;
        u32 ArrayLayers = 1;
        VkFormat             Format  = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags    Usage   = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VkImageAspectFlags   Aspect  = VK_IMAGE_ASPECT_COLOR_BIT;
        VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    };

    struct SamplerDesc
    {
        VkFilter              MinFilter   = VK_FILTER_LINEAR;
        VkFilter              MagFilter   = VK_FILTER_LINEAR;
        VkSamplerAddressMode  AddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode  AddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode  AddressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerMipmapMode   MipmapMode  = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        bool  EnableAnisotropy = true;
        float MaxAnisotropy    = 16.0f;
        float MinLod           = 0.0f;
        float MaxLod           = VK_LOD_CLAMP_NONE;
    };

    class Texture2D
    {
    public:
        Texture2D() = default;
        ~Texture2D();

        void Init(const TextureDesc& desc, const SamplerDesc& samplerDesc = {});
        void Upload(const void* data, u64 dataSize); // upload pixel data immediately
        void Destroy();

        // Insert a pipeline barrier to transition this image to a new layout
        void TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout);

        VkImage       GetImage()       const { return m_Image; }
        VkImageView   GetImageView()   const { return m_ImageView; }
        VkSampler     GetSampler()     const { return m_Sampler; }
        VkFormat      GetFormat()      const { return m_Format; }
        VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
        VkDescriptorImageInfo GetDescriptorInfo() const;

        u32 GetWidth()  const { return m_Width; }
        u32 GetHeight() const { return m_Height; }

        // Factory methods
        static Ref<Texture2D> Create(const TextureDesc& desc, const SamplerDesc& samplerDesc = {});
        static Ref<Texture2D> CreateFromData(u32 width, u32 height, VkFormat format,
                                              const void* data, u64 dataSize);
        // 1×1 solid colour helpers — always safe to bind
        static Ref<Texture2D> CreateWhite();
        static Ref<Texture2D> CreateBlack();
        static Ref<Texture2D> CreateNormal(); // flat normal (128,128,255,255)

        // Render-target helpers
        static Ref<Texture2D> CreateColorAttachment(u32 width, u32 height, VkFormat format);
        static Ref<Texture2D> CreateDepthAttachment(u32 width, u32 height, VkFormat format);

    private:
        void CreateImage(u32 width, u32 height, u32 mipLevels, u32 arrayLayers,
                         VkFormat format, VkImageUsageFlags usage,
                         VkSampleCountFlagBits samples);
        void CreateImageView(VkFormat format, VkImageAspectFlags aspect, u32 mipLevels);
        void CreateSampler(const SamplerDesc& desc);

    private:
        VkImage       m_Image      = VK_NULL_HANDLE;
        VkImageView   m_ImageView  = VK_NULL_HANDLE;
        VkSampler     m_Sampler    = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;

        u32            m_Width      = 0;
        u32            m_Height     = 0;
        u32            m_MipLevels  = 1;
        VkFormat           m_Format     = VK_FORMAT_UNDEFINED;
        VkImageAspectFlags m_Aspect     = VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageLayout      m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    // -------------------------------------------------------------------------
    class TextureLibrary
    {
    public:
        static TextureLibrary& Get();

        void         Add(const std::string& name, const Ref<Texture2D>& texture);
        Ref<Texture2D> Load(const std::filesystem::path& path); // stb_image — Phase 4
        Ref<Texture2D> Get(const std::string& name);
        bool         Exists(const std::string& name) const;
        void         Clear();

        // Default textures — always valid after InitDefaults()
        Ref<Texture2D> GetWhite()  const { return m_WhiteTexture;  }
        Ref<Texture2D> GetBlack()  const { return m_BlackTexture;  }
        Ref<Texture2D> GetNormal() const { return m_NormalTexture; }

        void InitDefaults(); // called by Renderer::Init

    private:
        TextureLibrary() = default;
        std::unordered_map<std::string, Ref<Texture2D>> m_Textures;
        Ref<Texture2D> m_WhiteTexture;
        Ref<Texture2D> m_BlackTexture;
        Ref<Texture2D> m_NormalTexture;
    };
}
