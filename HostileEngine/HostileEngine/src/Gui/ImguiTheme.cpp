#include "stdafx.h"
#include "ImguiTheme.h"
#include "imgui.h"

ImVec4 MakeColor(int _r, int _g, int _b, float _alpha)
{
  return ImVec4(_r / 255.0f, _g / 255.0f, _b / 255.0f, _alpha);
}


void SetImGuiTheme()
{
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 5.3f;
  style.FrameRounding = 2.3f;
  style.ScrollbarRounding = 0;


  //ImVec4 darkBrown = MakeColor(52, 42, 38, 1);
  ImVec4 darkGray = MakeColor(35,35,35, 1);
  ImVec4 brown = MakeColor(132, 68, 1, 1);


  ImVec4 darkGreen = MakeColor(46, 66, 40, 1);
  ImVec4 calPolyGreen = MakeColor(55, 78, 47, 1);
  ImVec4 hunterGreen = MakeColor(48, 99, 48, 1);
  ImVec4 forestGreen = MakeColor(24, 135, 43, 1);
  ImVec4 pigmentGreen = MakeColor(26, 160, 48, 1);
  ImVec4 lightPigmentGreen = MakeColor(61, 175, 80, 1);
  ImVec4 emerald = MakeColor(96, 190, 111, 1);


  ImVec4 white(1, 1, 1, 1);
  ImVec4 black(0, 0, 0, 1);
  ImVec4 darkGrey = MakeColor(25,25,25,0.75f);


  //text
  style.Colors[ImGuiCol_Text] = white;
  style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg] = darkGrey;

  //window backround
  style.Colors[ImGuiCol_WindowBg] = darkGray;

  //popup backround
  style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);

  style.Colors[ImGuiCol_TabHovered] = emerald;
  style.Colors[ImGuiCol_TabActive] = lightPigmentGreen;
  style.Colors[ImGuiCol_TabUnfocusedActive] = hunterGreen;
  style.Colors[ImGuiCol_Tab] = pigmentGreen;
  style.Colors[ImGuiCol_TabUnfocused] = calPolyGreen;

  //border
  style.Colors[ImGuiCol_Border] = ImVec4(0.40f, 0.40f, 0.40f, 0.85f);
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
  

  //frame
  style.Colors[ImGuiCol_FrameBg] = calPolyGreen;
  style.Colors[ImGuiCol_FrameBgHovered] = hunterGreen;
  style.Colors[ImGuiCol_FrameBgActive] = forestGreen;
  //title bar
  style.Colors[ImGuiCol_TitleBg] = darkGreen;
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
  style.Colors[ImGuiCol_TitleBgActive] = forestGreen;
  //menu bar
  style.Colors[ImGuiCol_MenuBarBg] = calPolyGreen;
  //scroll bar
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);

  //slider
  style.Colors[ImGuiCol_SliderGrab] = forestGreen;
  style.Colors[ImGuiCol_SliderGrabActive] = lightPigmentGreen;
  

  //buttons and Checkmark
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
  style.Colors[ImGuiCol_Button] = forestGreen;
  style.Colors[ImGuiCol_ButtonHovered] = pigmentGreen;
  style.Colors[ImGuiCol_ButtonActive] = hunterGreen;

  //header
  style.Colors[ImGuiCol_Header] = hunterGreen;
  style.Colors[ImGuiCol_HeaderHovered] = pigmentGreen;
  style.Colors[ImGuiCol_HeaderActive] = forestGreen;

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
