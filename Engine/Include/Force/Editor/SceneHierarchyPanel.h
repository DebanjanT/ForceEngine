#pragma once

#include "Force/Core/Base.h"
#include "Force/Core/ECS.h"

namespace Force
{
    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(Scene* scene);

        void SetContext(Scene* scene);
        void OnImGuiRender();

        Entity GetSelectedEntity() const { return m_SelectedEntity; }
        void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }

        bool IsEntitySelected() const { return m_SelectedEntity != NullEntity; }

    private:
        void DrawEntityNode(Entity entity);
        void DrawComponents(Entity entity);

        // Context menu actions
        void CreateEmptyEntity();
        void DuplicateEntity(Entity entity);
        void DeleteEntity(Entity entity);

        // Drag and drop for reparenting
        void HandleDragDrop(Entity entity);

    private:
        Scene* m_Context = nullptr;
        Entity m_SelectedEntity = NullEntity;

        // Multi-selection
        std::vector<Entity> m_SelectedEntities;
        bool m_AllowMultiSelect = true;

        // UI state
        bool m_RenamingEntity = false;
        char m_RenameBuffer[256] = {};

        // Filter
        char m_SearchFilter[256] = {};
        bool m_ShowInactive = true;
    };
}
