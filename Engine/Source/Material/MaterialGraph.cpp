#include "Force/Material/MaterialGraph.h"
#include "Force/Core/Logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <algorithm>

namespace Force
{
    // -------------------------------------------------------------------------
    // Node factory helpers

    static void PopulateNode(MatNode& n)
    {
        n.Label = "";
        switch (n.Type)
        {
        case MatNodeType::TextureSample:
            n.Label = "Texture Sample";
            n.Inputs  = { {"UV", glm::vec2{0,0}} };
            n.Outputs = { {"RGBA", glm::vec4{1,1,1,1}}, {"RGB", glm::vec3{1,1,1}}, {"R", 0.0f}, {"G", 0.0f}, {"B", 0.0f}, {"A", 1.0f} };
            n.Props["TexturePath"] = std::string("");
            n.Props["SamplerIndex"] = 0;
            break;
        case MatNodeType::Constant1:
            n.Label = "Scalar";
            n.Outputs = { {"Out", 0.0f} };
            n.Props["Value"] = 0.0f;
            break;
        case MatNodeType::Constant3:
            n.Label = "Vector3";
            n.Outputs = { {"Out", glm::vec3{0,0,0}} };
            n.Props["Value"] = glm::vec3{0,0,0};
            break;
        case MatNodeType::Constant4:
            n.Label = "Vector4";
            n.Outputs = { {"Out", glm::vec4{0,0,0,0}} };
            n.Props["Value"] = glm::vec4{0,0,0,0};
            break;
        case MatNodeType::TexCoord:
            n.Label = "TexCoord";
            n.Outputs = { {"UV", glm::vec2{0,0}} };
            break;
        case MatNodeType::VertexNormal:
            n.Label = "Vertex Normal";
            n.Outputs = { {"Normal", glm::vec3{0,0,1}} };
            break;
        case MatNodeType::Time:
            n.Label = "Time";
            n.Outputs = { {"Time", 0.0f}, {"SinTime", 0.0f} };
            break;
        case MatNodeType::Multiply:
            n.Label = "Multiply";
            n.Inputs  = { {"A", 1.0f}, {"B", 1.0f} };
            n.Outputs = { {"Out", 0.0f} };
            break;
        case MatNodeType::Add:
            n.Label = "Add";
            n.Inputs  = { {"A", 0.0f}, {"B", 0.0f} };
            n.Outputs = { {"Out", 0.0f} };
            break;
        case MatNodeType::Subtract:
            n.Label = "Subtract";
            n.Inputs  = { {"A", 0.0f}, {"B", 0.0f} };
            n.Outputs = { {"Out", 0.0f} };
            break;
        case MatNodeType::Lerp:
            n.Label = "Lerp";
            n.Inputs  = { {"A", 0.0f}, {"B", 1.0f}, {"Alpha", 0.5f} };
            n.Outputs = { {"Out", 0.0f} };
            break;
        case MatNodeType::Clamp:
            n.Label = "Clamp";
            n.Inputs  = { {"Value", 0.0f}, {"Min", 0.0f}, {"Max", 1.0f} };
            n.Outputs = { {"Out", 0.0f} };
            break;
        case MatNodeType::Power:
            n.Label = "Power";
            n.Inputs  = { {"Base", 0.0f}, {"Exp", 2.0f} };
            n.Outputs = { {"Out", 0.0f} };
            break;
        case MatNodeType::OneMinus:
            n.Label = "1 - x";
            n.Inputs  = { {"In", 0.0f} };
            n.Outputs = { {"Out", 0.0f} };
            break;
        case MatNodeType::Fresnel:
            n.Label = "Fresnel";
            n.Inputs  = { {"Normal", glm::vec3{0,0,1}}, {"Exponent", 5.0f} };
            n.Outputs = { {"Out", 0.0f} };
            break;
        case MatNodeType::NormalMap:
            n.Label = "Normal Map";
            n.Inputs  = { {"Texture", glm::vec4{0.5f,0.5f,1,1}}, {"Strength", 1.0f} };
            n.Outputs = { {"Normal", glm::vec3{0,0,1}} };
            break;
        case MatNodeType::Append:
            n.Label = "Append";
            n.Inputs  = { {"A", 0.0f}, {"B", 0.0f} };
            n.Outputs = { {"Out", glm::vec2{0,0}} };
            break;
        case MatNodeType::PBROutput:
            n.Label = "PBR Output";
            n.Inputs = {
                {"Albedo",    glm::vec3{1,1,1}},
                {"Metallic",  0.0f},
                {"Roughness", 0.5f},
                {"Normal",    glm::vec3{0,0,1}},
                {"AO",        1.0f},
                {"Emissive",  glm::vec3{0,0,0}},
                {"EmissiveStrength", 0.0f},
                {"Opacity",   1.0f},
            };
            break;
        default:
            n.Label = "Unknown";
            break;
        }
    }

