#include "Force/Renderer/Mesh.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Force
{
    // Vertex
    VkVertexInputBindingDescription Vertex::GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDesc;
    }
    
    std::array<VkVertexInputAttributeDescription, 4> Vertex::GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescs{};
        
        // Position
        attributeDescs[0].binding = 0;
        attributeDescs[0].location = 0;
        attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[0].offset = offsetof(Vertex, Position);
        
        // Normal
        attributeDescs[1].binding = 0;
        attributeDescs[1].location = 1;
        attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[1].offset = offsetof(Vertex, Normal);
        
        // TexCoord
        attributeDescs[2].binding = 0;
        attributeDescs[2].location = 2;
        attributeDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescs[2].offset = offsetof(Vertex, TexCoord);
        
        // Tangent
        attributeDescs[3].binding = 0;
        attributeDescs[3].location = 3;
        attributeDescs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[3].offset = offsetof(Vertex, Tangent);
        
        return attributeDescs;
    }
    
    // Mesh
    Mesh::Mesh(const std::string& name)
        : m_Name(name)
    {
    }
    
    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<u32>& indices)
    {
        MeshSource source;
        source.Vertices = vertices;
        source.Indices = indices;
        
        SubMesh subMesh;
        subMesh.IndexCount = static_cast<u32>(indices.size());
        source.SubMeshes.push_back(subMesh);
        
        Create(source);
    }
    
    Mesh::~Mesh()
    {
        Destroy();
    }
    
    void Mesh::Create(const MeshSource& source)
    {
        m_VertexCount = static_cast<u32>(source.Vertices.size());
        m_IndexCount = static_cast<u32>(source.Indices.size());
        m_SubMeshes = source.SubMeshes;
        m_BoundingBoxMin = source.BoundingBoxMin;
        m_BoundingBoxMax = source.BoundingBoxMax;
        
        // Create vertex buffer
        m_VertexBuffer = CreateScope<VertexBuffer>();
        m_VertexBuffer->Create(source.Vertices.data(), 
                               source.Vertices.size() * sizeof(Vertex));
        
        // Create index buffer
        m_IndexBuffer = CreateScope<IndexBuffer>();
        m_IndexBuffer->Create(source.Indices.data(),
                              static_cast<u32>(source.Indices.size()));
    }
    
    void Mesh::Destroy()
    {
        m_VertexBuffer.reset();
        m_IndexBuffer.reset();
        m_VertexCount = 0;
        m_IndexCount = 0;
        m_SubMeshes.clear();
    }
    
    void Mesh::Bind(VkCommandBuffer cmd) const
    {
        VkBuffer vertexBuffers[] = { m_VertexBuffer->GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }
    
    void Mesh::Draw(VkCommandBuffer cmd, u32 instanceCount) const
    {
        vkCmdDrawIndexed(cmd, m_IndexCount, instanceCount, 0, 0, 0);
    }
    
    void Mesh::DrawSubMesh(VkCommandBuffer cmd, u32 subMeshIndex, u32 instanceCount) const
    {
        if (subMeshIndex >= m_SubMeshes.size())
        {
            FORCE_CORE_WARN("SubMesh index out of range: {}", subMeshIndex);
            return;
        }
        
        const SubMesh& subMesh = m_SubMeshes[subMeshIndex];
        vkCmdDrawIndexed(cmd, subMesh.IndexCount, instanceCount, 
                         subMesh.BaseIndex, subMesh.BaseVertex, 0);
    }
    
    Ref<Mesh> Mesh::Create(const std::string& name)
    {
        return CreateRef<Mesh>(name);
    }
    
    Ref<Mesh> Mesh::CreateFromFile(const std::filesystem::path& path)
    {
        // TODO: Implement mesh loading from file (OBJ, FBX, GLTF)
        FORCE_CORE_WARN("Mesh loading from file not yet implemented: {}", path.string());
        return nullptr;
    }
    
    Ref<Mesh> Mesh::CreateCube(f32 size)
    {
        f32 h = size * 0.5f;
        
        std::vector<Vertex> vertices = {
            // Front face
            {{ -h, -h,  h }, {  0,  0,  1 }, { 0, 0 }, { 1, 0, 0 }},
            {{  h, -h,  h }, {  0,  0,  1 }, { 1, 0 }, { 1, 0, 0 }},
            {{  h,  h,  h }, {  0,  0,  1 }, { 1, 1 }, { 1, 0, 0 }},
            {{ -h,  h,  h }, {  0,  0,  1 }, { 0, 1 }, { 1, 0, 0 }},
            // Back face
            {{  h, -h, -h }, {  0,  0, -1 }, { 0, 0 }, { -1, 0, 0 }},
            {{ -h, -h, -h }, {  0,  0, -1 }, { 1, 0 }, { -1, 0, 0 }},
            {{ -h,  h, -h }, {  0,  0, -1 }, { 1, 1 }, { -1, 0, 0 }},
            {{  h,  h, -h }, {  0,  0, -1 }, { 0, 1 }, { -1, 0, 0 }},
            // Top face
            {{ -h,  h,  h }, {  0,  1,  0 }, { 0, 0 }, { 1, 0, 0 }},
            {{  h,  h,  h }, {  0,  1,  0 }, { 1, 0 }, { 1, 0, 0 }},
            {{  h,  h, -h }, {  0,  1,  0 }, { 1, 1 }, { 1, 0, 0 }},
            {{ -h,  h, -h }, {  0,  1,  0 }, { 0, 1 }, { 1, 0, 0 }},
            // Bottom face
            {{ -h, -h, -h }, {  0, -1,  0 }, { 0, 0 }, { 1, 0, 0 }},
            {{  h, -h, -h }, {  0, -1,  0 }, { 1, 0 }, { 1, 0, 0 }},
            {{  h, -h,  h }, {  0, -1,  0 }, { 1, 1 }, { 1, 0, 0 }},
            {{ -h, -h,  h }, {  0, -1,  0 }, { 0, 1 }, { 1, 0, 0 }},
            // Right face
            {{  h, -h,  h }, {  1,  0,  0 }, { 0, 0 }, { 0, 0, -1 }},
            {{  h, -h, -h }, {  1,  0,  0 }, { 1, 0 }, { 0, 0, -1 }},
            {{  h,  h, -h }, {  1,  0,  0 }, { 1, 1 }, { 0, 0, -1 }},
            {{  h,  h,  h }, {  1,  0,  0 }, { 0, 1 }, { 0, 0, -1 }},
            // Left face
            {{ -h, -h, -h }, { -1,  0,  0 }, { 0, 0 }, { 0, 0, 1 }},
            {{ -h, -h,  h }, { -1,  0,  0 }, { 1, 0 }, { 0, 0, 1 }},
            {{ -h,  h,  h }, { -1,  0,  0 }, { 1, 1 }, { 0, 0, 1 }},
            {{ -h,  h, -h }, { -1,  0,  0 }, { 0, 1 }, { 0, 0, 1 }},
        };
        
        std::vector<u32> indices = {
            0,  1,  2,  2,  3,  0,   // Front
            4,  5,  6,  6,  7,  4,   // Back
            8,  9,  10, 10, 11, 8,   // Top
            12, 13, 14, 14, 15, 12,  // Bottom
            16, 17, 18, 18, 19, 16,  // Right
            20, 21, 22, 22, 23, 20   // Left
        };
        
        auto mesh = CreateRef<Mesh>("Cube");
        
        MeshSource source;
        source.Vertices = vertices;
        source.Indices = indices;
        source.BoundingBoxMin = glm::vec3(-h);
        source.BoundingBoxMax = glm::vec3(h);
        
        SubMesh subMesh;
        subMesh.IndexCount = static_cast<u32>(indices.size());
        source.SubMeshes.push_back(subMesh);
        
        mesh->Create(source);
        return mesh;
    }
    
    Ref<Mesh> Mesh::CreateSphere(f32 radius, u32 segments, u32 rings)
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        
        for (u32 y = 0; y <= rings; y++)
        {
            for (u32 x = 0; x <= segments; x++)
            {
                f32 xSegment = static_cast<f32>(x) / static_cast<f32>(segments);
                f32 ySegment = static_cast<f32>(y) / static_cast<f32>(rings);
                
                f32 xPos = std::cos(xSegment * 2.0f * static_cast<f32>(M_PI)) * 
                           std::sin(ySegment * static_cast<f32>(M_PI));
                f32 yPos = std::cos(ySegment * static_cast<f32>(M_PI));
                f32 zPos = std::sin(xSegment * 2.0f * static_cast<f32>(M_PI)) * 
                           std::sin(ySegment * static_cast<f32>(M_PI));
                
                Vertex vertex;
                vertex.Position = glm::vec3(xPos, yPos, zPos) * radius;
                vertex.Normal = glm::vec3(xPos, yPos, zPos);
                vertex.TexCoord = glm::vec2(xSegment, ySegment);
                vertex.Tangent = glm::normalize(glm::vec3(-zPos, 0, xPos));
                
                vertices.push_back(vertex);
            }
        }
        
        for (u32 y = 0; y < rings; y++)
        {
            for (u32 x = 0; x < segments; x++)
            {
                u32 current = y * (segments + 1) + x;
                u32 next = current + segments + 1;
                
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
        
        auto mesh = CreateRef<Mesh>("Sphere");
        
        MeshSource source;
        source.Vertices = vertices;
        source.Indices = indices;
        source.BoundingBoxMin = glm::vec3(-radius);
        source.BoundingBoxMax = glm::vec3(radius);
        
        SubMesh subMesh;
        subMesh.IndexCount = static_cast<u32>(indices.size());
        source.SubMeshes.push_back(subMesh);
        
        mesh->Create(source);
        return mesh;
    }
    
    Ref<Mesh> Mesh::CreatePlane(f32 width, f32 height)
    {
        f32 hw = width * 0.5f;
        f32 hh = height * 0.5f;
        
        std::vector<Vertex> vertices = {
            {{ -hw, 0, -hh }, { 0, 1, 0 }, { 0, 0 }, { 1, 0, 0 }},
            {{  hw, 0, -hh }, { 0, 1, 0 }, { 1, 0 }, { 1, 0, 0 }},
            {{  hw, 0,  hh }, { 0, 1, 0 }, { 1, 1 }, { 1, 0, 0 }},
            {{ -hw, 0,  hh }, { 0, 1, 0 }, { 0, 1 }, { 1, 0, 0 }},
        };
        
        std::vector<u32> indices = { 0, 2, 1, 0, 3, 2 };
        
        auto mesh = CreateRef<Mesh>("Plane");
        
        MeshSource source;
        source.Vertices = vertices;
        source.Indices = indices;
        source.BoundingBoxMin = glm::vec3(-hw, 0, -hh);
        source.BoundingBoxMax = glm::vec3(hw, 0, hh);
        
        SubMesh subMesh;
        subMesh.IndexCount = static_cast<u32>(indices.size());
        source.SubMeshes.push_back(subMesh);
        
        mesh->Create(source);
        return mesh;
    }
    
    Ref<Mesh> Mesh::CreateCylinder(f32 radius, f32 height, u32 segments)
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        
        f32 halfHeight = height * 0.5f;
        
        // Side vertices
        for (u32 i = 0; i <= segments; i++)
        {
            f32 angle = static_cast<f32>(i) / static_cast<f32>(segments) * 2.0f * static_cast<f32>(M_PI);
            f32 x = std::cos(angle);
            f32 z = std::sin(angle);
            
            // Bottom vertex
            Vertex bottom;
            bottom.Position = glm::vec3(x * radius, -halfHeight, z * radius);
            bottom.Normal = glm::vec3(x, 0, z);
            bottom.TexCoord = glm::vec2(static_cast<f32>(i) / segments, 0);
            bottom.Tangent = glm::vec3(-z, 0, x);
            vertices.push_back(bottom);
            
            // Top vertex
            Vertex top;
            top.Position = glm::vec3(x * radius, halfHeight, z * radius);
            top.Normal = glm::vec3(x, 0, z);
            top.TexCoord = glm::vec2(static_cast<f32>(i) / segments, 1);
            top.Tangent = glm::vec3(-z, 0, x);
            vertices.push_back(top);
        }
        
        // Side indices
        for (u32 i = 0; i < segments; i++)
        {
            u32 bl = i * 2;
            u32 tl = bl + 1;
            u32 br = bl + 2;
            u32 tr = bl + 3;
            
            indices.push_back(bl);
            indices.push_back(br);
            indices.push_back(tl);
            
            indices.push_back(tl);
            indices.push_back(br);
            indices.push_back(tr);
        }
        
        // Top cap center
        u32 topCenterIdx = static_cast<u32>(vertices.size());
        Vertex topCenter;
        topCenter.Position = glm::vec3(0, halfHeight, 0);
        topCenter.Normal = glm::vec3(0, 1, 0);
        topCenter.TexCoord = glm::vec2(0.5f, 0.5f);
        topCenter.Tangent = glm::vec3(1, 0, 0);
        vertices.push_back(topCenter);
        
        // Bottom cap center
        u32 bottomCenterIdx = static_cast<u32>(vertices.size());
        Vertex bottomCenter;
        bottomCenter.Position = glm::vec3(0, -halfHeight, 0);
        bottomCenter.Normal = glm::vec3(0, -1, 0);
        bottomCenter.TexCoord = glm::vec2(0.5f, 0.5f);
        bottomCenter.Tangent = glm::vec3(1, 0, 0);
        vertices.push_back(bottomCenter);
        
        // Cap vertices and indices
        for (u32 i = 0; i <= segments; i++)
        {
            f32 angle = static_cast<f32>(i) / static_cast<f32>(segments) * 2.0f * static_cast<f32>(M_PI);
            f32 x = std::cos(angle);
            f32 z = std::sin(angle);
            
            // Top cap vertex
            u32 topIdx = static_cast<u32>(vertices.size());
            Vertex topV;
            topV.Position = glm::vec3(x * radius, halfHeight, z * radius);
            topV.Normal = glm::vec3(0, 1, 0);
            topV.TexCoord = glm::vec2(x * 0.5f + 0.5f, z * 0.5f + 0.5f);
            topV.Tangent = glm::vec3(1, 0, 0);
            vertices.push_back(topV);
            
            // Bottom cap vertex
            u32 bottomIdx = static_cast<u32>(vertices.size());
            Vertex bottomV;
            bottomV.Position = glm::vec3(x * radius, -halfHeight, z * radius);
            bottomV.Normal = glm::vec3(0, -1, 0);
            bottomV.TexCoord = glm::vec2(x * 0.5f + 0.5f, z * 0.5f + 0.5f);
            bottomV.Tangent = glm::vec3(1, 0, 0);
            vertices.push_back(bottomV);
            
            if (i > 0)
            {
                // Top cap triangle
                indices.push_back(topCenterIdx);
                indices.push_back(topIdx - 2);
                indices.push_back(topIdx);
                
                // Bottom cap triangle
                indices.push_back(bottomCenterIdx);
                indices.push_back(bottomIdx);
                indices.push_back(bottomIdx - 2);
            }
        }
        
        auto mesh = CreateRef<Mesh>("Cylinder");
        
        MeshSource source;
        source.Vertices = vertices;
        source.Indices = indices;
        source.BoundingBoxMin = glm::vec3(-radius, -halfHeight, -radius);
        source.BoundingBoxMax = glm::vec3(radius, halfHeight, radius);
        
        SubMesh subMesh;
        subMesh.IndexCount = static_cast<u32>(indices.size());
        source.SubMeshes.push_back(subMesh);
        
        mesh->Create(source);
        return mesh;
    }
    
    void Mesh::CalculateTangents(std::vector<Vertex>& vertices, const std::vector<u32>& indices)
    {
        // TODO: Implement Mikktspace tangent calculation
    }
    
    void Mesh::CalculateBoundingBox(const std::vector<Vertex>& vertices)
    {
        if (vertices.empty()) return;
        
        m_BoundingBoxMin = vertices[0].Position;
        m_BoundingBoxMax = vertices[0].Position;
        
        for (const auto& v : vertices)
        {
            m_BoundingBoxMin = glm::min(m_BoundingBoxMin, v.Position);
            m_BoundingBoxMax = glm::max(m_BoundingBoxMax, v.Position);
        }
    }
    
    // MeshLibrary
    MeshLibrary& MeshLibrary::Get()
    {
        static MeshLibrary instance;
        return instance;
    }
    
    void MeshLibrary::Add(const Ref<Mesh>& mesh)
    {
        Add(mesh->GetName(), mesh);
    }
    
    void MeshLibrary::Add(const std::string& name, const Ref<Mesh>& mesh)
    {
        FORCE_CORE_ASSERT(!Exists(name), "Mesh already exists: {}", name);
        m_Meshes[name] = mesh;
    }
    
    Ref<Mesh> MeshLibrary::Load(const std::filesystem::path& path)
    {
        auto mesh = Mesh::CreateFromFile(path);
        if (mesh)
        {
            Add(mesh);
        }
        return mesh;
    }
    
    Ref<Mesh> MeshLibrary::Get(const std::string& name)
    {
        FORCE_CORE_ASSERT(Exists(name), "Mesh not found: {}", name);
        return m_Meshes.at(name);
    }
    
    bool MeshLibrary::Exists(const std::string& name) const
    {
        return m_Meshes.find(name) != m_Meshes.end();
    }
    
    void MeshLibrary::Clear()
    {
        m_Meshes.clear();
    }
}
