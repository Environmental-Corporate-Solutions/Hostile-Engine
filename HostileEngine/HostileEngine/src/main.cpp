#include "stdafx.h"
#include <iostream>
#include <backends/imgui_impl_glfw.h>
#include "ImguiTheme.h"
#include "Engine.h"
#include "flecs.h"
#include "Camera.h"
#include "IGraphics.h"

using namespace Hostile;
void ErrorCallback(int _error, const char* _desc)
{
  Log::Critical("Error: {}\n{}", _error, _desc);
}

void KeyCallback(GLFWwindow* _pWindow, int _key, int _scancode, int _action, int _mods)
{
  if (_key == GLFW_KEY_ESCAPE && _action == GLFW_PRESS)
  {
    ImGui_ImplGlfw_Shutdown();
    glfwSetWindowShouldClose(_pWindow, true);
  }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    IGraphics::Get().OnResize(width, height);
}

int main()
{
  if (!glfwInit())
    return -1;

  std::cout << "after glfw init" << std::endl;
  Log::Info("Engine Started!");

  Log::Info("Test Info");
  Log::Debug("Test Debug");
  Log::Critical("Test Critical");
  Log::Error("Test Error");
  Log::Trace("Test Trace");
  Log::Warn("Test Warn");

  glfwSetErrorCallback(ErrorCallback);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(1920, 1080, "D3DTest", NULL, NULL);
  if (!window)
  {
    return -1;
  }
  std::cout << "after window create" << std::endl;
  glfwSetKeyCallback(window, KeyCallback);
  glfwSetWindowSizeCallback(window, window_size_callback);
  ImGui::SetCurrentContext(ImGui::CreateContext());

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

  HWND hwnd = glfwGetWin32Window(window);
  ImGui_ImplGlfw_InitForOther(window, true);

  std::cout << "before graphics init" << std::endl;
  IGraphics& graphics = IGraphics::Get();
  graphics.Init(window);
  std::cout << "after graphics init" << std::endl;

  int width, height;
  glfwGetWindowSize(window, &width, &height);
  Hostile::IEngine& engine = Hostile::IEngine::Get();
  engine.Init();
  auto& world = engine.GetWorld();
  
  float gamer = 0;
  bool thing1 = false;
  std::cout << "after engine init" << std::endl;
  while (!glfwWindowShouldClose(window))
  {
    ImGui_ImplGlfw_NewFrame();

    graphics.BeginFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    ImGui::GetIO().FontGlobalScale = 1.75f;
    SetImGuiTheme();
    ImGui::Begin("Test");
    ImGui::Button("Hello");
    ImGui::SliderFloat("test slider", &gamer, 0, 2.5f);
    ImGui::InputFloat("Test input", &gamer);
    ImGui::Checkbox("bool", &thing1);
    ImGui::End();

    ImGui::Begin("hello world");
    ImGui::End();

    ImGui::Begin("Test2");
    ImGui::End();

    Log::DrawConsole();

    world.progress();
    //graphics.RenderImGui();
    graphics.EndFrame();
    glfwPollEvents();
  }

  graphics.Shutdown();

  ImGui_ImplGlfw_Shutdown();
  glfwDestroyWindow(window);
  glfwTerminate();
}