    // -------------------------------------------------------------------------

    Ref<MaterialGraph> MaterialGraph::Create()
    {
        auto g = CreateRef<MaterialGraph>();
        // Add default PBR output node
        u32 outId = g->AddNode(MatNodeType::PBROutput, { 400, 200 });
        return g;
    }

    u32 MaterialGraph::AddNode(MatNodeType type, const glm::vec2& pos)
    {
        MatNode n;
        n.Id  = m_NextId++;
        n.Type = type;
        n.EditorPos = pos;
        PopulateNode(n);
        m_Nodes.push_back(std::move(n));
        return m_Nodes.back().Id;
    }

    void MaterialGraph::RemoveNode(u32 id)
    {
        m_Nodes.erase(std::remove_if(m_Nodes.begin(), m_Nodes.end(),
            [id](const MatNode& n){ return n.Id == id; }), m_Nodes.end());
        m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
            [id](const MatLink& l){ return l.FromNode == id || l.ToNode == id; }), m_Links.end());
    }

    u32 MaterialGraph::AddLink(u32 fromNode, u32 fromPin, u32 toNode, u32 toPin)
    {
        // Remove existing link to the same input pin
        m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
            [toNode, toPin](const MatLink& l){ return l.ToNode == toNode && l.ToPin == toPin; }), m_Links.end());
        MatLink l{ m_NextId++, fromNode, fromPin, toNode, toPin };
        m_Links.push_back(l);
        return l.Id;
    }

    void MaterialGraph::RemoveLink(u32 id)
    {
        m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
            [id](const MatLink& l){ return l.Id == id; }), m_Links.end());
    }

    MatNode* MaterialGraph::GetNode(u32 id)
    {
        for (auto& n : m_Nodes) if (n.Id == id) return &n;
        return nullptr;
    }

    const MatNode* MaterialGraph::FindNode(u32 id) const
    {
        for (const auto& n : m_Nodes) if (n.Id == id) return &n;
        return nullptr;
    }

    const MatNode* MaterialGraph::FindOutputNode() const
    {
        for (const auto& n : m_Nodes) if (n.Type == MatNodeType::PBROutput) return &n;
        return nullptr;
    }

    void MaterialGraph::SetProperty(u32 nodeId, const std::string& key, MatPinValue value)
    {
        if (auto* n = GetNode(nodeId)) n->Props[key] = std::move(value);
    }

    // -------------------------------------------------------------------------
    // GLSL codegen

    static std::string PinDefaultToGLSL(const MatPinValue& val)
    {
        return std::visit([](auto&& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            std::ostringstream s;
            if constexpr (std::is_same_v<T, f32>)        s << v;
            else if constexpr (std::is_same_v<T, glm::vec2>) s << "vec2(" << v.x << "," << v.y << ")";
            else if constexpr (std::is_same_v<T, glm::vec3>) s << "vec3(" << v.x << "," << v.y << "," << v.z << ")";
            else if constexpr (std::is_same_v<T, glm::vec4>) s << "vec4(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")";
            else if constexpr (std::is_same_v<T, i32>)    s << v;
            else if constexpr (std::is_same_v<T, std::string>) s << "vec4(1)";
            return s.str();
        }, val);
    }

    std::string MaterialGraph::ResolveInputPin(u32 nodeId, u32 pinIdx,
                                                std::vector<std::string>& lines,
                                                std::unordered_map<u32, std::string>& cache) const
    {
        for (const auto& link : m_Links)
        {
            if (link.ToNode == nodeId && link.ToPin == pinIdx)
            {
                std::string srcVar = CodegenNode(link.FromNode, lines, cache);
                // If the source node has multiple outputs, index it
                const MatNode* src = FindNode(link.FromNode);
                if (src && src->Outputs.size() > 1)
                    return srcVar + "_out" + std::to_string(link.FromPin);
                return srcVar;
            }
        }
        // No link: use default value
        const MatNode* n = FindNode(nodeId);
        if (n && pinIdx < n->Inputs.size())
            return PinDefaultToGLSL(n->Inputs[pinIdx].Default);
        return "0.0";
    }

    std::string MaterialGraph::CodegenNode(u32 nodeId,
                                            std::vector<std::string>& lines,
                                            std::unordered_map<u32, std::string>& cache) const
    {
        auto it = cache.find(nodeId);
        if (it != cache.end()) return it->second;

        const MatNode* n = FindNode(nodeId);
        if (!n) return "0.0";

        std::string var = "node" + std::to_string(nodeId);
        cache[nodeId] = var;

        switch (n->Type)
        {
        case MatNodeType::Constant1:
        {
            f32 v = std::holds_alternative<f32>(n->Props.count("Value") ? n->Props.at("Value") : MatPinValue{0.0f})
                    ? std::get<f32>(n->Props.at("Value")) : 0.0f;
            lines.push_back("    float " + var + " = " + std::to_string(v) + ";");
            break;
        }
        case MatNodeType::Constant3:
        {
            auto val = n->Props.count("Value") ? n->Props.at("Value") : MatPinValue{glm::vec3{0,0,0}};
            lines.push_back("    vec3 " + var + " = " + PinDefaultToGLSL(val) + ";");
            break;
        }
        case MatNodeType::Constant4:
        {
            auto val = n->Props.count("Value") ? n->Props.at("Value") : MatPinValue{glm::vec4{0,0,0,0}};
            lines.push_back("    vec4 " + var + " = " + PinDefaultToGLSL(val) + ";");
            break;
        }
        case MatNodeType::TexCoord:
            lines.push_back("    vec2 " + var + " = inUV;");
            break;
        case MatNodeType::VertexNormal:
            lines.push_back("    vec3 " + var + " = normalize(inNormal);");
            break;
        case MatNodeType::Time:
            lines.push_back("    float " + var + " = uTime;");
            lines.push_back("    float " + var + "_out1 = sin(uTime);");
            break;
        case MatNodeType::TextureSample:
        {
            int samplerIdx = n->Props.count("SamplerIndex") && std::holds_alternative<i32>(n->Props.at("SamplerIndex"))
                             ? std::get<i32>(n->Props.at("SamplerIndex")) : 0;
            std::string uv = ResolveInputPin(nodeId, 0, lines, cache);
            lines.push_back("    vec4 " + var + "_out0 = texture(uTextures[" + std::to_string(samplerIdx) + "], " + uv + ");");
            lines.push_back("    vec3 " + var + "_out1 = " + var + "_out0.rgb;");
            lines.push_back("    float " + var + "_out2 = " + var + "_out0.r;");
            lines.push_back("    float " + var + "_out3 = " + var + "_out0.g;");
            lines.push_back("    float " + var + "_out4 = " + var + "_out0.b;");
            lines.push_back("    float " + var + "_out5 = " + var + "_out0.a;");
            lines.push_back("    vec4 " + var + " = " + var + "_out0;");
            break;
        }
        case MatNodeType::Multiply:
        {
            std::string a = ResolveInputPin(nodeId, 0, lines, cache);
            std::string b = ResolveInputPin(nodeId, 1, lines, cache);
            lines.push_back("    auto " + var + " = (" + a + ") * (" + b + ");");
            // Use float for now — GLSL doesn't have auto, fix:
            lines.back() = "    float " + var + " = float(" + a + ") * float(" + b + ");";
            break;
        }
        case MatNodeType::Add:
        {
            std::string a = ResolveInputPin(nodeId, 0, lines, cache);
            std::string b = ResolveInputPin(nodeId, 1, lines, cache);
            lines.push_back("    float " + var + " = float(" + a + ") + float(" + b + ");");
            break;
        }
        case MatNodeType::Subtract:
        {
            std::string a = ResolveInputPin(nodeId, 0, lines, cache);
            std::string b = ResolveInputPin(nodeId, 1, lines, cache);
            lines.push_back("    float " + var + " = float(" + a + ") - float(" + b + ");");
            break;
        }
        case MatNodeType::Lerp:
        {
            std::string a = ResolveInputPin(nodeId, 0, lines, cache);
            std::string b = ResolveInputPin(nodeId, 1, lines, cache);
            std::string t = ResolveInputPin(nodeId, 2, lines, cache);
            lines.push_back("    float " + var + " = mix(float(" + a + "), float(" + b + "), float(" + t + "));");
            break;
        }
        case MatNodeType::Clamp:
        {
            std::string v  = ResolveInputPin(nodeId, 0, lines, cache);
            std::string mn = ResolveInputPin(nodeId, 1, lines, cache);
            std::string mx = ResolveInputPin(nodeId, 2, lines, cache);
            lines.push_back("    float " + var + " = clamp(float(" + v + "), float(" + mn + "), float(" + mx + "));");
            break;
        }
        case MatNodeType::Power:
        {
            std::string b = ResolveInputPin(nodeId, 0, lines, cache);
            std::string e = ResolveInputPin(nodeId, 1, lines, cache);
            lines.push_back("    float " + var + " = pow(max(float(" + b + "),0.0), float(" + e + "));");
            break;
        }
        case MatNodeType::OneMinus:
        {
            std::string x = ResolveInputPin(nodeId, 0, lines, cache);
            lines.push_back("    float " + var + " = 1.0 - float(" + x + ");");
            break;
        }
        case MatNodeType::Fresnel:
        {
            std::string norm = ResolveInputPin(nodeId, 0, lines, cache);
            std::string exp  = ResolveInputPin(nodeId, 1, lines, cache);
            lines.push_back("    float " + var + " = pow(1.0 - max(dot(normalize(vec3(" + norm + ")), normalize(inViewDir)), 0.0), float(" + exp + "));");
            break;
        }
        case MatNodeType::NormalMap:
        {
            std::string tex = ResolveInputPin(nodeId, 0, lines, cache);
            std::string str = ResolveInputPin(nodeId, 1, lines, cache);
            lines.push_back("    vec3 " + var + " = normalize(mix(inNormal, (vec4(" + tex + ").xyz * 2.0 - 1.0), float(" + str + ")));");
            break;
        }
        case MatNodeType::Append:
        {
            std::string a = ResolveInputPin(nodeId, 0, lines, cache);
            std::string b = ResolveInputPin(nodeId, 1, lines, cache);
            lines.push_back("    vec2 " + var + " = vec2(float(" + a + "), float(" + b + "));");
            break;
        }
        default:
            lines.push_back("    float " + var + " = 0.0;");
            break;
        }
        return var;
    }

    std::string MaterialGraph::GenerateGLSL() const
    {
        const MatNode* output = FindOutputNode();
        if (!output) return "";

        std::vector<std::string> bodyLines;
        std::unordered_map<u32, std::string> cache;

        // Evaluate all inputs of the PBR output node
        struct PBRInputs { std::string albedo, metallic, roughness, normal, ao, emissive, emissiveStr, opacity; };
        PBRInputs pbr;
        pbr.albedo      = ResolveInputPin(output->Id, 0, bodyLines, cache);
        pbr.metallic    = ResolveInputPin(output->Id, 1, bodyLines, cache);
        pbr.roughness   = ResolveInputPin(output->Id, 2, bodyLines, cache);
        pbr.normal      = ResolveInputPin(output->Id, 3, bodyLines, cache);
        pbr.ao          = ResolveInputPin(output->Id, 4, bodyLines, cache);
        pbr.emissive    = ResolveInputPin(output->Id, 5, bodyLines, cache);
        pbr.emissiveStr = ResolveInputPin(output->Id, 6, bodyLines, cache);
        pbr.opacity     = ResolveInputPin(output->Id, 7, bodyLines, cache);

        std::ostringstream glsl;
        glsl << R"(#version 450
layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inViewDir;

layout(location = 0) out vec4 outAlbedoMetal;
layout(location = 1) out vec4 outNormalRough;
layout(location = 2) out vec4 outEmissiveAO;

layout(set = 0, binding = 0) uniform sampler2D uTextures[8];
layout(set = 0, binding = 1) uniform MatUBO { float uTime; } mat;
#define uTime mat.uTime

void main() {
)";
        for (const auto& line : bodyLines) glsl << line << "\n";
        glsl << "    // PBR output\n";
        glsl << "    vec3 _albedo    = vec3(" << pbr.albedo    << ");\n";
        glsl << "    float _metallic = float(" << pbr.metallic  << ");\n";
        glsl << "    float _rough    = float(" << pbr.roughness << ");\n";
        glsl << "    vec3 _normal    = normalize(vec3(" << pbr.normal << "));\n";
        glsl << "    float _ao       = float(" << pbr.ao        << ");\n";
        glsl << "    vec3 _emissive  = vec3(" << pbr.emissive   << ") * float(" << pbr.emissiveStr << ");\n";
        glsl << "    outAlbedoMetal = vec4(_albedo, _metallic);\n";
        glsl << "    outNormalRough = vec4(_normal * 0.5 + 0.5, _rough);\n";
        glsl << "    outEmissiveAO  = vec4(_emissive, _ao);\n";
        glsl << "}\n";
        return glsl.str();
    }

    MatCompileResult MaterialGraph::Compile(const std::string& outSpvPath) const
    {
        MatCompileResult result;
        result.GeneratedGLSL = GenerateGLSL();
        if (result.GeneratedGLSL.empty()) { result.Error = "No PBR output node"; return result; }

        // Write to temp file
        std::string tmpGlsl = outSpvPath + ".gen.frag";
        { std::ofstream f(tmpGlsl); if (!f) { result.Error = "Cannot write temp file"; return result; } f << result.GeneratedGLSL; }

        std::string cmd = "glslc -fshader-stage=frag -o \"" + outSpvPath + "\" \"" + tmpGlsl + "\"";
        int ret = std::system(cmd.c_str());
        std::remove(tmpGlsl.c_str());

        if (ret != 0) { result.Error = "glslc failed (exit " + std::to_string(ret) + ")"; FORCE_CORE_ERROR("MaterialGraph: {}", result.Error); return result; }

        result.Success     = true;
        result.FragSpvPath = outSpvPath;
        FORCE_CORE_INFO("MaterialGraph: compiled → {}", outSpvPath);
        return result;
    }

    MaterialSpec MaterialGraph::BuildSpec(const std::string& compiledSpvPath) const
    {
        MaterialSpec spec;
        spec.Name = "GeneratedMaterial";
        // Caller is responsible for constructing a Shader from the compiled .spv
        return spec;
    }

    // -------------------------------------------------------------------------
    // JSON serialization

    std::string MaterialGraph::SerializeJSON() const
    {
        nlohmann::json j;
        j["nextId"] = m_NextId;
        for (const auto& n : m_Nodes)
        {
            nlohmann::json jn;
            jn["id"]   = n.Id;
            jn["type"] = (int)n.Type;
            jn["posX"] = n.EditorPos.x;
            jn["posY"] = n.EditorPos.y;
            for (const auto& [k, v] : n.Props)
            {
                std::visit([&](auto&& val){
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, f32>)         jn["props"][k] = val;
                    else if constexpr (std::is_same_v<T, i32>)    jn["props"][k] = val;
                    else if constexpr (std::is_same_v<T, std::string>) jn["props"][k] = val;
                    else if constexpr (std::is_same_v<T, glm::vec3>)   jn["props"][k] = { val.x, val.y, val.z };
                    else if constexpr (std::is_same_v<T, glm::vec4>)   jn["props"][k] = { val.x, val.y, val.z, val.w };
                }, v);
            }
            j["nodes"].push_back(jn);
        }
        for (const auto& l : m_Links)
            j["links"].push_back({ l.Id, l.FromNode, l.FromPin, l.ToNode, l.ToPin });
        return j.dump(2);
    }

    void MaterialGraph::DeserializeJSON(const std::string& json)
    {
        m_Nodes.clear(); m_Links.clear();
        try {
            auto j = nlohmann::json::parse(json);
            m_NextId = j.value("nextId", 1u);
            for (const auto& jn : j.value("nodes", nlohmann::json::array()))
            {
                MatNode n;
                n.Id        = jn["id"];
                n.Type      = (MatNodeType)(int)jn["type"];
                n.EditorPos = { jn.value("posX", 0.0f), jn.value("posY", 0.0f) };
                PopulateNode(n);
                if (jn.contains("props"))
                    for (const auto& [k, v] : jn["props"].items())
                    {
                        if (v.is_number_float())  n.Props[k] = (f32)v;
                        else if (v.is_number_integer()) n.Props[k] = (i32)v;
                        else if (v.is_string())   n.Props[k] = v.get<std::string>();
                        else if (v.is_array() && v.size() == 3) n.Props[k] = glm::vec3{ v[0], v[1], v[2] };
                        else if (v.is_array() && v.size() == 4) n.Props[k] = glm::vec4{ v[0], v[1], v[2], v[3] };
                    }
                m_Nodes.push_back(std::move(n));
            }
            for (const auto& jl : j.value("links", nlohmann::json::array()))
                m_Links.push_back({ jl[0], jl[1], jl[2], jl[3], jl[4] });
        } catch (const std::exception& e) { FORCE_CORE_ERROR("MaterialGraph::DeserializeJSON: {}", e.what()); }
    }
}
