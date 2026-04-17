#pragma once
#include "Force/Core/Base.h"
#include <glm/glm.hpp>

namespace Force
{
    enum class EditorCameraMode { Orbit, Fly };

    class EditorCamera
    {
    public:
        EditorCamera() = default;

        void OnUpdate(f32 dt, bool viewportHovered, bool viewportFocused);
        void OnResize(u32 width, u32 height);

        const glm::mat4& GetView()       const { return m_View; }
        const glm::mat4& GetProjection() const { return m_Proj; }
        glm::mat4 GetViewProjection()    const { return m_Proj * m_View; }

        glm::vec3 GetPosition()   const { return m_Position; }
        glm::vec3 GetForward()    const;
        glm::vec3 GetRight()      const;

        float GetFOV()   const { return m_FOV; }
        float GetNear()  const { return m_Near; }
        float GetFar()   const { return m_Far; }

        void SetFocusPoint(const glm::vec3& p) { m_FocalPoint = p; RecalcView(); }
        void Reset();

        EditorCameraMode GetMode() const { return m_Mode; }

    private:
        void RecalcView();
        void RecalcProj();

        void HandleOrbit(float dx, float dy);
        void HandlePan(float dx, float dy);
        void HandleZoom(float delta);

        EditorCameraMode m_Mode   = EditorCameraMode::Orbit;

        glm::vec3 m_Position    = { 0.f, 5.f, 10.f };
        glm::vec3 m_FocalPoint  = { 0.f, 0.f,  0.f };

        float m_Yaw     = -90.f;
        float m_Pitch   = -20.f;
        float m_Distance = 10.f;

        float m_FOV  = 60.f;
        float m_Near = 0.1f;
        float m_Far  = 10000.f;
        float m_AspectRatio = 16.f / 9.f;

        float m_OrbitSpeed  = 0.25f;
        float m_PanSpeed    = 0.003f;
        float m_ZoomSpeed   = 0.4f;
        float m_FlySpeed    = 10.f;

        glm::vec2 m_LastMousePos = {};
        bool      m_Dragging     = false;

        glm::mat4 m_View = glm::mat4(1.f);
        glm::mat4 m_Proj = glm::mat4(1.f);
    };
}
