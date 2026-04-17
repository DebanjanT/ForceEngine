#include "Force/Editor/EditorCamera.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>

namespace Force
{
    static float s_MouseSensitivity = 0.25f;

    void EditorCamera::Reset()
    {
        m_Position   = { 0.f, 5.f, 10.f };
        m_FocalPoint = { 0.f, 0.f,  0.f };
        m_Yaw        = -90.f;
        m_Pitch      = -20.f;
        m_Distance   = 10.f;
        RecalcView();
        RecalcProj();
    }

    glm::vec3 EditorCamera::GetForward() const
    {
        return glm::normalize(m_FocalPoint - m_Position);
    }

    glm::vec3 EditorCamera::GetRight() const
    {
        return glm::normalize(glm::cross(GetForward(), glm::vec3(0, 1, 0)));
    }

    void EditorCamera::OnResize(u32 width, u32 height)
    {
        if (height == 0) return;
        m_AspectRatio = (float)width / (float)height;
        RecalcProj();
    }

    void EditorCamera::OnUpdate(f32 dt, bool viewportHovered, bool viewportFocused)
    {
        ImGuiIO& io = ImGui::GetIO();
        glm::vec2 mouse{ io.MousePos.x, io.MousePos.y };
        glm::vec2 delta = (mouse - m_LastMousePos) * 0.01f;
        m_LastMousePos = mouse;

        if (!viewportHovered && !viewportFocused) return;

        // Right-mouse = fly mode
        if (io.MouseDown[ImGuiMouseButton_Right] && viewportFocused)
        {
            m_Mode = EditorCameraMode::Fly;

            // Rotate
            if (glm::length(delta) > 0.f)
            {
                m_Yaw   += delta.x * 60.f;
                m_Pitch  = std::clamp(m_Pitch - delta.y * 60.f, -89.f, 89.f);
            }

            // WASD movement
            glm::vec3 fwd   = GetForward();
            glm::vec3 right = GetRight();
            glm::vec3 up    = glm::vec3(0, 1, 0);
            float spd = m_FlySpeed * dt;
            if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow))    m_Position += fwd   * spd;
            if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow))  m_Position -= fwd   * spd;
            if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow))  m_Position -= right * spd;
            if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) m_Position += right * spd;
            if (ImGui::IsKeyDown(ImGuiKey_Q)) m_Position -= up * spd;
            if (ImGui::IsKeyDown(ImGuiKey_E)) m_Position += up * spd;

            // Shift = faster
            if (io.KeyShift) m_Position += fwd * spd * 3.f;

            m_FocalPoint = m_Position + GetForward() * m_Distance;
        }
        else if (io.MouseDown[ImGuiMouseButton_Middle] && viewportHovered)
        {
            m_Mode = EditorCameraMode::Orbit;
            HandlePan(delta.x, delta.y);
        }
        else if (io.MouseDown[ImGuiMouseButton_Left] &&
                 io.KeyAlt && viewportHovered)
        {
            m_Mode = EditorCameraMode::Orbit;
            HandleOrbit(delta.x, delta.y);
        }
        else
        {
            m_Mode = EditorCameraMode::Orbit;
        }

        // Scroll zoom
        if (viewportHovered && io.MouseWheel != 0.f)
            HandleZoom(io.MouseWheel);

        RecalcView();
    }

    void EditorCamera::HandleOrbit(float dx, float dy)
    {
        m_Yaw   += dx * 100.f * m_OrbitSpeed;
        m_Pitch  = std::clamp(m_Pitch - dy * 100.f * m_OrbitSpeed, -89.f, 89.f);

        float yawR   = glm::radians(m_Yaw);
        float pitchR = glm::radians(m_Pitch);
        m_Position = m_FocalPoint - glm::vec3(
            m_Distance * cosf(pitchR) * cosf(yawR),
            m_Distance * sinf(pitchR),
            m_Distance * cosf(pitchR) * sinf(yawR));
    }

    void EditorCamera::HandlePan(float dx, float dy)
    {
        float speed = m_Distance * m_PanSpeed * 100.f;
        m_FocalPoint -= GetRight() * dx * speed;
        m_FocalPoint += glm::vec3(0, 1, 0) * dy * speed;
        m_Position = m_FocalPoint - GetForward() * m_Distance;
    }

    void EditorCamera::HandleZoom(float delta)
    {
        m_Distance = std::max(0.5f, m_Distance - delta * m_ZoomSpeed * m_Distance * 0.2f);
        float yawR   = glm::radians(m_Yaw);
        float pitchR = glm::radians(m_Pitch);
        m_Position = m_FocalPoint - glm::vec3(
            m_Distance * cosf(pitchR) * cosf(yawR),
            m_Distance * sinf(pitchR),
            m_Distance * cosf(pitchR) * sinf(yawR));
    }

    void EditorCamera::RecalcView()
    {
        m_View = glm::lookAt(m_Position, m_FocalPoint, glm::vec3(0, 1, 0));
    }

    void EditorCamera::RecalcProj()
    {
        m_Proj = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_Near, m_Far);
        m_Proj[1][1] *= -1.f; // Vulkan Y-flip
    }
}
