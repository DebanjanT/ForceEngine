#include "Force/Platform/Window.h"
#include "Force/Platform/Input.h"
#include "Force/Core/Logger.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Force
{
    static u32 s_GLFWWindowCount = 0;
    
    static void GLFWErrorCallback(int error, const char* description)
    {
        FORCE_CORE_ERROR("GLFW Error ({}): {}", error, description);
    }
    
    Window::Window(const WindowProps& props)
    {
        Init(props);
    }
    
    Window::~Window()
    {
        Shutdown();
    }
    
    void Window::Init(const WindowProps& props)
    {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;
        m_Data.VSync = props.VSync;
        
        FORCE_CORE_INFO("Creating window {} ({} x {})", props.Title, props.Width, props.Height);
        
        if (s_GLFWWindowCount == 0)
        {
            int success = glfwInit();
            FORCE_CORE_ASSERT(success, "Could not initialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);
        }
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, props.Resizable ? GLFW_TRUE : GLFW_FALSE);
        
        if (props.Fullscreen)
        {
            GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
            m_Window = glfwCreateWindow(mode->width, mode->height, props.Title.c_str(), primaryMonitor, nullptr);
            m_Data.Width = mode->width;
            m_Data.Height = mode->height;
        }
        else
        {
            m_Window = glfwCreateWindow(static_cast<int>(props.Width), static_cast<int>(props.Height), 
                                       props.Title.c_str(), nullptr, nullptr);
        }
        
        s_GLFWWindowCount++;
        
        glfwSetWindowUserPointer(m_Window, &m_Data);
        SetVSync(props.VSync);
        
        // Set GLFW callbacks
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
        {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            data.Width = width;
            data.Height = height;
        });
        
        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
        {
            // Window close event
        });
        
        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            Input::ProcessKeyEvent(static_cast<KeyCode>(key), action != GLFW_RELEASE);
        });
        
        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
        {
            Input::ProcessMouseButtonEvent(static_cast<MouseButton>(button), action != GLFW_RELEASE);
        });
        
        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xpos, double ypos)
        {
            Input::ProcessMouseMoveEvent(static_cast<f32>(xpos), static_cast<f32>(ypos));
        });
        
        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xoffset, double yoffset)
        {
            Input::ProcessMouseScrollEvent(static_cast<f32>(yoffset));
        });
    }
    
    void Window::Shutdown()
    {
        glfwDestroyWindow(m_Window);
        s_GLFWWindowCount--;
        
        if (s_GLFWWindowCount == 0)
        {
            glfwTerminate();
        }
    }
    
    void Window::OnUpdate()
    {
        glfwPollEvents();
    }
    
    void Window::SetVSync(bool enabled)
    {
        m_Data.VSync = enabled;
    }
    
    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_Window);
    }
    
    void Window::SetTitle(const std::string& title)
    {
        m_Data.Title = title;
        glfwSetWindowTitle(m_Window, title.c_str());
    }
    
    void Window::CreateVulkanSurface(void* instance, void* surface)
    {
        VkResult result = glfwCreateWindowSurface(
            static_cast<VkInstance>(instance),
            m_Window,
            nullptr,
            static_cast<VkSurfaceKHR*>(surface)
        );
        
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan surface!");
    }
    
    Scope<Window> Window::Create(const WindowProps& props)
    {
        return CreateScope<Window>(props);
    }
}
