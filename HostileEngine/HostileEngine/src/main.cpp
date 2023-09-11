#include "stdafx.h"
#include <iostream>
#include <backends/imgui_impl_glfw.h>
#include "Graphics.h"
#include "ImguiTheme.h"

#include "Camera.h"

static Graphics graphics;
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
    graphics.OnResize(width, height);
}

int main()
{
  if (!glfwInit())
    return -1;

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
  glfwSetKeyCallback(window, KeyCallback);
  glfwSetWindowSizeCallback(window, window_size_callback);
  ImGui::SetCurrentContext(ImGui::CreateContext());

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

  HWND hwnd = glfwGetWin32Window(window);
  ImGui_ImplGlfw_InitForOther(window, true);

  
  graphics.Init(window);

  int width, height;
  glfwGetWindowSize(window, &width, &height);

  std::vector<Vertex> vertices = {
       { {  0.5f,  0.5f, -0.5f, 1 }, { 1, 0, 0, 1 } },
       { { -0.5f,  0.5f, -0.5f, 1 }, { 0, 0, 1, 1 } },
       { { -0.5f,  0.5f,  0.5f, 1 }, { 0, 1, 0, 1 } },
       { {  0.5f,  0.5f,  0.5f, 1 }, { 0, 0, 1, 1 } },
       { {  0.5f, -0.5f, -0.5f, 1 }, { 0, 1, 0, 1 } },
       { { -0.5f, -0.5f, -0.5f, 1 }, { 1, 0, 0, 1 } },
       { { -0.5f, -0.5f,  0.5f, 1 }, { 1, 0, 0, 1 } },
       { {  0.5f, -0.5f,  0.5f, 1 }, { 0, 1, 0, 1 } }
  };
  std::vector<uint32_t> indices = {
      0,1,2,
      0,2,3,
      0,4,5,
      0,5,1,
      1,5,6,
      1,6,2,
      2,6,7,
      2,7,3,
      3,7,4,
      3,4,0,
      4,7,6,
      4,6,5
  };
  VertexBuffer vertexBuffer;
  graphics.CreateVertexBuffer(vertices, indices, vertexBuffer);
  Texture texture;
  graphics.CreateTexture("grid", texture);
  while (!glfwWindowShouldClose(window))
  {
    ImGui_ImplGlfw_NewFrame();

    graphics.BeginFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    ImGui::GetIO().FontGlobalScale = 1.75f;
    SetImGuiTheme();
    ImGui::Begin("FUCK");
    ImGui::Button("Hello");
    ImGui::End();

    ImGui::Begin("hello world");
    ImGui::End();

    
    //graphics.RenderImGui();
    graphics.RenderVertexBuffer(vertexBuffer, texture, Matrix::Identity);
    graphics.RenderVertexBuffer(vertexBuffer, texture, Matrix::CreateTranslation({ 1, 1, 1 }));
    graphics.EndFrame();
    glfwPollEvents();
  }

  graphics.Shutdown();

  ImGui_ImplGlfw_Shutdown();
  glfwDestroyWindow(window);
  glfwTerminate();
}