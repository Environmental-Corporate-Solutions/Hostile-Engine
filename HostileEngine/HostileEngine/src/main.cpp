#include "stdafx.h"
#include <iostream>
#include <backends/imgui_impl_glfw.h>
#include "Graphics.h"
#include "ImguiTheme.h"


void ErrorCallback(int _error, const char* _desc)
{
  std::cout << "Error: " << _error << "\n" << _desc << std::endl;
}

void KeyCallback(GLFWwindow* _pWindow, int _key, int _scancode, int _action, int _mods)
{
  if (_key == GLFW_KEY_ESCAPE && _action == GLFW_PRESS)
  {
    ImGui_ImplGlfw_Shutdown();
    glfwSetWindowShouldClose(_pWindow, true);
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

  std::vector<Vertex> vertices = {
       { { -0.5f, -0.5f, 0, 1 }, { 0, 0, 0, 0 } },
       { { -0.5f,  0.5f, 0, 1 }, { 0, 0, 0, 0 } },
       { {  0.5f,  0.5f, 0, 1 }, { 0, 0, 0, 0 } },
       { { -0.5f, -0.5f, 0, 1 }, { 0, 0, 0, 0 } },
       { {  0.5f,  0.5f, 0, 1 }, { 0, 0, 0, 0 } },
       { {  0.5f, -0.5f, 0, 1 }, { 0, 0, 0, 0 } }
  };
  VertexBuffer vertexBuffer;
  graphics.CreateVertexBuffer(vertices, vertexBuffer);
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
    graphics.RenderVertexBuffer(vertexBuffer);
    graphics.EndFrame();
    glfwPollEvents();
  }

  graphics.Shutdown();

  ImGui_ImplGlfw_Shutdown();
  FreeConsole();
  glfwDestroyWindow(window);
  glfwTerminate();
}