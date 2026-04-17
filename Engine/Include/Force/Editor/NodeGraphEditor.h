#pragma once
#include "Force/Core/Base.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <functional>

namespace Force
{
    // Pure ImGui node graph renderer — no imnodes dependency.
    // Caller drives layout by calling BeginNode/EndNode/AddPin/DrawLink each frame.

    struct NodeGraphStyle
    {
        float  NodeRounding      = 6.0f;
        float  NodePadding       = 8.0f;
        float  PinRadius         = 6.0f;
        float  LinkThickness     = 2.5f;
        ImVec4 NodeBg            = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
        ImVec4 NodeBgSelected    = ImVec4(0.22f, 0.30f, 0.40f, 1.0f);
        ImVec4 NodeBorder        = ImVec4(0.40f, 0.40f, 0.45f, 1.0f);
        ImVec4 PinDefault        = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
        ImVec4 PinConnected      = ImVec4(0.30f, 0.80f, 0.30f, 1.0f);
        ImVec4 LinkColor         = ImVec4(0.55f, 0.80f, 0.30f, 1.0f);
        ImVec4 LinkPending       = ImVec4(0.80f, 0.80f, 0.20f, 0.8f);
    };

    // Pin handle used to draw links
    struct PinHandle
    {
        u32     NodeId;
        u32     PinIndex;
        bool    IsInput;
        ImVec2  ScreenPos;
        bool    IsConnected = false;
    };

    class NodeGraphEditor
    {
    public:
        NodeGraphEditor();

        void SetStyle(const NodeGraphStyle& style) { m_Style = style; }

        // Wrap your node-drawing code between these calls.
        // Renders a scrollable canvas with pan/zoom inside the current ImGui window.
        void BeginCanvas();
        void EndCanvas();

        // Call per node. Returns true while inside the node block.
        // pos is in graph space (canvas coords); will be updated if node is dragged.
        bool BeginNode(u32 id, const char* label, glm::vec2& pos, bool selected = false);
        void EndNode();

        // Call inside BeginNode/EndNode. Returns the PinHandle for link routing.
        PinHandle AddInputPin(u32 pinIndex, const char* label, bool connected = false);
        PinHandle AddOutputPin(u32 pinIndex, const char* label, bool connected = false);

        // Draw a link between two pins (call between BeginCanvas/EndCanvas).
        void DrawLink(u32 linkId, const PinHandle& from, const PinHandle& to,
                      bool selected = false);

        // Query interactions
        bool IsNodeHovered(u32 id) const;
        bool IsNodeSelected(u32 id) const;
        bool WasNodeDoubleClicked(u32 id) const;

        // Link creation: returns true when user completes a new link.
        bool IsNewLinkCreated(PinHandle& outFrom, PinHandle& outTo) const;
        bool IsLinkDeleted(u32& outLinkId) const;

        // Selection
        const std::vector<u32>& GetSelectedNodes() const { return m_SelectedNodes; }
        void ClearSelection();
        void SelectNode(u32 id, bool addToSelection = false);

        // Canvas state
        glm::vec2 GetScrollOffset()  const { return m_Scroll; }
        void      SetScrollOffset(const glm::vec2& s) { m_Scroll = s; }
        float     GetZoom()          const { return m_Zoom; }

        ImVec2    GraphToScreen(const glm::vec2& gp) const;
        glm::vec2 ScreenToGraph(const ImVec2& sp) const;

    private:
        void  DrawPendingLink();

        NodeGraphStyle     m_Style;
        ImDrawList*        m_DrawList = nullptr;
        ImVec2             m_CanvasOrigin{};
        ImVec2             m_CanvasSize{};

        glm::vec2          m_Scroll   = {};
        float              m_Zoom     = 1.0f;

        u32                m_ActiveNodeId  = 0; // being dragged
        u32                m_HoveredNodeId = 0;
        u32                m_DoubleClickedId = 0;

        std::vector<u32>   m_SelectedNodes;

        // Pending link drag
        bool               m_DraggingLink = false;
        PinHandle          m_DragStartPin{};
        ImVec2             m_DragEndPos{};

        // Completed link this frame
        mutable bool       m_NewLinkCreated = false;
        mutable PinHandle  m_NewLinkFrom{}, m_NewLinkTo{};

        // Link deletion
        mutable bool       m_LinkDeleted   = false;
        mutable u32        m_DeletedLinkId = 0;

        // Current node being built
        u32                m_CurrentNodeId = 0;
        ImVec2             m_CurrentNodePos{};
        ImVec2             m_CurrentNodeSize{};
        u32                m_CurrentPinRow = 0;
    };
}
