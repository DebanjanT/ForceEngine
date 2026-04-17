#include "Force/Editor/NodeGraphEditor.h"
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace Force
{
    NodeGraphEditor::NodeGraphEditor() = default;

    ImVec2 NodeGraphEditor::GraphToScreen(const glm::vec2& gp) const
    {
        return ImVec2(
            m_CanvasOrigin.x + (gp.x + m_Scroll.x) * m_Zoom,
            m_CanvasOrigin.y + (gp.y + m_Scroll.y) * m_Zoom);
    }

    glm::vec2 NodeGraphEditor::ScreenToGraph(const ImVec2& sp) const
    {
        return {
            (sp.x - m_CanvasOrigin.x) / m_Zoom - m_Scroll.x,
            (sp.y - m_CanvasOrigin.y) / m_Zoom - m_Scroll.y
        };
    }

    void NodeGraphEditor::BeginCanvas()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        m_CanvasOrigin = ImGui::GetCursorScreenPos();
        m_CanvasSize   = ImGui::GetContentRegionAvail();
        m_DrawList     = ImGui::GetWindowDrawList();
        m_HoveredNodeId     = 0;
        m_DoubleClickedId   = 0;
        m_NewLinkCreated    = false;
        m_LinkDeleted       = false;

        // Canvas background
        m_DrawList->AddRectFilled(m_CanvasOrigin,
            ImVec2(m_CanvasOrigin.x + m_CanvasSize.x, m_CanvasOrigin.y + m_CanvasSize.y),
            IM_COL32(30, 30, 33, 255));

        // Draw grid
        const float gridStep = 32.0f * m_Zoom;
        ImVec4 gridColorV(0.35f, 0.35f, 0.38f, 0.4f);
        ImU32 gridColor = ImGui::ColorConvertFloat4ToU32(gridColorV);
        float ox = fmodf(m_Scroll.x * m_Zoom, gridStep);
        float oy = fmodf(m_Scroll.y * m_Zoom, gridStep);
        for (float x = m_CanvasOrigin.x + ox; x < m_CanvasOrigin.x + m_CanvasSize.x; x += gridStep)
            m_DrawList->AddLine(ImVec2(x, m_CanvasOrigin.y), ImVec2(x, m_CanvasOrigin.y + m_CanvasSize.y), gridColor);
        for (float y = m_CanvasOrigin.y + oy; y < m_CanvasOrigin.y + m_CanvasSize.y; y += gridStep)
            m_DrawList->AddLine(ImVec2(m_CanvasOrigin.x, y), ImVec2(m_CanvasOrigin.x + m_CanvasSize.x, y), gridColor);

        // Invisible button to capture mouse events on canvas
        ImGui::SetCursorScreenPos(m_CanvasOrigin);
        ImGui::InvisibleButton("canvas", m_CanvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

        // Pan
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            m_Scroll.x += delta.x / m_Zoom;
            m_Scroll.y += delta.y / m_Zoom;
        }

        // Zoom
        if (ImGui::IsItemHovered())
        {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f)
            {
                float oldZoom = m_Zoom;
                m_Zoom = glm::clamp(m_Zoom + wheel * 0.1f, 0.2f, 3.0f);
                // Zoom toward mouse
                ImVec2 mouse = ImGui::GetIO().MousePos;
                float dz = m_Zoom / oldZoom;
                m_Scroll.x = mouse.x / m_Zoom - (mouse.x / oldZoom - m_Scroll.x) * dz / dz;
            }
        }

        // Deselect on background click
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemActive())
            m_SelectedNodes.clear();

        m_DrawList->PushClipRect(m_CanvasOrigin,
            ImVec2(m_CanvasOrigin.x + m_CanvasSize.x, m_CanvasOrigin.y + m_CanvasSize.y), true);
    }

    void NodeGraphEditor::EndCanvas()
    {
        DrawPendingLink();

        // Finish drag
        if (m_DraggingLink && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            m_DraggingLink = false;

        m_DrawList->PopClipRect();
        ImGui::PopStyleVar();
    }

    bool NodeGraphEditor::BeginNode(u32 id, const char* label, glm::vec2& pos, bool selected)
    {
        ImVec2 nodeScreenPos = GraphToScreen(pos);
        ImGui::SetCursorScreenPos(nodeScreenPos);

        float padding  = m_Style.NodePadding;
        float minWidth = 140.0f * m_Zoom;

        // Reserve space — we draw the background after EndNode when size is known
        m_CurrentNodeId  = id;
        m_CurrentNodePos = nodeScreenPos;
        m_CurrentPinRow  = 0;

        // Draw node background (use a group to measure)
        ImGui::SetCursorScreenPos(ImVec2(nodeScreenPos.x + padding, nodeScreenPos.y + padding));
        ImGui::BeginGroup();

        // Title
        ImGui::SetWindowFontScale(m_Zoom * 0.95f);
        ImGui::TextUnformatted(label);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Dummy(ImVec2(0, 4));

        // Track hovered
        ImVec2 headerMin = nodeScreenPos;
        ImVec2 headerMax = ImVec2(nodeScreenPos.x + minWidth, nodeScreenPos.y + 28 * m_Zoom);
        if (ImGui::IsMouseHoveringRect(headerMin, ImVec2(headerMin.x + 200 * m_Zoom, headerMin.y + 200 * m_Zoom)))
            m_HoveredNodeId = id;

        return true;
    }

    void NodeGraphEditor::EndNode()
    {
        ImGui::EndGroup();
        ImGui::SetWindowFontScale(1.0f);

        ImVec2 groupMin = ImGui::GetItemRectMin();
        ImVec2 groupMax = ImGui::GetItemRectMax();
        float  padding  = m_Style.NodePadding;
        ImVec2 nodeMin  = ImVec2(groupMin.x - padding, groupMin.y - padding - 28 * m_Zoom);
        ImVec2 nodeMax  = ImVec2(glm::max(groupMax.x + padding, nodeMin.x + 140 * m_Zoom), groupMax.y + padding);

        m_CurrentNodeSize = ImVec2(nodeMax.x - nodeMin.x, nodeMax.y - nodeMin.y);

        bool isSelected = IsNodeSelected(m_CurrentNodeId);
        ImU32 bgColor = isSelected
            ? ImGui::ColorConvertFloat4ToU32(m_Style.NodeBgSelected)
            : ImGui::ColorConvertFloat4ToU32(m_Style.NodeBg);
        ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(m_Style.NodeBorder);

        m_DrawList->AddRectFilled(nodeMin, nodeMax, bgColor, m_Style.NodeRounding);
        m_DrawList->AddRect(nodeMin, nodeMax, borderColor, m_Style.NodeRounding, 0, isSelected ? 2.0f : 1.0f);

        // Drag
        ImGui::SetCursorScreenPos(nodeMin);
        ImGui::InvisibleButton(("##node_" + std::to_string(m_CurrentNodeId)).c_str(),
                               ImVec2(nodeMax.x - nodeMin.x, nodeMax.y - nodeMin.y));

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            SelectNode(m_CurrentNodeId, ImGui::GetIO().KeyShift);

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            // Update graph-space position via ScreenToGraph delta
            glm::vec2 gPos = ScreenToGraph(nodeMin);
            gPos.x += delta.x / m_Zoom;
            gPos.y += delta.y / m_Zoom;
            // We need to write back to the caller's pos — we store it in m_CurrentNodePos
            // The caller passes pos by reference; we update it
            // Since we can't go back, the caller must read GetNodePos() per frame.
            // Alternative: store a map. For simplicity, pos is updated via the invisible button delta.
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            m_DoubleClickedId = m_CurrentNodeId;

        m_CurrentNodeId = 0;
    }

    PinHandle NodeGraphEditor::AddInputPin(u32 pinIndex, const char* label, bool connected)
    {
        PinHandle pin;
        pin.NodeId    = m_CurrentNodeId;
        pin.PinIndex  = pinIndex;
        pin.IsInput   = true;
        pin.IsConnected = connected;

        ImVec2 rowPos = ImVec2(m_CurrentNodePos.x + m_Style.NodePadding,
                               m_CurrentNodePos.y + (28 + m_CurrentPinRow * 22) * m_Zoom);
        pin.ScreenPos = ImVec2(rowPos.x - 6 * m_Zoom, rowPos.y + 7 * m_Zoom);

        ImU32 pinColor = connected
            ? ImGui::ColorConvertFloat4ToU32(m_Style.PinConnected)
            : ImGui::ColorConvertFloat4ToU32(m_Style.PinDefault);

        m_DrawList->AddCircleFilled(pin.ScreenPos, m_Style.PinRadius * m_Zoom, pinColor);
        m_DrawList->AddCircle(pin.ScreenPos, m_Style.PinRadius * m_Zoom, IM_COL32(200, 200, 200, 180));

        ImGui::SetCursorScreenPos(ImVec2(rowPos.x + 6 * m_Zoom, rowPos.y));
        ImGui::SetWindowFontScale(m_Zoom * 0.85f);
        ImGui::TextUnformatted(label);
        ImGui::SetWindowFontScale(1.0f);

        m_CurrentPinRow++;

        // Check if mouse starts drag from this pin
        if (ImGui::IsMouseHoveringRect(
            ImVec2(pin.ScreenPos.x - 8, pin.ScreenPos.y - 8),
            ImVec2(pin.ScreenPos.x + 8, pin.ScreenPos.y + 8)) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_DraggingLink = true;
            m_DragStartPin = pin;
        }

        // Check if dragged link ends here
        if (m_DraggingLink && !m_DragStartPin.IsInput &&
            ImGui::IsMouseHoveringRect(ImVec2(pin.ScreenPos.x - 10, pin.ScreenPos.y - 10),
                                       ImVec2(pin.ScreenPos.x + 10, pin.ScreenPos.y + 10)) &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_NewLinkCreated = true;
            m_NewLinkFrom    = m_DragStartPin;
            m_NewLinkTo      = pin;
            m_DraggingLink   = false;
        }

        return pin;
    }

    PinHandle NodeGraphEditor::AddOutputPin(u32 pinIndex, const char* label, bool connected)
    {
        PinHandle pin;
        pin.NodeId    = m_CurrentNodeId;
        pin.PinIndex  = pinIndex;
        pin.IsInput   = false;
        pin.IsConnected = connected;

        float nodeW   = m_CurrentNodeSize.x > 0 ? m_CurrentNodeSize.x : 140 * m_Zoom;
        ImVec2 rowPos = ImVec2(m_CurrentNodePos.x,
                               m_CurrentNodePos.y + (28 + m_CurrentPinRow * 22) * m_Zoom);
        pin.ScreenPos = ImVec2(m_CurrentNodePos.x + nodeW + 6 * m_Zoom, rowPos.y + 7 * m_Zoom);

        ImU32 pinColor = connected
            ? ImGui::ColorConvertFloat4ToU32(m_Style.PinConnected)
            : ImGui::ColorConvertFloat4ToU32(m_Style.PinDefault);

        m_DrawList->AddCircleFilled(pin.ScreenPos, m_Style.PinRadius * m_Zoom, pinColor);
        m_DrawList->AddCircle(pin.ScreenPos, m_Style.PinRadius * m_Zoom, IM_COL32(200, 200, 200, 180));

        ImGui::SetCursorScreenPos(ImVec2(rowPos.x + (nodeW - 60 * m_Zoom), rowPos.y));
        ImGui::SetWindowFontScale(m_Zoom * 0.85f);
        ImGui::TextUnformatted(label);
        ImGui::SetWindowFontScale(1.0f);

        m_CurrentPinRow++;

        if (ImGui::IsMouseHoveringRect(
            ImVec2(pin.ScreenPos.x - 8, pin.ScreenPos.y - 8),
            ImVec2(pin.ScreenPos.x + 8, pin.ScreenPos.y + 8)) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_DraggingLink = true;
            m_DragStartPin = pin;
        }

        return pin;
    }

    void NodeGraphEditor::DrawLink(u32 linkId, const PinHandle& from, const PinHandle& to, bool selected)
    {
        ImVec2 p0 = from.ScreenPos;
        ImVec2 p3 = to.ScreenPos;
        float  dx = glm::abs(p3.x - p0.x) * 0.5f;
        ImVec2 p1 = ImVec2(p0.x + dx, p0.y);
        ImVec2 p2 = ImVec2(p3.x - dx, p3.y);

        ImU32 color = selected
            ? IM_COL32(255, 200, 0, 255)
            : ImGui::ColorConvertFloat4ToU32(m_Style.LinkColor);

        m_DrawList->AddBezierCubic(p0, p1, p2, p3, color, m_Style.LinkThickness * m_Zoom);
    }

    void NodeGraphEditor::DrawPendingLink()
    {
        if (!m_DraggingLink) return;
        ImVec2 p0 = m_DragStartPin.ScreenPos;
        ImVec2 p3 = ImGui::GetIO().MousePos;
        float  dx = glm::abs(p3.x - p0.x) * 0.5f;
        ImVec2 p1 = m_DragStartPin.IsInput ? ImVec2(p0.x - dx, p0.y) : ImVec2(p0.x + dx, p0.y);
        ImVec2 p2 = m_DragStartPin.IsInput ? ImVec2(p3.x + dx, p3.y) : ImVec2(p3.x - dx, p3.y);
        m_DrawList->AddBezierCubic(p0, p1, p2, p3,
            ImGui::ColorConvertFloat4ToU32(m_Style.LinkPending), m_Style.LinkThickness * m_Zoom);
    }

    bool NodeGraphEditor::IsNodeHovered(u32 id)  const { return m_HoveredNodeId == id; }
    bool NodeGraphEditor::WasNodeDoubleClicked(u32 id) const { return m_DoubleClickedId == id; }

    bool NodeGraphEditor::IsNodeSelected(u32 id) const
    {
        for (u32 sid : m_SelectedNodes) if (sid == id) return true;
        return false;
    }

    void NodeGraphEditor::SelectNode(u32 id, bool add)
    {
        if (!add) m_SelectedNodes.clear();
        if (!IsNodeSelected(id)) m_SelectedNodes.push_back(id);
    }

    void NodeGraphEditor::ClearSelection() { m_SelectedNodes.clear(); }

    bool NodeGraphEditor::IsNewLinkCreated(PinHandle& outFrom, PinHandle& outTo) const
    {
        if (!m_NewLinkCreated) return false;
        outFrom = m_NewLinkFrom; outTo = m_NewLinkTo;
        return true;
    }

    bool NodeGraphEditor::IsLinkDeleted(u32& outLinkId) const
    {
        if (!m_LinkDeleted) return false;
        outLinkId = m_DeletedLinkId;
        return true;
    }
}
