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
		SimpleMath::Vector3 position = { 0,0,0 };
		SimpleMath::Quaternion orientation = { 0,0,0,0 };
		SimpleMath::Vector3 scale = { 1,1,1 };
		SimpleMath::Matrix matrix;
	};
	struct ObjectName
	{
		std::string name;
	};

	class TransformSys : public ISystem
	{
	private:

    static Transform GetWorldTransformUtil(const flecs::entity& e);
  public:
    virtual ~TransformSys() {}
    virtual void OnCreate(flecs::world& _world) override;
    static void OnUpdate(flecs::iter _info, Transform* _ptransforms, Transform* _ptransformParent);
    void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
    void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
    void GuiDisplay(flecs::entity& _entity, const std::string& type);
    void AddTransform(flecs::entity& _entity);

    static Transform CombineTransforms(const Transform& _parent, const Transform& _child);
    static Transform GetWorldTransform(const flecs::entity& e);
  };
}