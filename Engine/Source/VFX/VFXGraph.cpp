#include "Force/VFX/VFXGraph.h"
#include "Force/Core/Logger.h"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace Force
{
    u32 VFXGraph::AddNode(VFXNodeType type, const glm::vec2& editorPos)
    {
        VFXNode node;
        node.Id        = m_NextId++;
        node.Type      = type;
        node.EditorPos = editorPos;

        // Set up default pins per node type
        switch (type)
        {
            case VFXNodeType::Emitter:
                node.Label = "Emitter";
                node.Inputs  = {};
                node.Outputs = { { "Out", 0.0f, false } };
                node.Properties["MaxParticles"] = 1000;
                node.Properties["EmissionRate"] = 100.0f;
                node.Properties["LifetimeMin"]  = 1.0f;
                node.Properties["LifetimeMax"]  = 2.0f;
                break;

            case VFXNodeType::InitialVelocity:
                node.Label = "Initial Velocity";
                node.Inputs  = { { "In",   0.0f,                false } };
                node.Outputs = { { "Out",  0.0f,                false } };
                node.Properties["VelocityMin"] = glm::vec3(-1, 2, -1);
                node.Properties["VelocityMax"] = glm::vec3( 1, 5,  1);
                break;

            case VFXNodeType::Drag:
                node.Label = "Drag";
                node.Inputs  = { { "In",  0.0f, false } };
                node.Outputs = { { "Out", 0.0f, false } };
                node.Properties["Drag"] = 0.1f;
                break;

            case VFXNodeType::Gravity:
                node.Label = "Gravity";
                node.Inputs  = { { "In",  0.0f, false } };
                node.Outputs = { { "Out", 0.0f, false } };
                node.Properties["Gravity"] = glm::vec3(0, -9.81f, 0);
                break;

            case VFXNodeType::ColorOverLifetime:
                node.Label = "Color Over Lifetime";
                node.Inputs  = { { "In",  0.0f, false } };
                node.Outputs = { { "Out", 0.0f, false } };
                node.Properties["ColorStart"] = glm::vec4(1, 1, 1, 1);
                node.Properties["ColorEnd"]   = glm::vec4(1, 1, 1, 0);
                break;

            case VFXNodeType::SizeOverLifetime:
                node.Label = "Size Over Lifetime";
                node.Inputs  = { { "In",  0.0f, false } };
                node.Outputs = { { "Out", 0.0f, false } };
                node.Properties["SizeStart"] = glm::vec3(0.1f, 0.1f, 0);
                node.Properties["SizeEnd"]   = glm::vec3(0.0f, 0.0f, 0);
                break;

            case VFXNodeType::SubEmitter:
                node.Label = "Sub Emitter";
                node.Inputs  = { { "In",  0.0f, false } };
                node.Outputs = { { "Out", 0.0f, false } };
                node.Properties["Event"] = 0;  // 0=Birth, 1=Death
                break;

            case VFXNodeType::Output:
                node.Label = "Output";
                node.Inputs  = { { "In", 0.0f, true } };
                node.Outputs = {};
                break;

            default:
                node.Label = "Unknown";
                break;
        }

        m_Nodes.push_back(node);
        return node.Id;
    }

    void VFXGraph::RemoveNode(u32 nodeId)
    {
        m_Nodes.erase(std::remove_if(m_Nodes.begin(), m_Nodes.end(),
            [nodeId](const VFXNode& n){ return n.Id == nodeId; }), m_Nodes.end());
        m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
            [nodeId](const VFXLink& l){
                return l.FromNode == nodeId || l.ToNode == nodeId; }),
            m_Links.end());
    }

    u32 VFXGraph::AddLink(u32 fromNode, u32 fromPin, u32 toNode, u32 toPin)
    {
        VFXLink link{ m_NextId++, fromNode, fromPin, toNode, toPin };
        m_Links.push_back(link);
        return link.Id;
    }

    void VFXGraph::RemoveLink(u32 linkId)
    {
        m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
            [linkId](const VFXLink& l){ return l.Id == linkId; }), m_Links.end());
    }

    VFXNode* VFXGraph::GetNode(u32 id)
    {
        for (auto& n : m_Nodes)
            if (n.Id == id) return &n;
        return nullptr;
    }

    void VFXGraph::SetProperty(u32 nodeId, const std::string& name, VFXPinValue value)
    {
        if (auto* n = GetNode(nodeId))
            n->Properties[name] = value;
    }

    ParticleEmitterDesc VFXGraph::Evaluate() const
    {
        ParticleEmitterDesc desc;

        // Walk all nodes and apply them to desc
        for (const auto& node : m_Nodes)
            ApplyNode(node, desc);

        return desc;
    }

    void VFXGraph::ApplyNode(const VFXNode& node, ParticleEmitterDesc& desc) const
    {
        auto getF = [&](const std::string& k, f32 def) -> f32 {
            auto it = node.Properties.find(k);
            if (it == node.Properties.end()) return def;
            return std::holds_alternative<f32>(it->second) ? std::get<f32>(it->second) : def;
        };
        auto getV3 = [&](const std::string& k, glm::vec3 def) -> glm::vec3 {
            auto it = node.Properties.find(k);
            if (it == node.Properties.end()) return def;
            return std::holds_alternative<glm::vec3>(it->second) ? std::get<glm::vec3>(it->second) : def;
        };
        auto getV4 = [&](const std::string& k, glm::vec4 def) -> glm::vec4 {
            auto it = node.Properties.find(k);
            if (it == node.Properties.end()) return def;
            return std::holds_alternative<glm::vec4>(it->second) ? std::get<glm::vec4>(it->second) : def;
        };

        switch (node.Type)
        {
            case VFXNodeType::Emitter:
            {
                auto it = node.Properties.find("MaxParticles");
                if (it != node.Properties.end() && std::holds_alternative<i32>(it->second))
                    desc.MaxParticles = static_cast<u32>(std::get<i32>(it->second));
                desc.EmissionRate = getF("EmissionRate", 100.0f);
                desc.LifetimeMin  = getF("LifetimeMin",   1.0f);
                desc.LifetimeMax  = getF("LifetimeMax",   2.0f);
                break;
            }
            case VFXNodeType::InitialVelocity:
                desc.InitialVelocityMin = getV3("VelocityMin", { -1,2,-1 });
                desc.InitialVelocityMax = getV3("VelocityMax", {  1,5, 1 });
                break;
            case VFXNodeType::Drag:
                desc.Drag = getF("Drag", 0.1f);
                break;
            case VFXNodeType::Gravity:
                desc.Gravity = getV3("Gravity", { 0,-9.81f,0 });
                break;
            case VFXNodeType::ColorOverLifetime:
                desc.ColorStart = getV4("ColorStart", { 1,1,1,1 });
                desc.ColorEnd   = getV4("ColorEnd",   { 1,1,1,0 });
                break;
            case VFXNodeType::SizeOverLifetime:
            {
                glm::vec3 ss = getV3("SizeStart", { 0.1f, 0.1f, 0 });
                glm::vec3 se = getV3("SizeEnd",   { 0.0f, 0.0f, 0 });
                desc.SizeStart = { ss.x, ss.y };
                desc.SizeEnd   = { se.x, se.y };
                break;
            }
            default: break;
        }
    }

    const VFXNode* VFXGraph::FindNodeOfType(VFXNodeType t) const
    {
        for (const auto& n : m_Nodes)
            if (n.Type == t) return &n;
        return nullptr;
    }

    std::string VFXGraph::SerialiseJSON() const
    {
        nlohmann::json j;
        j["nodes"] = nlohmann::json::array();
        for (const auto& n : m_Nodes)
        {
            nlohmann::json jn;
            jn["id"]    = n.Id;
            jn["type"]  = static_cast<int>(n.Type);
            jn["label"] = n.Label;
            jn["pos"]   = { n.EditorPos.x, n.EditorPos.y };
            j["nodes"].push_back(jn);
        }
        j["links"] = nlohmann::json::array();
        for (const auto& l : m_Links)
        {
            j["links"].push_back({
                {"id",       l.Id},
                {"fromNode", l.FromNode}, {"fromPin", l.FromPin},
                {"toNode",   l.ToNode},   {"toPin",   l.ToPin}
            });
        }
        return j.dump(2);
    }

    void VFXGraph::DeserialiseJSON(const std::string& json)
    {
        m_Nodes.clear();
        m_Links.clear();
        try
        {
            auto j = nlohmann::json::parse(json);
            for (const auto& jn : j["nodes"])
            {
                VFXNode n;
                n.Id        = jn["id"];
                n.Type      = static_cast<VFXNodeType>(int(jn["type"]));
                n.Label     = jn["label"];
                n.EditorPos = { jn["pos"][0], jn["pos"][1] };
                m_Nodes.push_back(n);
                m_NextId = std::max(m_NextId, n.Id + 1);
            }
            for (const auto& jl : j["links"])
            {
                VFXLink l;
                l.Id       = jl["id"];
                l.FromNode = jl["fromNode"]; l.FromPin = jl["fromPin"];
                l.ToNode   = jl["toNode"];   l.ToPin   = jl["toPin"];
                m_Links.push_back(l);
                m_NextId = std::max(m_NextId, l.Id + 1);
            }
        }
        catch (...) { FORCE_CORE_ERROR("VFXGraph: JSON deserialise failed"); }
    }

    Ref<VFXGraph> VFXGraph::Create()
    {
        auto g = CreateRef<VFXGraph>();
        g->AddNode(VFXNodeType::Emitter, { 50, 200 });
        g->AddNode(VFXNodeType::Output,  { 500, 200 });
        return g;
    }
}
