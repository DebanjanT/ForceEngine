#pragma once
#include "Force/Core/Base.h"
#include "Force/Renderer/Material.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

namespace Force
{
    enum class MatNodeType
    {
        // Inputs
        TextureSample,
        Constant1, Constant3, Constant4,
        TexCoord, VertexNormal, CameraVector, Time,
        // Math
        Multiply, Add, Subtract, Divide,
        Lerp, Clamp, Power, Sqrt, OneMinus,
        // Utility
        Fresnel, Normalize, Dot, Append, ComponentMask,
        // Normal
        NormalMap,
        // Output
        PBROutput,
    };

    using MatPinValue = std::variant<f32, glm::vec2, glm::vec3, glm::vec4, i32, std::string>;

    struct MatPin
    {
        std::string Name;
        MatPinValue Default = 0.0f;
        bool        IsInput = true;
    };

    struct MatNode
    {
        u32                     Id      = 0;
        MatNodeType             Type    = MatNodeType::PBROutput;
        std::string             Label;
        std::vector<MatPin>     Inputs;
        std::vector<MatPin>     Outputs;
        glm::vec2               EditorPos = {};
        std::unordered_map<std::string, MatPinValue> Props;
    };

    struct MatLink
    {
        u32 Id              = 0;
        u32 FromNode, FromPin;
        u32 ToNode,   ToPin;
    };

    struct MatCompileResult
    {
        bool        Success  = false;
        std::string Error;
        std::string FragSpvPath; // path to compiled .spv
        std::string GeneratedGLSL;
    };

    class MaterialGraph
    {
    public:
        u32       AddNode(MatNodeType type, const glm::vec2& pos = {});
        void      RemoveNode(u32 id);
        u32       AddLink(u32 fromNode, u32 fromPin, u32 toNode, u32 toPin);
        void      RemoveLink(u32 id);

        MatNode*  GetNode(u32 id);
        void      SetProperty(u32 nodeId, const std::string& key, MatPinValue value);

        // Generate GLSL source from graph, compile to SPIR-V via glslc subprocess
        MatCompileResult Compile(const std::string& outSpvPath) const;

        // Produce a MaterialSpec wired to the compiled shader
        MaterialSpec     BuildSpec(const std::string& compiledSpvPath) const;

        std::string SerializeJSON() const;
        void        DeserializeJSON(const std::string& json);

        const std::vector<MatNode>& GetNodes() const { return m_Nodes; }
        const std::vector<MatLink>& GetLinks() const { return m_Links; }

        static Ref<MaterialGraph> Create();

    private:
        std::string GenerateGLSL() const;
        std::string CodegenNode(u32 nodeId, std::vector<std::string>& lines,
                                std::unordered_map<u32, std::string>& cache) const;
        std::string ResolveInputPin(u32 nodeId, u32 pinIdx,
                                    std::vector<std::string>& lines,
                                    std::unordered_map<u32, std::string>& cache) const;
        const MatNode* FindNode(u32 id) const;
        const MatNode* FindOutputNode() const;

        std::vector<MatNode> m_Nodes;
        std::vector<MatLink> m_Links;
        u32                  m_NextId = 1;
    };
}
