#include "Force/Renderer/AtmosphereRenderer.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"
#include <vk_mem_alloc.h>
#include <fstream>
#include <vector>
#include <cstring>

namespace Force
{
    static VkShaderModule LoadSPIRV(const char* path)
    {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) { FORCE_CORE_ERROR("AtmosphereRenderer: cannot open shader {}", path); return VK_NULL_HANDLE; }
        size_t size = (size_t)f.tellg();
        f.seekg(0);
        std::vector<u32> code(size / 4);
        f.read(reinterpret_cast<char*>(code.data()), size);
        VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        ci.codeSize = size; ci.pCode = code.data();
        VkShaderModule mod = VK_NULL_HANDLE;
        vkCreateShaderModule(VulkanDevice::Get().GetDevice(), &ci, nullptr, &mod);
        return mod;
    }

    AtmosphereRenderer& AtmosphereRenderer::Get()
    {
        static AtmosphereRenderer instance;
        return instance;
    }

    void AtmosphereRenderer::Init(const AtmosphereDesc& desc)
    {
        if (m_Initialized) Shutdown();
        m_Desc = desc;
        CreateUBO();
        CreateDescriptors();
        CreatePipeline();
        m_Initialized = true;
        FORCE_CORE_INFO("AtmosphereRenderer: initialized");
    }

    void AtmosphereRenderer::Shutdown()
    {
        if (!m_Initialized) return;
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();
        VulkanDevice::Get().WaitIdle();

        if (m_UBOMapped) vmaUnmapMemory(allocator, m_UBOAlloc);
        if (m_UBO)       vmaDestroyBuffer(allocator, m_UBO, m_UBOAlloc);
        if (m_Pipeline)  vkDestroyPipeline(device, m_Pipeline, nullptr);
        if (m_Layout)    vkDestroyPipelineLayout(device, m_Layout, nullptr);
        if (m_DSL)       vkDestroyDescriptorSetLayout(device, m_DSL, nullptr);
        if (m_Pool)      vkDestroyDescriptorPool(device, m_Pool, nullptr);
        m_Initialized = false;
    }

    void AtmosphereRenderer::CreateUBO()
    {
        VkBufferCreateInfo bCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bCI.size  = sizeof(AtmosphereUBO);
        bCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VmaAllocationCreateInfo aCI{};
        aCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        aCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo info{};
        vmaCreateBuffer(VulkanDevice::Get().GetAllocator(), &bCI, &aCI, &m_UBO, &m_UBOAlloc, &info);
        m_UBOMapped = info.pMappedData;
    }

    void AtmosphereRenderer::CreateDescriptors()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        VkDescriptorPoolSize ps{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
        VkDescriptorPoolCreateInfo poolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolCI.maxSets = 1; poolCI.poolSizeCount = 1; poolCI.pPoolSizes = &ps;
        vkCreateDescriptorPool(device, &poolCI, nullptr, &m_Pool);

        VkDescriptorSetLayoutBinding binding{};
        binding.binding         = 0;
        binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo dslCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        dslCI.bindingCount = 1; dslCI.pBindings = &binding;
        vkCreateDescriptorSetLayout(device, &dslCI, nullptr, &m_DSL);

        VkDescriptorSetAllocateInfo allocI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocI.descriptorPool = m_Pool; allocI.descriptorSetCount = 1; allocI.pSetLayouts = &m_DSL;
        vkAllocateDescriptorSets(device, &allocI, &m_DS);

        VkDescriptorBufferInfo bufInfo{ m_UBO, 0, sizeof(AtmosphereUBO) };
        VkWriteDescriptorSet   write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet          = m_DS;
        write.dstBinding      = 0;
        write.descriptorCount = 1;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo     = &bufInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    void AtmosphereRenderer::CreatePipeline()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        VkShaderModule vert = LoadSPIRV("Assets/Shaders/Compiled/Atmosphere.vert.spv");
        VkShaderModule frag = LoadSPIRV("Assets/Shaders/Compiled/Atmosphere.frag.spv");

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;   stages[0].module = vert; stages[0].pName = "main";
        stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module = frag; stages[1].pName = "main";

        VkPipelineVertexInputStateCreateInfo   vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo   vps{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vps.viewportCount = 1; vps.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = VK_CULL_MODE_NONE; rs.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds.depthTestEnable  = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState blendAtt{};
        blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        cb.attachmentCount = 1; cb.pAttachments = &blendAtt;

        VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dyn.dynamicStateCount = 2; dyn.pDynamicStates = dynStates;

        VkPipelineLayoutCreateInfo layoutCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        layoutCI.setLayoutCount = 1; layoutCI.pSetLayouts = &m_DSL;
        vkCreatePipelineLayout(device, &layoutCI, nullptr, &m_Layout);

        // Dynamic rendering (VK 1.3)
        VkPipelineRenderingCreateInfo renderCI{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        renderCI.colorAttachmentCount    = 1;
        renderCI.pColorAttachmentFormats = &m_Desc.ColorFormat;

        VkGraphicsPipelineCreateInfo gpCI{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        gpCI.pNext               = &renderCI;
        gpCI.stageCount          = 2; gpCI.pStages             = stages;
        gpCI.pVertexInputState   = &vi;
        gpCI.pInputAssemblyState = &ia;
        gpCI.pViewportState      = &vps;
        gpCI.pRasterizationState = &rs;
        gpCI.pMultisampleState   = &ms;
        gpCI.pDepthStencilState  = &ds;
        gpCI.pColorBlendState    = &cb;
        gpCI.pDynamicState       = &dyn;
        gpCI.layout              = m_Layout;
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpCI, nullptr, &m_Pipeline);

        vkDestroyShaderModule(device, vert, nullptr);
        vkDestroyShaderModule(device, frag, nullptr);
    }

    void AtmosphereRenderer::Render(VkCommandBuffer cmd,
                                     const glm::mat4& invView, const glm::mat4& invProj,
                                     const DirectionalLight& sun)
    {
        if (!m_Initialized || !m_UBOMapped) return;

        AtmosphereUBO ubo{};
        ubo.SunDirection     = glm::vec4(glm::normalize(glm::vec3(sun.Direction)), sun.Color.w);
        ubo.RayleighScatter  = glm::vec4(m_Desc.RayleighScattering, m_Desc.RayleighHeight);
        ubo.MieParams        = glm::vec4(m_Desc.MieScattering, m_Desc.MieHeight, m_Desc.MieAnisotropy, 0.0f);
        ubo.PlanetRadii      = glm::vec4(m_Desc.PlanetRadius, m_Desc.AtmosphereRadius, 0, 0);
        ubo.InvView          = invView;
        ubo.InvProj          = invProj;
        memcpy(m_UBOMapped, &ubo, sizeof(ubo));

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, 0, 1, &m_DS, 0, nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0); // fullscreen triangle
    }
}
