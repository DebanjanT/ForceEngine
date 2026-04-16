#pragma once

#include "Force/Core/Base.h"
#include <vulkan/vulkan.h>
#include <filesystem>
#include <unordered_map>

namespace Force
{
    enum class ShaderStage : u32
    {
        Vertex   = VK_SHADER_STAGE_VERTEX_BIT,
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
        Compute  = VK_SHADER_STAGE_COMPUTE_BIT,
        Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
        TessControl = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
        TessEval = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
    };

    enum class ShaderDataType : u32
    {
        None = 0,
        Float, Float2, Float3, Float4,
        Int, Int2, Int3, Int4,
        UInt, UInt2, UInt3, UInt4,
        Bool,
        Mat3, Mat4,
        Sampler2D, SamplerCube
    };

    struct ShaderUniform
    {
        std::string Name;
        ShaderDataType Type = ShaderDataType::None;
        u32 Size = 0;
        u32 Offset = 0;
        u32 Binding = 0;
        u32 Set = 0;
    };

    struct ShaderResource
    {
        std::string Name;
        ShaderDataType Type = ShaderDataType::None;
        u32 Binding = 0;
        u32 Set = 0;
        u32 ArraySize = 1;
    };

    struct ShaderPushConstant
    {
        std::string Name;
        u32 Size = 0;
        u32 Offset = 0;
        ShaderStage Stage = ShaderStage::Vertex;
    };

    struct ShaderDescriptorSet
    {
        u32 Set = 0;
        std::vector<ShaderUniform> Uniforms;
        std::vector<ShaderResource> Resources;
    };

    struct ShaderReflectionData
    {
        std::vector<ShaderDescriptorSet> DescriptorSets;
        std::vector<ShaderPushConstant> PushConstants;
        std::unordered_map<std::string, u32> UniformLocations;
    };

    // Used for pipelines created for dynamic rendering (Vulkan 1.3, no VkRenderPass needed)
    struct PipelineRenderingInfo
    {
        std::vector<VkFormat> ColorAttachmentFormats;
        VkFormat DepthAttachmentFormat   = VK_FORMAT_UNDEFINED;
        VkFormat StencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    };

    class Shader
    {
    public:
        Shader(const std::string& name);
        ~Shader();

        // Load for traditional render pass (e.g. swapchain)
        void Load(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);

        // Load for dynamic rendering (Vulkan 1.3, used by RenderGraph)
        void Load(const std::filesystem::path& vertexPath,
                  const std::filesystem::path& fragmentPath,
                  const PipelineRenderingInfo& renderingInfo);

        void LoadCompute(const std::filesystem::path& computePath);
        void Destroy();

        void Bind(VkCommandBuffer cmd) const;

        const std::string& GetName() const { return m_Name; }
        VkPipeline GetPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

        const ShaderReflectionData& GetReflectionData() const { return m_ReflectionData; }
        const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }

        // Factory methods
        static Ref<Shader> Create(const std::string& name);
        static Ref<Shader> Create(const std::string& name,
                                   const std::filesystem::path& vertexPath,
                                   const std::filesystem::path& fragmentPath);
        static Ref<Shader> Create(const std::string& name,
                                   const std::filesystem::path& vertexPath,
                                   const std::filesystem::path& fragmentPath,
                                   const PipelineRenderingInfo& renderingInfo);

    private:
        std::vector<u32> LoadSPIRV(const std::filesystem::path& path);
        VkShaderModule CreateShaderModule(const std::vector<u32>& code);

        void Reflect(const std::vector<u32>& vertexCode, const std::vector<u32>& fragmentCode);
        void ReflectStage(const std::vector<u32>& code, ShaderStage stage);

        void CreateDescriptorSetLayouts();
        void CreatePipelineLayout();

        // renderPass != VK_NULL_HANDLE  → traditional render pass
        // renderPass == VK_NULL_HANDLE  → use pRenderingInfo for dynamic rendering
        void CreateGraphicsPipeline(VkShaderModule vertModule, VkShaderModule fragModule,
                                     VkRenderPass renderPass,
                                     const PipelineRenderingInfo* renderingInfo = nullptr);
        void CreateComputePipeline(VkShaderModule compModule);

    private:
        std::string m_Name;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

        ShaderReflectionData m_ReflectionData;
        bool m_IsCompute = false;
    };

    class ShaderLibrary
    {
    public:
        static ShaderLibrary& Get();

        void Add(const Ref<Shader>& shader);
        void Add(const std::string& name, const Ref<Shader>& shader);
        Ref<Shader> Load(const std::string& name,
                         const std::filesystem::path& vertexPath,
                         const std::filesystem::path& fragmentPath);
        Ref<Shader> Load(const std::string& name,
                         const std::filesystem::path& vertexPath,
                         const std::filesystem::path& fragmentPath,
                         const PipelineRenderingInfo& renderingInfo);
        Ref<Shader> Get(const std::string& name);
        bool Exists(const std::string& name) const;

        void Clear();

    private:
        ShaderLibrary() = default;
        std::unordered_map<std::string, Ref<Shader>> m_Shaders;
    };
}
