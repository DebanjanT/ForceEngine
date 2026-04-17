#include "Force/Editor/PropertiesPanel.h"
#include "Force/Editor/Widgets.h"
#include "Force/Scene/Components.h"
#include "Force/Core/ECS.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

namespace Force
{
    // ─── helpers ────────────────────────────────────────────────────────────────

    static bool DrawVec3(const char* label, glm::vec3& v, float reset = 0.f, float speed = 0.1f)
    {
        bool changed = false;
        ImGui::PushID(label);

        float lineH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
        ImVec2 btnSz{ lineH + 3.f, lineH };

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 80.f);
        ImGui::Text("%s", label);
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());

        struct Axis { const char* id; const char* btn; ImVec4 col; float* val; };
        Axis axes[3] = {
            {"##X","X",{0.8f,0.15f,0.15f,1.f}, &v.x},
            {"##Y","Y",{0.15f,0.7f,0.15f,1.f}, &v.y},
            {"##Z","Z",{0.15f,0.45f,0.85f,1.f},&v.z}
        };

        for (int i = 0; i < 3; i++)
        {
            if (i > 0) ImGui::SameLine(0, 0);
            ImGui::PushStyleColor(ImGuiCol_Button,        axes[i].col);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, axes[i].col);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  axes[i].col);
            if (ImGui::Button(axes[i].btn, btnSz)) { *axes[i].val = reset; changed = true; }
            ImGui::PopStyleColor(3);
            ImGui::SameLine(0, 0);
            if (ImGui::DragFloat(axes[i].id, axes[i].val, speed)) changed = true;
            ImGui::PopItemWidth();
        }

        ImGui::Columns(1);
        ImGui::PopID();
        return changed;
    }

    template<typename T, typename Fn>
    void PropertiesPanel::DrawComponent(const char* label, Fn fn, bool removable)
    {
        if (!m_Scene) return;
        auto& reg = m_Scene->GetRegistry();
        if (!reg.all_of<T>(m_Entity)) return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        float lineH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
        bool  open  = ImGui::TreeNodeEx((void*)(uintptr_t)typeid(T).hash_code(), flags, "%s", label);
        ImGui::PopStyleVar();

        bool removed = false;
        if (removable)
        {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - lineH * 0.5f);
            if (ImGui::Button("...", ImVec2{ lineH, lineH }))
                ImGui::OpenPopup("ComponentSettings");
            if (ImGui::BeginPopup("ComponentSettings"))
            {
                if (ImGui::MenuItem("Remove Component")) removed = true;
                ImGui::EndPopup();
            }
        }

        if (open)
        {
            auto& comp = reg.get<T>(m_Entity);
            fn(comp);
            ImGui::TreePop();
        }
        if (removed) reg.remove<T>(m_Entity);
    }

    // ─── main draw ──────────────────────────────────────────────────────────────

    void PropertiesPanel::OnImGui()
    {
        if (!m_Open) return;
        ImGui::Begin("Details", &m_Open);

        if (m_Entity == entt::null || !m_Scene)
        {
            ImGui::TextDisabled("No entity selected");
            ImGui::End();
            return;
        }

        DrawEntityHeader();
        ImGui::Separator();

        DrawTransform();
        DrawStaticMesh();
        DrawSkeletalMesh();
        DrawCamera();
        DrawDirectionalLight();
        DrawPointLight();
        DrawSpotLight();
        DrawRigidBody();
        DrawBoxCollider();
        DrawSphereCollider();
        DrawCapsuleCollider();
        DrawAudioSource();
        DrawScript();

        ImGui::Spacing();
        float bw = 180.f;
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - bw) * 0.5f);
        if (Widgets::PrimaryButton("+ Add Component", ImVec2(bw, 0)))
            ImGui::OpenPopup("AddComponent");
        AddComponentPopup();

        ImGui::End();
    }

    void PropertiesPanel::DrawEntityHeader()
    {
        auto& reg = m_Scene->GetRegistry();

        // Tag (name)
        if (reg.all_of<TagComponent>(m_Entity))
        {
            auto& tag = reg.get<TagComponent>(m_Entity);
            char buf[256];
            strncpy_s(buf, sizeof(buf), tag.Tag.c_str(), sizeof(buf) - 1);
            buf[255] = 0;
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##Name", buf, sizeof(buf)))
                tag.Tag = buf;
        }

        // Active toggle
        if (reg.all_of<ActiveComponent>(m_Entity))
        {
            auto& active = reg.get<ActiveComponent>(m_Entity);
            ImGui::SameLine();
            ImGui::Checkbox("Active", &active.Active);
        }
    }

    void PropertiesPanel::DrawTransform()
    {
        DrawComponent<TransformComponent>("Transform", [](TransformComponent& t)
        {
            DrawVec3("Position", t.Position, 0.f, 0.1f);
            glm::vec3 deg = glm::degrees(t.Rotation);
            if (DrawVec3("Rotation", deg, 0.f, 0.5f))
                t.Rotation = glm::radians(deg);
            DrawVec3("Scale",    t.Scale,    1.f, 0.01f);
        }, false);
    }

    void PropertiesPanel::DrawStaticMesh()
    {
        DrawComponent<StaticMeshComponent>("Static Mesh", [](StaticMeshComponent& c)
        {
            ImGui::Text("Mesh: %llx-%llx", c.MeshHandle.High, c.MeshHandle.Low);
            ImGui::Checkbox("Visible",       &c.Visible);
            ImGui::Checkbox("Cast Shadow",   &c.CastShadow);
            ImGui::Checkbox("Recv Shadow",   &c.ReceiveShadow);
        });
    }

    void PropertiesPanel::DrawSkeletalMesh()
    {
        DrawComponent<SkeletalMeshComponent>("Skeletal Mesh", [](SkeletalMeshComponent& c)
        {
            ImGui::Text("Mesh: %llx-%llx", c.MeshHandle.High, c.MeshHandle.Low);
            ImGui::Text("Skeleton: %llx-%llx", c.SkeletonHandle.High, c.SkeletonHandle.Low);
            ImGui::Checkbox("Visible",   &c.Visible);
            ImGui::DragFloat("Anim Speed", &c.AnimationSpeed, 0.01f, 0.f, 10.f);
            ImGui::Checkbox("Loop",      &c.AnimationLooping);
        });
    }

    void PropertiesPanel::DrawCamera()
    {
        DrawComponent<CameraComponent>("Camera", [](CameraComponent& c)
        {
            const char* projTypes[] = { "Perspective", "Orthographic" };
            int pt = (int)c.Projection;
            if (ImGui::Combo("Projection", &pt, projTypes, 2))
                c.Projection = (CameraComponent::ProjectionType)pt;

            if (c.Projection == CameraComponent::ProjectionType::Perspective)
                ImGui::DragFloat("FOV",  &c.FOV,  0.5f, 10.f, 170.f);
            else
                ImGui::DragFloat("Size", &c.OrthoSize, 0.1f, 0.1f, 1000.f);

            ImGui::DragFloat("Near Clip", &c.NearClip, 0.01f, 0.001f, 10.f);
            ImGui::DragFloat("Far Clip",  &c.FarClip,  1.f, 1.f, 100000.f);
            ImGui::Checkbox("Primary",   &c.Primary);
        });
    }

    void PropertiesPanel::DrawDirectionalLight()
    {
        DrawComponent<DirectionalLightComponent>("Directional Light", [](DirectionalLightComponent& c)
        {
            ImGui::ColorEdit3("Color",     &c.Color.x);
            ImGui::DragFloat("Intensity",  &c.Intensity, 0.01f, 0.f, 100.f);
            ImGui::Checkbox("Cast Shadows", &c.CastShadows);
        });
    }

    void PropertiesPanel::DrawPointLight()
    {
        DrawComponent<PointLightComponent>("Point Light", [](PointLightComponent& c)
        {
            ImGui::ColorEdit3("Color",     &c.Color.x);
            ImGui::DragFloat("Intensity",  &c.Intensity,        0.01f, 0.f, 1000.f);
            ImGui::DragFloat("Radius",     &c.Radius,           0.1f,  0.f, 10000.f);
            ImGui::DragFloat("Falloff",    &c.FalloffExponent,  0.1f,  0.f, 8.f);
            ImGui::Checkbox("Cast Shadows",&c.CastShadows);
        });
    }

    void PropertiesPanel::DrawSpotLight()
    {
        DrawComponent<SpotLightComponent>("Spot Light", [](SpotLightComponent& c)
        {
            ImGui::ColorEdit3("Color",      &c.Color.x);
            ImGui::DragFloat("Intensity",   &c.Intensity,       0.01f, 0.f, 1000.f);
            ImGui::DragFloat("Range",       &c.Range,           0.1f,  0.f, 10000.f);
            ImGui::DragFloat("Inner Angle", &c.InnerConeAngle,  0.5f,  0.f, 89.f);
            ImGui::DragFloat("Outer Angle", &c.OuterConeAngle,  0.5f,  0.f, 89.f);
            ImGui::Checkbox("Cast Shadows", &c.CastShadows);
        });
    }

    void PropertiesPanel::DrawRigidBody()
    {
        DrawComponent<RigidBodyComponent>("Rigid Body", [](RigidBodyComponent& c)
        {
            const char* types[] = { "Static", "Kinematic", "Dynamic" };
            int t = (int)c.Type;
            if (ImGui::Combo("Type", &t, types, 3)) c.Type = (RigidBodyComponent::BodyType)t;
            ImGui::DragFloat("Mass",            &c.Mass,            0.1f, 0.f, 10000.f);
            ImGui::DragFloat("Linear Damping",  &c.LinearDamping,   0.01f, 0.f, 1.f);
            ImGui::DragFloat("Angular Damping", &c.AngularDamping,  0.01f, 0.f, 1.f);
            ImGui::Checkbox("Use Gravity",      &c.UseGravity);
        });
    }

    void PropertiesPanel::DrawBoxCollider()
    {
        DrawComponent<BoxColliderComponent>("Box Collider", [](BoxColliderComponent& c)
        {
            DrawVec3("Size",   c.Size,   1.f, 0.01f);
            DrawVec3("Offset", c.Offset, 0.f, 0.01f);
            ImGui::Checkbox("Is Trigger", &c.IsTrigger);
        });
    }

    void PropertiesPanel::DrawSphereCollider()
    {
        DrawComponent<SphereColliderComponent>("Sphere Collider", [](SphereColliderComponent& c)
        {
            ImGui::DragFloat("Radius", &c.Radius, 0.01f, 0.f, 1000.f);
            DrawVec3("Offset", c.Offset, 0.f, 0.01f);
            ImGui::Checkbox("Is Trigger", &c.IsTrigger);
        });
    }

    void PropertiesPanel::DrawCapsuleCollider()
    {
        DrawComponent<CapsuleColliderComponent>("Capsule Collider", [](CapsuleColliderComponent& c)
        {
            ImGui::DragFloat("Radius", &c.Radius, 0.01f, 0.f, 1000.f);
            ImGui::DragFloat("Height", &c.Height, 0.01f, 0.f, 1000.f);
            DrawVec3("Offset", c.Offset, 0.f, 0.01f);
            ImGui::Checkbox("Is Trigger", &c.IsTrigger);
        });
    }

    void PropertiesPanel::DrawAudioSource()
    {
        DrawComponent<AudioSourceComponent>("Audio Source", [](AudioSourceComponent& c)
        {
            ImGui::DragFloat("Volume",       &c.Volume, 0.01f, 0.f, 1.f);
            ImGui::DragFloat("Pitch",        &c.Pitch,  0.01f, 0.1f, 4.f);
            ImGui::Checkbox("Loop",          &c.Loop);
            ImGui::Checkbox("Play On Awake", &c.PlayOnAwake);
            ImGui::Checkbox("Spatial",       &c.Spatial);
            if (c.Spatial)
            {
                ImGui::DragFloat("Min Distance", &c.MinDistance, 0.1f, 0.f, 1000.f);
                ImGui::DragFloat("Max Distance", &c.MaxDistance, 1.f,  0.f, 10000.f);
            }
        });
    }

    void PropertiesPanel::DrawScript()
    {
        DrawComponent<ScriptComponent>("Script", [](ScriptComponent& c)
        {
            char buf[256];
            strncpy_s(buf, sizeof(buf), c.ClassName.c_str(), sizeof(buf) - 1);
            buf[255] = 0;
            if (ImGui::InputText("Class", buf, sizeof(buf)))
                c.ClassName = buf;
        });
    }

    void PropertiesPanel::AddComponentPopup()
    {
        if (!ImGui::BeginPopup("AddComponent")) return;
        auto& reg = m_Scene->GetRegistry();

        auto addIf = [&]<typename T>(const char* label)
        {
            if (!reg.all_of<T>(m_Entity))
                if (ImGui::MenuItem(label))
                { reg.emplace<T>(m_Entity); ImGui::CloseCurrentPopup(); }
        };

        if (ImGui::BeginMenu("Rendering"))
        {
            addIf.operator()<StaticMeshComponent>  ("Static Mesh");
            addIf.operator()<SkeletalMeshComponent>("Skeletal Mesh");
            addIf.operator()<CameraComponent>      ("Camera");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Lighting"))
        {
            addIf.operator()<DirectionalLightComponent>("Directional Light");
            addIf.operator()<PointLightComponent>      ("Point Light");
            addIf.operator()<SpotLightComponent>       ("Spot Light");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Physics"))
        {
            addIf.operator()<RigidBodyComponent>     ("Rigid Body");
            addIf.operator()<BoxColliderComponent>   ("Box Collider");
            addIf.operator()<SphereColliderComponent>("Sphere Collider");
            addIf.operator()<CapsuleColliderComponent>("Capsule Collider");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Audio"))
        {
            addIf.operator()<AudioSourceComponent>("Audio Source");
            ImGui::EndMenu();
        }
        addIf.operator()<ScriptComponent>("Script");

        ImGui::EndPopup();
    }
}
