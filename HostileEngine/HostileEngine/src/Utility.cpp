//------------------------------------------------------------------------------
//
// File Name:	Utility.cpp
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h" 
namespace Hostile
{
  nlohmann::json Hostile::WriteVec3(DirectX::SimpleMath::Vector3 _vec)
  {
    nlohmann::json arr = nlohmann::json::array();
    arr.push_back(_vec.x);
    arr.push_back(_vec.y);
    arr.push_back(_vec.z);
    return arr;
  }
  nlohmann::json WriteVec4(DirectX::SimpleMath::Vector4 _vec)
  {
    nlohmann::json arr = nlohmann::json::array();
    arr.push_back(_vec.x);
    arr.push_back(_vec.y);
    arr.push_back(_vec.z);
    arr.push_back(_vec.w);
    return arr;

  }

  Vector3 ReadVec3(const nlohmann::json& _data)
  {
    Vector3 vec;
    auto arr = _data.begin();
    vec.x = arr.value();
    arr++;
    vec.y = arr.value();
    arr++;
    vec.z = arr.value();
    return vec;
  }

  Vector4 ReadVec4(const nlohmann::json& _data)
  {
    Vector4 vec;
    auto arr = _data.begin();
    vec.x = arr.value();
    arr++;
    vec.y = arr.value();
    arr++;
    vec.z = arr.value();
    arr++;
    vec.w = arr.value();
    return vec;
  }

  nlohmann::json WriteMat3(const Matrix3& matrix)
  {
      return {
          { matrix[0], matrix[1], matrix[2] },
          { matrix[3], matrix[4], matrix[5] },
          { matrix[6], matrix[7], matrix[8] }
      };
  }

  Matrix3 ReadMat3(const nlohmann::json& jsonMatrix)
  {
      Matrix3 matrix;
      for (int i{}; i < 9; ++i)
      {
          matrix[i] = jsonMatrix[i / 3][i % 3];
      }
      return matrix;
  }

  bool ImGuiButtonWithAlign(const char* label, float alignment, ImVec2 _size)
  {
      ImGuiStyle& style = ImGui::GetStyle();

      float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
      float avail = ImGui::GetContentRegionAvail().x;

    float off = (avail - size) * alignment;
     if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

      return ImGui::Button(label , _size);
  }


}