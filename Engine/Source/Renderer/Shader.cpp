#include "Force/Renderer/Shader.h"
#include "Force/Renderer/Mesh.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Renderer/VulkanSwapchain.h"
#include "Force/Renderer/Renderer.h"
#include "Force/Core/Logger.h"
#include <glm/glm.hpp>
#include <fstream>

namespace Force
{
    // -------------------------------------------------------------------------
    // Shader
    // -------------------------------------------------------------------------
    Shader::Shader(const std::string& name)
        : m_Name(name)
    {
    }

    Shader::~Shader()
    {
        Destroy();
    }

    void Shader::Load(const std::filesystem::path& vertexPath,
                      const std::filesystem::path& fragmentPath)
    {
        m_IsCompute = false;

        std::vector<u32> vertexCode   = LoadSPIRV(vertexPath);
        std::vector<u32> fragmentCode = LoadSPIRV(fragmentPath);

        if (vertexCode.empty() || fragmentCode.empty())
        {
            FORCE_CORE_ERROR("Failed to load shader: {}", m_Name);
            return;
        }

        Reflect(vertexCode, fragmentCode);

        VkShaderModule vertModule = CreateShaderModule(vertexCode);
        VkShaderModule fragModule = CreateShaderModule(fragmentCode);

        CreateDescriptorSetLayouts();
        CreatePipelineLayout();
        CreateGraphicsPipeline(vertModule, fragModule,
                               Renderer::GetSwapchain().GetRenderPass(), nullptr);

        VkDevice device = VulkanDevice::Get().GetDevice();
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);

