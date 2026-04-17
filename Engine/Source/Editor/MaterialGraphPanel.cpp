#include "Force/Editor/MaterialGraphPanel.h"
#include "Force/Editor/UndoRedo.h"
#include "Force/Editor/Widgets.h"
#include "Force/Core/Logger.h"
#include <imgui.h>
#include <fstream>
#include <sstream>

namespace Force
{
    void MaterialGraphPanel::SetGraph(const Ref<MaterialGraph>& graph, const std::string& assetPath)
    {
        m_Graph     = graph;
        m_AssetPath = assetPath;
        m_Open      = true;
    }

    void MaterialGraphPanel::OnImGui()
    {
        if (!m_Open || !m_Graph) return;

        ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Material Graph", &m_Open, ImGuiWindowFlags_MenuBar))
        { ImGui::End(); return; }

        DrawToolbar();
        ImGui::Separator();

        // Left: canvas | Right: properties
        float panelW = 220.0f;
        float canvasW = ImGui::GetContentRegionAvail().x - panelW - 4.0f;

        ImGui::BeginChild("##canvas", ImVec2(canvasW, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        DrawNodeCanvas();
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("##props", ImVec2(panelW, 0), true);
        DrawPropertiesPanel();
        ImGui::EndChild();

        // Handle compile
        if (m_NeedsCompile)
        {
            CompileAndSave();
            m_NeedsCompile = false;
        }

        ImGui::End();
    }

    void MaterialGraphPanel::DrawToolbar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::MenuItem("Save & Compile", "Ctrl+S")) m_NeedsCompile = true;
            if (ImGui::MenuItem("Add Node"))                 { m_ShowAddMenu = true; m_AddMenuPos = { ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y }; }
            if (ImGui::MenuItem("Delete Selected"))          DeleteSelectedNodes();
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Reset View")) { m_Editor.SetScrollOffset({}); }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Keyboard shortcuts
        if (ImGui::IsWindowFocused() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) m_NeedsCompile = true;
        if (ImGui::IsWindowFocused() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) UndoRedoStack::Get().Undo();
        if (ImGui::IsWindowFocused() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y)) UndoRedoStack::Get().Redo();
        if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete))                 DeleteSelectedNodes();

        if (!m_LastCompile.Success && !m_LastCompile.Error.empty())
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0.3f,0.3f,1));
            ImGui::TextUnformatted(m_LastCompile.Error.c_str());
            ImGui::PopStyleColor();
        }
        else if (m_LastCompile.Success)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f,1,0.3f,1));
            ImGui::TextUnformatted("Compiled OK");
            ImGui::PopStyleColor();
        }
    }

    void MaterialGraphPanel::DrawNodeCanvas()
    {
        m_Editor.BeginCanvas();

        auto& nodes = m_Graph->GetNodes();
        auto& links = m_Graph->GetLinks();

        // Build pin handle maps this frame
        m_InputPins.clear();
        m_OutputPins.clear();

        for (const auto& node : nodes)
        {
            // Check which pins have connections
            auto hasInputLink  = [&](u32 pin) {
                for (const auto& l : links) if (l.ToNode == node.Id && l.ToPin == pin) return true; return false;
            };
            auto hasOutputLink = [&](u32 pin) {
                for (const auto& l : links) if (l.FromNode == node.Id && l.FromPin == pin) return true; return false;
            };

            glm::vec2 pos = node.EditorPos;
            bool selected = m_Editor.IsNodeSelected(node.Id);

            if (m_Editor.BeginNode(node.Id, node.Label.c_str(), pos, selected))
            {
                for (u32 i = 0; i < (u32)node.Inputs.size(); i++)
                    m_InputPins[node.Id].push_back(m_Editor.AddInputPin(i, node.Inputs[i].Name.c_str(), hasInputLink(i)));
                for (u32 i = 0; i < (u32)node.Outputs.size(); i++)
                    m_OutputPins[node.Id].push_back(m_Editor.AddOutputPin(i, node.Outputs[i].Name.c_str(), hasOutputLink(i)));

                m_Editor.EndNode();

                // If node was moved, update graph
                if (pos.x != node.EditorPos.x || pos.y != node.EditorPos.y)
                {
                    MatNode* n = m_Graph->GetNode(node.Id);
                    if (n) n->EditorPos = pos;
                }
            }
        }

        // Draw links
        for (const auto& link : links)
        {
            auto fromIt = m_OutputPins.find(link.FromNode);
            auto toIt   = m_InputPins.find(link.ToNode);
            if (fromIt == m_OutputPins.end() || toIt == m_InputPins.end()) continue;
            if (link.FromPin >= fromIt->second.size() || link.ToPin >= toIt->second.size()) continue;
            m_Editor.DrawLink(link.Id, fromIt->second[link.FromPin], toIt->second[link.ToPin]);
        }

        // Handle new link
        PinHandle from, to;
        if (m_Editor.IsNewLinkCreated(from, to))
        {
            // Ensure from=output, to=input
            if (from.IsInput && !to.IsInput) std::swap(from, to);
            if (!from.IsInput && to.IsInput && from.NodeId != to.NodeId)
            {
                u32 fromNode = from.NodeId, fromPin = from.PinIndex;
                u32 toNode   = to.NodeId,   toPin   = to.PinIndex;
                auto cmd = std::make_unique<LambdaCommand>("Add Link",
                    [this, fromNode, fromPin, toNode, toPin]{ m_Graph->AddLink(fromNode, fromPin, toNode, toPin); },
                    [this, toNode, toPin]{
                        for (const auto& l : m_Graph->GetLinks())
                            if (l.ToNode == toNode && l.ToPin == toPin) { m_Graph->RemoveLink(l.Id); break; }
                    });
                UndoRedoStack::Get().Execute(std::move(cmd));
            }
        }

        // Handle link delete
        u32 delLinkId = 0;
        if (m_Editor.IsLinkDeleted(delLinkId))
        {
            auto cmd = std::make_unique<LambdaCommand>("Delete Link",
                [this, delLinkId]{ m_Graph->RemoveLink(delLinkId); },
                [](){ /* re-add not supported in simple undo */ });
            UndoRedoStack::Get().Execute(std::move(cmd));
        }

        // Context menu: right-click to add node
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            m_ShowAddMenu = true;
            m_AddMenuPos = { ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y };
            ImGui::OpenPopup("##addnode");
        }
        DrawNodeContextMenu();

        m_Editor.EndCanvas();
    }

    void MaterialGraphPanel::DrawNodeContextMenu()
    {
        if (ImGui::BeginPopup("##addnode"))
        {
            glm::vec2 graphPos = m_Editor.ScreenToGraph(ImVec2(m_AddMenuPos.x, m_AddMenuPos.y));

            auto addItem = [&](const char* label, MatNodeType type) {
                if (ImGui::MenuItem(label)) AddNodeFromMenu(type, graphPos);
            };
            if (ImGui::BeginMenu("Inputs"))
            {
                addItem("Texture Sample",  MatNodeType::TextureSample);
                addItem("Scalar",          MatNodeType::Constant1);
                addItem("Vector3",         MatNodeType::Constant3);
                addItem("Vector4",         MatNodeType::Constant4);
                addItem("TexCoord",        MatNodeType::TexCoord);
                addItem("Vertex Normal",   MatNodeType::VertexNormal);
                addItem("Time",            MatNodeType::Time);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Math"))
            {
                addItem("Multiply",  MatNodeType::Multiply);
                addItem("Add",       MatNodeType::Add);
                addItem("Subtract",  MatNodeType::Subtract);
                addItem("Lerp",      MatNodeType::Lerp);
                addItem("Clamp",     MatNodeType::Clamp);
                addItem("Power",     MatNodeType::Power);
                addItem("1 - x",     MatNodeType::OneMinus);
                addItem("Append",    MatNodeType::Append);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Utility"))
            {
                addItem("Fresnel",    MatNodeType::Fresnel);
                addItem("Normal Map", MatNodeType::NormalMap);
                ImGui::EndMenu();
            }
            addItem("PBR Output", MatNodeType::PBROutput);
            ImGui::EndPopup();
        }
    }

    void MaterialGraphPanel::DrawPropertiesPanel()
    {
        ImGui::TextUnformatted("Properties");
        ImGui::Separator();

        const auto& selected = m_Editor.GetSelectedNodes();
        if (selected.empty()) { ImGui::TextDisabled("Select a node"); return; }

        MatNode* node = m_Graph->GetNode(selected.front());
        if (!node) return;

        ImGui::Text("Node: %s (ID %u)", node->Label.c_str(), node->Id);
        ImGui::Separator();

        for (auto& [key, val] : node->Props)
        {
            ImGui::PushID(key.c_str());
            bool changed = false;
            std::visit([&](auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, f32>)
                    changed = ImGui::DragFloat(key.c_str(), &v, 0.01f);
                else if constexpr (std::is_same_v<T, i32>)
                    changed = ImGui::DragInt(key.c_str(), &v);
                else if constexpr (std::is_same_v<T, glm::vec3>)
                    changed = ImGui::ColorEdit3(key.c_str(), &v.x);
                else if constexpr (std::is_same_v<T, glm::vec4>)
                    changed = ImGui::ColorEdit4(key.c_str(), &v.x);
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    char buf[256]; strncpy_s(buf, sizeof(buf), v.c_str(), sizeof(buf)-1);
                    if (ImGui::InputText(key.c_str(), buf, sizeof(buf))) { v = buf; changed = true; }
                }
            }, val);
            ImGui::PopID();
        }
    }

    void MaterialGraphPanel::AddNodeFromMenu(MatNodeType type, const glm::vec2& pos)
    {
        auto cmd = std::make_unique<LambdaCommand>("Add Node",
            [this, type, pos]{ m_Graph->AddNode(type, pos); },
            [this]{ /* remove last node — simplified */ });
        UndoRedoStack::Get().Execute(std::move(cmd));
        m_ShowAddMenu = false;
    }

    void MaterialGraphPanel::DeleteSelectedNodes()
    {
        for (u32 id : m_Editor.GetSelectedNodes())
        {
            u32 capturedId = id;
            auto cmd = std::make_unique<LambdaCommand>("Delete Node",
                [this, capturedId]{ m_Graph->RemoveNode(capturedId); },
                [](){ /* restore not supported in simple undo */ });
            UndoRedoStack::Get().Execute(std::move(cmd));
        }
        m_Editor.ClearSelection();
    }

    void MaterialGraphPanel::CompileAndSave()
    {
        if (!m_Graph || m_AssetPath.empty()) return;

        // Save JSON
        std::string json = m_Graph->SerializeJSON();
        { std::ofstream f(m_AssetPath); f << json; }

        // Compile SPIR-V
        std::string spvPath = m_AssetPath + ".frag.spv";
        m_LastCompile = m_Graph->Compile(spvPath);

        if (m_LastCompile.Success)
            FORCE_CORE_INFO("MaterialGraphPanel: saved & compiled '{}'", m_AssetPath);
        else
            FORCE_CORE_ERROR("MaterialGraphPanel: compile failed: {}", m_LastCompile.Error);
    }
}
