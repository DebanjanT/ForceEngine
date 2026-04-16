#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/ParticleSystem.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>

namespace Force
{
    // -------------------------------------------------------------------------
    // Node types
    enum class VFXNodeType
    {
        // Source
        Emitter,
        // Velocity modifiers
        InitialVelocity,
        Drag,
        Gravity,
        Vortex,
        // Visual
        ColorOverLifetime,
        SizeOverLifetime,
        RotationOverLifetime,
        TextureSheet,
        // Events
        SubEmitter,
        Collision,
        // Output
        Output
    };

    // -------------------------------------------------------------------------
    // A single pin value (float, vec3, vec4, bool, int)
    using VFXPinValue = std::variant<f32, glm::vec3, glm::vec4, bool, i32>;

    struct VFXPin
    {
        std::string Name;
        VFXPinValue DefaultValue;
        bool        IsInput = true;
    };

    // -------------------------------------------------------------------------
    struct VFXNode
    {
        u32                    Id   = 0;
        VFXNodeType            Type = VFXNodeType::Output;
        std::string            Label;
        std::vector<VFXPin>    Inputs;
        std::vector<VFXPin>    Outputs;
        glm::vec2              EditorPos = glm::vec2(0.0f); // imgui-node-editor position

        // Runtime property overrides (name → value)
        std::unordered_map<std::string, VFXPinValue> Properties;
    };

    struct VFXLink
    {
        u32 Id     = 0;
        u32 FromNode, FromPin;
        u32 ToNode,   ToPin;
    };

    // -------------------------------------------------------------------------
    // The graph — evaluates to a ParticleEmitterDesc
    class VFXGraph
    {
    public:
        u32       AddNode(VFXNodeType type, const glm::vec2& editorPos = {});
        void      RemoveNode(u32 nodeId);
        u32       AddLink(u32 fromNode, u32 fromPin, u32 toNode, u32 toPin);
        void      RemoveLink(u32 linkId);

        VFXNode*  GetNode(u32 id);
        void      SetProperty(u32 nodeId, const std::string& name, VFXPinValue value);

        // Walk the graph from the Output node and produce an emitter desc
        ParticleEmitterDesc Evaluate() const;

        // Serialise to/from JSON (for editor save/load)
        std::string SerialiseJSON() const;
        void        DeserialiseJSON(const std::string& json);

        const std::vector<VFXNode>& GetNodes() const { return m_Nodes; }
        const std::vector<VFXLink>& GetLinks() const { return m_Links; }

        static Ref<VFXGraph> Create();

    private:
        const VFXNode* FindNodeOfType(VFXNodeType t) const;
        void  ApplyNode(const VFXNode& node, ParticleEmitterDesc& desc) const;

        std::vector<VFXNode> m_Nodes;
        std::vector<VFXLink> m_Links;
        u32                  m_NextId = 1;
    };
}
