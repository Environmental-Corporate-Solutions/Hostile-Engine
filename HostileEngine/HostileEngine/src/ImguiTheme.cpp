#include "stdafx.h"
#include "ImguiTheme.h"
#include "imgui.h"

void SetImGuiTheme()
{
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 5.3f;
  style.FrameRounding = 2.3f;
  style.ScrollbarRounding = 0;

  ImVec4 darkBrown(70 / 255.0f, 40 / 255.0f, 4 / 255.0f, 255 / 255.0f);
  ImVec4 brown(132 / 255.0f, 68 / 255.0f, 1 / 255.0f, 255 / 255.0f);
  ImVec4 darkGreen(55 / 255.0f, 78 / 255.0f, 47 / 255.0f,255 / 255.0f);
  ImVec4 forestGreen(24 / 255.0f, 135 / 255.0f, 43 / 255.0f,255 / 255.0f);
  ImVec4 limeGreen(32 / 255.0f, 201 / 255.0f, 60 / 255.0f, 255 / 255.0f);
  ImVec4 pigmentGreen(26 / 255.0f, 160 / 255.0f, 48 / 255.0f, 255 / 255.0f);

  ImVec4 white(1,1,1,1);
  ImVec4 black(0, 0, 0, 1);


  //text
  style.Colors[ImGuiCol_Text] = white;
  style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
  //window backround
  style.Colors[ImGuiCol_WindowBg] = darkBrown;
  //popup backround
  style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);

  style.Colors[ImGuiCol_TabHovered] = black;
  style.Colors[ImGuiCol_TabActive] = limeGreen;
  style.Colors[ImGuiCol_TabUnfocusedActive] = forestGreen;
  style.Colors[ImGuiCol_Tab] = pigmentGreen;

  //border
  style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
  //frame
  style.Colors[ImGuiCol_FrameBg] = black;
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
  //title bar
  style.Colors[ImGuiCol_TitleBg] = darkGreen;
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
  style.Colors[ImGuiCol_TitleBgActive] = forestGreen;
  //menu bar
  style.Colors[ImGuiCol_MenuBarBg] = black;
  //scroll bar
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
  //slider
  style.Colors[ImGuiCol_SliderGrab] = black;
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
  //buttons and misc
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
  style.Colors[ImGuiCol_Button] = darkGreen;
  style.Colors[ImGuiCol_ButtonHovered] = forestGreen;
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
  //header
  style.Colors[ImGuiCol_Header] = black;
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
  style.Colors[ImGuiCol_HeaderActive] = black;
  //resize
  style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
  style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
  style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
  //plot
  style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
}
