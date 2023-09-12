//------------------------------------------------------------------------------
//
// File Name:	TransformSys.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "directxtk/SimpleMath.h"
#include "ISystem.h"
using namespace DirectX;
namespace Hostile
{
  struct Transform
  {
    SimpleMath::Vector3 position;
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