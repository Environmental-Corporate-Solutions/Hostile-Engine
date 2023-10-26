#include "stdafx.h"
#include <iostream>
#include <backends/imgui_impl_glfw.h>
#include "Graphics/IGraphics.h"
#include "Engine.h"
#include "flecs.h"
#include "Camera.h"
#include "Input.h"
#include "Profiler/Profiler.h"
#include "Script/ScriptCompiler.h"
#include "Script/ScriptEngine.h"
#include "ImGuizmo.h"

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

void window_size_callback(GLFWwindow* window, int width, int height)
{
    IGraphics::Get().OnResize(width, height);
}

int main(int [[maybe_unused]] argc, char** argv)
{
  if (!glfwInit())
    return -1;
  Script::ScriptEngine::Init(argv[0]);
  //Script::ScriptCompiler::CompileAllCSFiles();
  

  Log::Info("Engine Started!");

  Log::Info("Test Info");
  Log::Debug("Test Debug");
  Log::Critical("Test Critical");
  Log::Error("Test Error");
  Log::Trace("Test Trace");
  Log::Warn("Test Warn");

  glfwSetErrorCallback(ErrorCallback);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  //glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); //start window in maximixed window
  GLFWwindow* window = glfwCreateWindow(1920, 1080, "Hostile Editor", NULL, NULL);
  if (!window)
  {
    return -1;
  }
  glfwSetKeyCallback(window, KeyCallback);
  glfwSetWindowSizeCallback(window, window_size_callback);
  ImGui::SetCurrentContext(ImGui::CreateContext());
  ImNodes::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

  HWND hwnd = glfwGetWin32Window(window);
  ImGui_ImplGlfw_InitForOther(window, true);

  IGraphics& graphics = IGraphics::Get();
  graphics.Init(window);

  int width, height;
  glfwGetWindowSize(window, &width, &height);
  Hostile::IEngine& engine = Hostile::IEngine::Get();
  engine.Init();
  
  while (!glfwWindowShouldClose(window))
  {
    ImGui_ImplGlfw_NewFrame();
   

    graphics.BeginFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
    ImGui::DockSpaceOverViewport();

    engine.Update();
    ImGui::Begin("Profiler");
        if (ImGui::Button("open"))
            Profiler::OpenProfiler();
  	ImGui::End();
    //if (Input::IsPressed(Key::Escape))
      //glfwSetWindowShouldClose(window, true);

    //flecs::entity e1 = world.entity("Sphere1");
    //if (e1.has<Matrix>()) {
    //    const Matrix& entityMatrix = *e1.get<Matrix>();
    //    graphics.RenderVertexBuffer(vertexBuffer, texture, entityMatrix);
    //}

    //for testing please remove it later
    const int hardcoded_node_id = 1;
    ImGui::Begin("node editor");
    ImNodes::BeginNodeEditor();

    ImNodes::BeginNode(hardcoded_node_id);
    ImGui::Dummy(ImVec2(80.0f, 45.0f));
    ImNodes::EndNode();

    ImNodes::EndNodeEditor();
    ImGui::End();

    graphics.EndFrame();
    Input::Reset();
    glfwPollEvents();
    FrameMark;
  }

  ImNodes::DestroyContext();
  graphics.Shutdown();
  Script::ScriptEngine::Shutdown();
  ImGui_ImplGlfw_Shutdown();
  glfwDestroyWindow(window);
  glfwTerminate();
}