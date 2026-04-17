#pragma once
#include "Force/Core/Base.h"
#include "Force/Material/MaterialGraph.h"
#include "Force/Editor/NodeGraphEditor.h"
#include "Force/Editor/UndoRedo.h"
#include <string>

namespace Force
{
    class MaterialGraphPanel
    {
    public:
        MaterialGraphPanel() = default;

        void SetGraph(const Ref<MaterialGraph>& graph, const std::string& assetPath);
        void OnImGui();

        bool IsOpen()      const { return m_Open; }
        void Open()              { m_Open = true; }
        void Close()             { m_Open = false; }

    private:
        void DrawToolbar();
        void DrawNodeCanvas();
        void DrawNodeContextMenu();
        void DrawPropertiesPanel();

        void CompileAndSave();
        void AddNodeFromMenu(MatNodeType type, const glm::vec2& pos);
        void DeleteSelectedNodes();

        // Sync m_Graph pin connectivity to m_Editor state
        void RebuildPinHandles();

        Ref<MaterialGraph> m_Graph;
        NodeGraphEditor    m_Editor;
        std::string        m_AssetPath;
        bool               m_Open = false;

        // Per-node pin handles rebuilt each frame
        std::unordered_map<u32, std::vector<PinHandle>> m_InputPins;
        std::unordered_map<u32, std::vector<PinHandle>> m_OutputPins;

        MatCompileResult   m_LastCompile;
        bool               m_NeedsCompile  = false;
        bool               m_ShowAddMenu   = false;
        glm::vec2          m_AddMenuPos    = {};
        u32                m_SelectedNodeId = 0;
    };
}
