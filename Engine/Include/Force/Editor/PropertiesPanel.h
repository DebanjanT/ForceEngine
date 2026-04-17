#pragma once
#include "Force/Core/Base.h"
#include "Force/Core/ECS.h"

namespace Force
{
    class PropertiesPanel
    {
    public:
        void OnImGui();
        void SetSelectedEntity(Entity e, Scene* scene) { m_Entity = e; m_Scene = scene; }

        bool m_Open = true;

    private:
        void DrawEntityHeader();
        void DrawTransform();
        void DrawStaticMesh();
        void DrawSkeletalMesh();
        void DrawCamera();
        void DrawDirectionalLight();
        void DrawPointLight();
        void DrawSpotLight();
        void DrawRigidBody();
        void DrawBoxCollider();
        void DrawSphereCollider();
        void DrawCapsuleCollider();
        void DrawAudioSource();
        void DrawScript();

        template<typename T, typename Fn>
        void DrawComponent(const char* label, Fn fn, bool removable = true);

        void AddComponentPopup();

        Entity  m_Entity = entt::null;
        Scene*  m_Scene  = nullptr;
    };
}
