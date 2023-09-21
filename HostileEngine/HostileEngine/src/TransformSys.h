//------------------------------------------------------------------------------
//
// File Name:	TransformSys.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "directxtk12/SimpleMath.h"
#include "ISystem.h"
using namespace DirectX;
namespace Hostile
{
  struct Transform
  {
    SimpleMath::Vector3 position;
    SimpleMath::Quaternion orientation;
    SimpleMath::Vector3 scale;
    SimpleMath::Matrix matrix;
  };

  class TransformSys : public ISystem
  {
  private:

  public:
    virtual ~TransformSys() {}
    virtual void OnCreate(flecs::world& _world) override;
    static void OnUpdate(flecs::iter _info, Transform* _ptransforms);
  };
}