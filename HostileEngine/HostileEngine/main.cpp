#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include <iostream>
#include "backends/imgui_impl_glfw.h"
#include "Graphics.h"
#include "Input.h"

void ErrorCallback(int _error, const char* _desc)
{
    std::cout << "Error: " << _error << "\n" << _desc << std::endl;
}

void KeyCallback(GLFWwindow* _pWindow, int _key, int _scancode, int _action, int _mods)
{
    if (_key < 0)
        return;
    switch (_action)
    {
    case GLFW_PRESS:
    {
        Input::SetKey(static_cast<KeyCode>(_key), true);
        break;
    }
    case GLFW_RELEASE:
    {
        Input::SetKey(static_cast<KeyCode>(_key), false);
        break;
    }
    case GLFW_REPEAT:
    {
        Input::SetKey(static_cast<KeyCode>(_key), true);
        break;
    }
    }
}

int main()
{
    if (!glfwInit())
        return -1;

    AllocConsole();
    FILE* pFile;
    freopen_s(&pFile, "CONIN$", "r", stdin);
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    freopen_s(&pFile, "CONOUT$", "w", stderr);

    glfwSetErrorCallback(ErrorCallback);
    

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "D3DTest", NULL, NULL);
    if (!window)
    {
        return -1;
    }
    glfwSetKeyCallback(window, KeyCallback);
    ImGui::SetCurrentContext(ImGui::CreateContext());

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    HWND hwnd = glfwGetWin32Window(window);
    ImGui_ImplGlfw_InitForOther(window, true);

    Graphics graphics;
    graphics.Init(window);

    
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    while (!glfwWindowShouldClose(window))
    {
        Input::Reset();
        glfwPollEvents();

        if(Input::IsPressed(KeyCode::Escape))
        {
            glfwSetWindowShouldClose(window, true);
        }

        ImGui_ImplGlfw_NewFrame();
        graphics.BeginFrame();
        ImGui::NewFrame();
        ImGui::Begin("FUCK");
        ImGui::Button("Hello");
        ImGui::End();

        graphics.RenderImGui();
        graphics.EndFrame();
    }

    graphics.Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    FreeConsole();
    glfwDestroyWindow(window);
    glfwTerminate();
}