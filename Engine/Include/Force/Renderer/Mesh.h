#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/VulkanBuffer.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>
namespace Force
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoord;
        glm::vec3 Tangent;
        
        static VkVertexInputBindingDescription GetBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions();
        
        bool operator==(const Vertex& other) const
        {
            return Position == other.Position && 
                   Normal == other.Normal && 
                   TexCoord == other.TexCoord &&
                   Tangent == other.Tangent;
        }
    };
    
    struct SubMesh
    {
        u32 BaseVertex = 0;
        u32 BaseIndex = 0;
        u32 IndexCount = 0;
        u32 MaterialIndex = 0;
        
        glm::mat4 Transform = glm::mat4(1.0f);
        glm::vec3 BoundingBoxMin = glm::vec3(0.0f);
        glm::vec3 BoundingBoxMax = glm::vec3(0.0f);
    };
    
    struct MeshSource
    {
        std::vector<Vertex> Vertices;
        std::vector<u32> Indices;
        std::vector<SubMesh> SubMeshes;
        
        glm::vec3 BoundingBoxMin = glm::vec3(0.0f);
        glm::vec3 BoundingBoxMax = glm::vec3(0.0f);
    };
    
    class Mesh
    {
    public:
        Mesh() = default;
        Mesh(const std::string& name);
        Mesh(const std::vector<Vertex>& vertices, const std::vector<u32>& indices);
        ~Mesh();
        
        void Create(const MeshSource& source);
        void Destroy();
        
        void Bind(VkCommandBuffer cmd) const;
        void Draw(VkCommandBuffer cmd, u32 instanceCount = 1) const;
        void DrawSubMesh(VkCommandBuffer cmd, u32 subMeshIndex, u32 instanceCount = 1) const;
        
        const std::string& GetName() const { return m_Name; }
        u32 GetVertexCount() const { return m_VertexCount; }
        u32 GetIndexCount() const { return m_IndexCount; }
        const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
        
        const glm::vec3& GetBoundingBoxMin() const { return m_BoundingBoxMin; }
        const glm::vec3& GetBoundingBoxMax() const { return m_BoundingBoxMax; }
        
        static Ref<Mesh> Create(const std::string& name);
        static Ref<Mesh> CreateFromFile(const std::filesystem::path& path);
        
        // Primitive generators
        static Ref<Mesh> CreateCube(f32 size = 1.0f);
        static Ref<Mesh> CreateSphere(f32 radius = 0.5f, u32 segments = 32, u32 rings = 16);
        static Ref<Mesh> CreatePlane(f32 width = 1.0f, f32 height = 1.0f);
        static Ref<Mesh> CreateCylinder(f32 radius = 0.5f, f32 height = 1.0f, u32 segments = 32);
        
    private:
        void CalculateTangents(std::vector<Vertex>& vertices, const std::vector<u32>& indices);
        void CalculateBoundingBox(const std::vector<Vertex>& vertices);
        
    private:
        std::string m_Name;
        
        Scope<VertexBuffer> m_VertexBuffer;
        Scope<IndexBuffer> m_IndexBuffer;
        
        u32 m_VertexCount = 0;
        u32 m_IndexCount = 0;
        
        std::vector<SubMesh> m_SubMeshes;
        
        glm::vec3 m_BoundingBoxMin = glm::vec3(0.0f);
        glm::vec3 m_BoundingBoxMax = glm::vec3(0.0f);
    };
    
    class MeshLibrary
    {
    public:
        static MeshLibrary& Get();
        
        void Add(const Ref<Mesh>& mesh);
        void Add(const std::string& name, const Ref<Mesh>& mesh);
        Ref<Mesh> Load(const std::filesystem::path& path);
        Ref<Mesh> Get(const std::string& name);
        bool Exists(const std::string& name) const;
        
        void Clear();
        
    private:
        MeshLibrary() = default;
        std::unordered_map<std::string, Ref<Mesh>> m_Meshes;
    };
}
