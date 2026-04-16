#include "Force/Editor/SceneHierarchyPanel.h"
#include "Force/Scene/Components.h"
#include "Force/Core/Logger.h"
#include <imgui.h>
#include <cstring>

namespace Force
{
    SceneHierarchyPanel::SceneHierarchyPanel(Scene* scene)
        : m_Context(scene)
    {
    }

    void SceneHierarchyPanel::SetContext(Scene* scene)
    {
        m_Context = scene;
        m_SelectedEntity = NullEntity;
        m_SelectedEntities.clear();
    }

    void SceneHierarchyPanel::OnImGuiRender()
    {
        if (!m_Context)
            return;

        ImGui::Begin("Scene Hierarchy");

        // Search filter
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##Search", "Search entities...", m_SearchFilter, sizeof(m_SearchFilter)))
        {
            // Filter will be applied in DrawEntityNode
        }

        ImGui::Separator();

        // Context menu for empty space
        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                CreateEmptyEntity();
            }
            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Camera"))
                {
                    auto entity = m_Context->CreateEntity("Camera");
                    entity.AddComponent<CameraComponent>();
                }
                if (ImGui::MenuItem("Directional Light"))
                {
                    auto entity = m_Context->CreateEntity("Directional Light");
                    entity.AddComponent<DirectionalLightComponent>();
                }
                if (ImGui::MenuItem("Point Light"))
                {
                    auto entity = m_Context->CreateEntity("Point Light");
                    entity.AddComponent<PointLightComponent>();
                }
                if (ImGui::MenuItem("Spot Light"))
                {
                    auto entity = m_Context->CreateEntity("Spot Light");
                    entity.AddComponent<SpotLightComponent>();
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        // Draw all entities
        auto& registry = m_Context->GetRegistry();
        auto view = registry.view<TagComponent>();

        for (auto entityId : view)
        {
            Entity entity = entityId;

            // Skip children (they'll be drawn under their parent)
            if (registry.all_of<RelationshipComponent>(entity))
            {
                auto& rel = registry.get<RelationshipComponent>(entity);
                if (rel.Parent != NullEntity)
                    continue;
            }

            DrawEntityNode(entity);
        }

        // Click on empty space to deselect
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        {
            m_SelectedEntity = NullEntity;
        }

        ImGui::End();

        // Properties panel
        ImGui::Begin("Properties");
        if (m_SelectedEntity != NullEntity)
        {
            DrawComponents(m_SelectedEntity);
        }
        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        auto& registry = m_Context->GetRegistry();
        auto& tag = registry.get<TagComponent>(entity);

        // Apply search filter
        if (strlen(m_SearchFilter) > 0)
        {
            if (tag.Tag.find(m_SearchFilter) == std::string::npos)
                return;
        }

        // Check if entity is active
        bool isActive = true;
        if (registry.all_of<ActiveComponent>(entity))
        {
            isActive = registry.get<ActiveComponent>(entity).Active;
        }

        if (!m_ShowInactive && !isActive)
            return;

        // Build flags
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                    ImGuiTreeNodeFlags_SpanAvailWidth;

        if (m_SelectedEntity == entity)
            flags |= ImGuiTreeNodeFlags_Selected;

        // Check for children
        bool hasChildren = false;
        if (registry.all_of<RelationshipComponent>(entity))
        {
            auto& rel = registry.get<RelationshipComponent>(entity);
            hasChildren = !rel.Children.empty();
        }

        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf;

        // Gray out inactive entities
        if (!isActive)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        // Tree node
        void* ptrId = reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<u32>(entity)));
        bool opened = ImGui::TreeNodeEx(ptrId, flags, "%s", tag.Tag.c_str());

        if (!isActive)
            ImGui::PopStyleColor();

        // Selection
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            m_SelectedEntity = entity;
        }

        // Context menu
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Duplicate"))
            {
                DuplicateEntity(entity);
            }
            if (ImGui::MenuItem("Delete"))
            {
                DeleteEntity(entity);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Create Child"))
            {
                auto child = m_Context->CreateEntity("Child");
                // TODO: Set parent
            }
            ImGui::EndPopup();
        }

        // Drag and drop
        HandleDragDrop(entity);

        // Draw children
        if (opened)
        {
            if (registry.all_of<RelationshipComponent>(entity))
            {
                auto& rel = registry.get<RelationshipComponent>(entity);
                for (Entity child : rel.Children)
                {
                    DrawEntityNode(child);
                }
            }
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::DrawComponents(Entity entity)
    {
        auto& registry = m_Context->GetRegistry();

        // Tag component (always present)
        if (registry.all_of<TagComponent>(entity))
        {
            auto& tag = registry.get<TagComponent>(entity);

            char buffer[256];
            std::strncpy(buffer, tag.Tag.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0';

            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            {
                tag.Tag = std::string(buffer);
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            if (!registry.all_of<CameraComponent>(entity))
            {
                if (ImGui::MenuItem("Camera"))
                {
                    registry.emplace<CameraComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!registry.all_of<StaticMeshComponent>(entity))
            {
                if (ImGui::MenuItem("Static Mesh"))
                {
                    registry.emplace<StaticMeshComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!registry.all_of<DirectionalLightComponent>(entity))
            {
                if (ImGui::MenuItem("Directional Light"))
                {
                    registry.emplace<DirectionalLightComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!registry.all_of<PointLightComponent>(entity))
            {
                if (ImGui::MenuItem("Point Light"))
                {
                    registry.emplace<PointLightComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!registry.all_of<SpotLightComponent>(entity))
            {
                if (ImGui::MenuItem("Spot Light"))
                {
                    registry.emplace<SpotLightComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!registry.all_of<RigidBodyComponent>(entity))
            {
                if (ImGui::MenuItem("Rigid Body"))
                {
                    registry.emplace<RigidBodyComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!registry.all_of<BoxColliderComponent>(entity))
            {
                if (ImGui::MenuItem("Box Collider"))
                {
                    registry.emplace<BoxColliderComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();

        ImGui::Separator();

        // Transform component
        if (registry.all_of<TransformComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& transform = registry.get<TransformComponent>(entity);

                ImGui::DragFloat3("Position", &transform.Position.x, 0.1f);

                glm::vec3 rotationDeg = glm::degrees(transform.Rotation);
                if (ImGui::DragFloat3("Rotation", &rotationDeg.x, 1.0f))
                {
                    transform.Rotation = glm::radians(rotationDeg);
                }

                ImGui::DragFloat3("Scale", &transform.Scale.x, 0.1f, 0.01f, 100.0f);
            }
        }

        // Camera component
        if (registry.all_of<CameraComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& camera = registry.get<CameraComponent>(entity);

                const char* projectionTypes[] = { "Perspective", "Orthographic" };
                int currentProj = static_cast<int>(camera.Projection);
                if (ImGui::Combo("Projection", &currentProj, projectionTypes, 2))
                {
                    camera.Projection = static_cast<CameraComponent::ProjectionType>(currentProj);
                }

                if (camera.Projection == CameraComponent::ProjectionType::Perspective)
                {
                    ImGui::DragFloat("FOV", &camera.FOV, 1.0f, 1.0f, 179.0f);
                }
                else
                {
                    ImGui::DragFloat("Ortho Size", &camera.OrthoSize, 0.1f, 0.1f, 100.0f);
                }

                ImGui::DragFloat("Near Clip", &camera.NearClip, 0.01f, 0.001f, camera.FarClip - 0.01f);
                ImGui::DragFloat("Far Clip", &camera.FarClip, 1.0f, camera.NearClip + 0.01f, 10000.0f);

                ImGui::Checkbox("Primary", &camera.Primary);
            }
        }

        // Static Mesh component
        if (registry.all_of<StaticMeshComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Static Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& mesh = registry.get<StaticMeshComponent>(entity);

                ImGui::Text("Mesh: %s", mesh.MeshHandle.IsValid() ? "Loaded" : "None");
                // TODO: Add mesh asset picker

                ImGui::Checkbox("Visible", &mesh.Visible);
                ImGui::Checkbox("Cast Shadow", &mesh.CastShadow);
                ImGui::Checkbox("Receive Shadow", &mesh.ReceiveShadow);

                // Remove component button
                if (ImGui::Button("Remove##StaticMesh"))
                {
                    registry.remove<StaticMeshComponent>(entity);
                }
            }
        }

        // Directional Light component
        if (registry.all_of<DirectionalLightComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& light = registry.get<DirectionalLightComponent>(entity);

                ImGui::ColorEdit3("Color", &light.Color.x);
                ImGui::DragFloat("Intensity", &light.Intensity, 0.1f, 0.0f, 100.0f);
                ImGui::Checkbox("Cast Shadows", &light.CastShadows);

                if (ImGui::Button("Remove##DirLight"))
                {
                    registry.remove<DirectionalLightComponent>(entity);
                }
            }
        }

        // Point Light component
        if (registry.all_of<PointLightComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& light = registry.get<PointLightComponent>(entity);

                ImGui::ColorEdit3("Color", &light.Color.x);
                ImGui::DragFloat("Intensity", &light.Intensity, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Radius", &light.Radius, 0.1f, 0.1f, 1000.0f);
                ImGui::DragFloat("Falloff", &light.FalloffExponent, 0.1f, 0.1f, 10.0f);
                ImGui::Checkbox("Cast Shadows", &light.CastShadows);

                if (ImGui::Button("Remove##PointLight"))
                {
                    registry.remove<PointLightComponent>(entity);
                }
            }
        }

        // Spot Light component
        if (registry.all_of<SpotLightComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& light = registry.get<SpotLightComponent>(entity);

                ImGui::ColorEdit3("Color", &light.Color.x);
                ImGui::DragFloat("Intensity", &light.Intensity, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Range", &light.Range, 0.1f, 0.1f, 1000.0f);
                ImGui::DragFloat("Inner Angle", &light.InnerConeAngle, 1.0f, 1.0f, light.OuterConeAngle - 1.0f);
                ImGui::DragFloat("Outer Angle", &light.OuterConeAngle, 1.0f, light.InnerConeAngle + 1.0f, 89.0f);
                ImGui::Checkbox("Cast Shadows", &light.CastShadows);

                if (ImGui::Button("Remove##SpotLight"))
                {
                    registry.remove<SpotLightComponent>(entity);
                }
            }
        }

        // Rigid Body component
        if (registry.all_of<RigidBodyComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Rigid Body", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& rb = registry.get<RigidBodyComponent>(entity);

                const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
                int currentType = static_cast<int>(rb.Type);
                if (ImGui::Combo("Body Type", &currentType, bodyTypes, 3))
                {
                    rb.Type = static_cast<RigidBodyComponent::BodyType>(currentType);
                }

                if (rb.Type == RigidBodyComponent::BodyType::Dynamic)
                {
                    ImGui::DragFloat("Mass", &rb.Mass, 0.1f, 0.001f, 10000.0f);
                    ImGui::DragFloat("Linear Damping", &rb.LinearDamping, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("Angular Damping", &rb.AngularDamping, 0.01f, 0.0f, 1.0f);
                    ImGui::Checkbox("Use Gravity", &rb.UseGravity);
                }

                if (ImGui::Button("Remove##RigidBody"))
                {
                    registry.remove<RigidBodyComponent>(entity);
                }
            }
        }

        // Box Collider component
        if (registry.all_of<BoxColliderComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Box Collider", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& collider = registry.get<BoxColliderComponent>(entity);

                ImGui::DragFloat3("Size", &collider.Size.x, 0.1f, 0.01f, 1000.0f);
                ImGui::DragFloat3("Offset", &collider.Offset.x, 0.1f);
                ImGui::Checkbox("Is Trigger", &collider.IsTrigger);

                if (ImGui::Button("Remove##BoxCollider"))
                {
                    registry.remove<BoxColliderComponent>(entity);
                }
            }
        }
    }

    void SceneHierarchyPanel::CreateEmptyEntity()
    {
        m_SelectedEntity = m_Context->CreateEntity("Empty Entity").GetEntity();
    }

    void SceneHierarchyPanel::DuplicateEntity(Entity entity)
    {
        auto& registry = m_Context->GetRegistry();

        if (!registry.all_of<TagComponent>(entity))
            return;

        auto& sourceTag = registry.get<TagComponent>(entity);
        auto newEntity = m_Context->CreateEntity(sourceTag.Tag + " (Copy)");

        // Copy TransformComponent
        if (registry.all_of<TransformComponent>(entity))
        {
            auto& source = registry.get<TransformComponent>(entity);
            newEntity.AddComponent<TransformComponent>(source);
        }

        // Copy other components as needed
        // TODO: Add more component copying

        m_SelectedEntity = newEntity.GetEntity();
    }

    void SceneHierarchyPanel::DeleteEntity(Entity entity)
    {
        if (m_SelectedEntity == entity)
        {
            m_SelectedEntity = NullEntity;
        }

        EntityHandle handle(entity, m_Context);
        m_Context->DestroyEntity(handle);
    }

    void SceneHierarchyPanel::HandleDragDrop(Entity entity)
    {
        // Drag source
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("SCENE_ENTITY", &entity, sizeof(Entity));

            auto& registry = m_Context->GetRegistry();
            auto& tag = registry.get<TagComponent>(entity);
            ImGui::Text("%s", tag.Tag.c_str());

            ImGui::EndDragDropSource();
        }

        // Drop target (for reparenting)
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
            {
                Entity droppedEntity = *static_cast<Entity*>(payload->Data);

                if (droppedEntity != entity)
                {
                    // TODO: Implement reparenting
                    // SetParent(droppedEntity, entity);
                    FORCE_CORE_INFO("Reparenting entity (not yet implemented)");
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
}
