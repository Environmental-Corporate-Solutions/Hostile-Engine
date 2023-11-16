//------------------------------------------------------------------------------
//
// File Name:	Utility.h
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#include "nlohmann/json.hpp"
#include "Matrix3.h"
#include <directxtk12/SimpleMath.h>

namespace Hostile
{
  nlohmann::json WriteVec3(DirectX::SimpleMath::Vector3 _vec);
  nlohmann::json WriteVec4(DirectX::SimpleMath::Vector4 _vec);
  nlohmann::json WriteMat3(const Matrix3& matrix);

  DirectX::SimpleMath::Vector3 ReadVec3(const nlohmann::json& _data);
  DirectX::SimpleMath::Vector4 ReadVec4(const nlohmann::json& _data);
  Matrix3 ReadMat3(const nlohmann::json& jsonMatrix);

  bool ImGuiButtonWithAlign(const char* label, float alignment = 0.5f, ImVec2 _size = {0,0});
  bool ImGuiMenuItemWithAlign(const char* label, float alignment = 0.5f, ImVec2 _size = { 0,0 });


}