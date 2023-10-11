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
#include <directxtk12/SimpleMath.h>

namespace Hostile
{
  nlohmann::json WriteVec3(DirectX::SimpleMath::Vector3 _vec);
  nlohmann::json WriteVec4(DirectX::SimpleMath::Vector4 _vec);


}