        FORCE_CORE_INFO("Shader '{}' loaded successfully", m_Name);
    }

    void Shader::Load(const std::filesystem::path& vertexPath,
                      const std::filesystem::path& fragmentPath,
                      const PipelineRenderingInfo& renderingInfo)
    {
        m_IsCompute = false;

        std::vector<u32> vertexCode   = LoadSPIRV(vertexPath);
        std::vector<u32> fragmentCode = LoadSPIRV(fragmentPath);

        if (vertexCode.empty() || fragmentCode.empty())
        {
            FORCE_CORE_ERROR("Failed to load shader (dynamic rendering): {}", m_Name);
            return;
        }

        Reflect(vertexCode, fragmentCode);

        VkShaderModule vertModule = CreateShaderModule(vertexCode);
        VkShaderModule fragModule = CreateShaderModule(fragmentCode);

        CreateDescriptorSetLayouts();
        CreatePipelineLayout();
        CreateGraphicsPipeline(vertModule, fragModule, VK_NULL_HANDLE, &renderingInfo);

        VkDevice device = VulkanDevice::Get().GetDevice();
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);

        FORCE_CORE_INFO("Shader '{}' loaded (dynamic rendering)", m_Name);
    }

    void Shader::LoadCompute(const std::filesystem::path& computePath)
    {
        m_IsCompute = true;

        std::vector<u32> computeCode = LoadSPIRV(computePath);

        if (computeCode.empty())
        {
            FORCE_CORE_ERROR("Failed to load compute shader: {}", m_Name);
            return;
        }

        ReflectStage(computeCode, ShaderStage::Compute);

        VkShaderModule compModule = CreateShaderModule(computeCode);

        CreateDescriptorSetLayouts();
        CreatePipelineLayout();
        CreateComputePipeline(compModule);

        VkDevice device = VulkanDevice::Get().GetDevice();
        vkDestroyShaderModule(device, compModule, nullptr);

        FORCE_CORE_INFO("Compute shader '{}' loaded successfully", m_Name);
    }

    void Shader::Destroy()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        if (m_Pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, m_Pipeline, nullptr);
            m_Pipeline = VK_NULL_HANDLE;
        }

        if (m_PipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }

        for (auto layout : m_DescriptorSetLayouts)
        {
            vkDestroyDescriptorSetLayout(device, layout, nullptr);
        }
        m_DescriptorSetLayouts.clear();
    }

    void Shader::Bind(VkCommandBuffer cmd) const
    {
        VkPipelineBindPoint bindPoint = m_IsCompute
            ? VK_PIPELINE_BIND_POINT_COMPUTE
            : VK_PIPELINE_BIND_POINT_GRAPHICS;
        vkCmdBindPipeline(cmd, bindPoint, m_Pipeline);
    }

    // -------------------------------------------------------------------------
    // Factory
    // -------------------------------------------------------------------------
    Ref<Shader> Shader::Create(const std::string& name)
    {
        return CreateRef<Shader>(name);
    }

    Ref<Shader> Shader::Create(const std::string& name,
                                const std::filesystem::path& vertexPath,
                                const std::filesystem::path& fragmentPath)
    {
        auto shader = CreateRef<Shader>(name);
        shader->Load(vertexPath, fragmentPath);
        return shader;
    }

    Ref<Shader> Shader::Create(const std::string& name,
                                const std::filesystem::path& vertexPath,
                                const std::filesystem::path& fragmentPath,
                                const PipelineRenderingInfo& renderingInfo)
    {
        auto shader = CreateRef<Shader>(name);
        shader->Load(vertexPath, fragmentPath, renderingInfo);
        return shader;
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------
    std::vector<u32> Shader::LoadSPIRV(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            FORCE_CORE_ERROR("Failed to open shader file: {}", path.string());
            return {};
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<u32> buffer(fileSize / sizeof(u32));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();

        return buffer;
    }

    VkShaderModule Shader::CreateShaderModule(const std::vector<u32>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size() * sizeof(u32);
        createInfo.pCode    = code.data();

        VkShaderModule shaderModule;
        VkResult result = vkCreateShaderModule(VulkanDevice::Get().GetDevice(),
                                                &createInfo, nullptr, &shaderModule);

        if (result != VK_SUCCESS)
        {
            FORCE_CORE_ERROR("Failed to create shader module");
            return VK_NULL_HANDLE;
        }

        return shaderModule;
    }

    void Shader::Reflect(const std::vector<u32>& vertexCode,
                          const std::vector<u32>& fragmentCode)
    {
        // TODO: Integrate SPIRV-Reflect for full automatic reflection.
        // For now the descriptor layouts are built conventionally in
        // CreateDescriptorSetLayouts() to match the standard shader layout.
        m_ReflectionData.DescriptorSets.clear();
        m_ReflectionData.PushConstants.clear();
    }

    void Shader::ReflectStage(const std::vector<u32>& code, ShaderStage stage)
    {
        // TODO: single-stage reflection
    }

    void Shader::CreateDescriptorSetLayouts()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        // Set 0 — per-frame data: camera UBO (binding 0)
        {
            VkDescriptorSetLayoutBinding cameraBinding{};
            cameraBinding.binding         = 0;
            cameraBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            cameraBinding.descriptorCount = 1;
            cameraBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings    = &cameraBinding;

            VkDescriptorSetLayout layout;
            vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout);
            m_DescriptorSetLayouts.push_back(layout);
        }

        // Set 1 — per-material data:
        //   binding 0 : MaterialUBO
        //   binding 1 : albedo texture
        //   binding 2 : normal texture
        //   binding 3 : metallic-roughness texture
        //   binding 4 : AO texture
        //   binding 5 : emissive texture
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;

            VkDescriptorSetLayoutBinding materialUBO{};
            materialUBO.binding         = 0;
            materialUBO.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            materialUBO.descriptorCount = 1;
            materialUBO.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings.push_back(materialUBO);

            for (u32 i = 1; i <= 5; i++)
            {
                VkDescriptorSetLayoutBinding texBinding{};
                texBinding.binding         = i;
                texBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                texBinding.descriptorCount = 1;
                texBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
                bindings.push_back(texBinding);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<u32>(bindings.size());
            layoutInfo.pBindings    = bindings.data();

            VkDescriptorSetLayout layout;
            vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout);
            m_DescriptorSetLayouts.push_back(layout);
        }
    }

    void Shader::CreatePipelineLayout()
    {
        // Push constant: model matrix (vertex stage)
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstant.offset     = 0;
        pushConstant.size       = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount         = static_cast<u32>(m_DescriptorSetLayouts.size());
        layoutInfo.pSetLayouts            = m_DescriptorSetLayouts.data();
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges    = &pushConstant;

        VkResult result = vkCreatePipelineLayout(VulkanDevice::Get().GetDevice(),
                                                  &layoutInfo, nullptr, &m_PipelineLayout);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create pipeline layout");
    }

    void Shader::CreateGraphicsPipeline(VkShaderModule vertModule, VkShaderModule fragModule,
                                         VkRenderPass renderPass,
                                         const PipelineRenderingInfo* renderingInfo)
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        // Shader stages
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName  = "main";

        stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName  = "main";

        // Vertex input — standard Vertex layout (pos, normal, uv, tangent)
        auto bindingDesc    = Vertex::GetBindingDescription();
        auto attributeDescs = Vertex::GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount   = 1;
        vertexInputInfo.pVertexBindingDescriptions      = &bindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescs.size());
        vertexInputInfo.pVertexAttributeDescriptions    = attributeDescs.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport / scissor (dynamic)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount  = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode      = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth        = 1.0f;
        rasterizer.cullMode         = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace        = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable  = VK_FALSE;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Depth stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable  = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS;

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable    = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable   = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments    = &colorBlendAttachment;

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicState.pDynamicStates    = dynamicStates.data();

        // Pipeline create info
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = 2;
        pipelineInfo.pStages             = stages;
        pipelineInfo.pVertexInputState   = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pDepthStencilState  = &depthStencil;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = &dynamicState;
        pipelineInfo.layout              = m_PipelineLayout;
        pipelineInfo.subpass             = 0;

        // Render pass OR dynamic rendering
        VkPipelineRenderingCreateInfo dynRenderingInfo{};
        if (renderPass != VK_NULL_HANDLE)
        {
            pipelineInfo.renderPass = renderPass;
        }
        else
        {
            FORCE_CORE_ASSERT(renderingInfo != nullptr,
                "Dynamic rendering requires a PipelineRenderingInfo");

            dynRenderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
            dynRenderingInfo.colorAttachmentCount    = static_cast<u32>(renderingInfo->ColorAttachmentFormats.size());
            dynRenderingInfo.pColorAttachmentFormats = renderingInfo->ColorAttachmentFormats.data();
            dynRenderingInfo.depthAttachmentFormat   = renderingInfo->DepthAttachmentFormat;
            dynRenderingInfo.stencilAttachmentFormat = renderingInfo->StencilAttachmentFormat;

            pipelineInfo.renderPass = VK_NULL_HANDLE;
            pipelineInfo.pNext      = &dynRenderingInfo;
        }

        VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                                     &pipelineInfo, nullptr, &m_Pipeline);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create graphics pipeline");
    }

    void Shader::CreateComputePipeline(VkShaderModule compModule)
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = compModule;
        stageInfo.pName  = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage  = stageInfo;
        pipelineInfo.layout = m_PipelineLayout;

        VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
                                                    &pipelineInfo, nullptr, &m_Pipeline);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create compute pipeline");
    }

    // -------------------------------------------------------------------------
    // ShaderLibrary
    // -------------------------------------------------------------------------
    ShaderLibrary& ShaderLibrary::Get()
    {
        static ShaderLibrary instance;
        return instance;
    }

    void ShaderLibrary::Add(const Ref<Shader>& shader)
    {
        Add(shader->GetName(), shader);
    }

    void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
    {
        FORCE_CORE_ASSERT(!Exists(name), "Shader already exists: {}", name);
        m_Shaders[name] = shader;
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& name,
                                     const std::filesystem::path& vertexPath,
                                     const std::filesystem::path& fragmentPath)
    {
        auto shader = Shader::Create(name, vertexPath, fragmentPath);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& name,
                                     const std::filesystem::path& vertexPath,
                                     const std::filesystem::path& fragmentPath,
                                     const PipelineRenderingInfo& renderingInfo)
    {
        auto shader = Shader::Create(name, vertexPath, fragmentPath, renderingInfo);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(const std::string& name)
    {
        FORCE_CORE_ASSERT(Exists(name), "Shader not found: {}", name);
        return m_Shaders.at(name);
    }

    bool ShaderLibrary::Exists(const std::string& name) const
    {
        return m_Shaders.find(name) != m_Shaders.end();
    }

    void ShaderLibrary::Clear()
    {
        m_Shaders.clear();
    }
}
