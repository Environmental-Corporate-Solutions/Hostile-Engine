//------------------------------------------------------------------------------
//
// File Name:	DetecCollisionSys.cpp
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Engine.h"
#include "DetectCollisionSys.h"
#include "TransformSys.h"

namespace Hostile {

	ADD_SYSTEM(DetectCollisionSys);

	void DetectCollisionSys::OnCreate(flecs::world& _world)
	{
		_world.system<Transform, SphereCollider>("TestSphereCollision")
			.kind(IEngine::Get().GetDetectCollisionPhase())
			.iter(TestSphereCollision);

		_world.system<Transform, BoxCollider>("TestBoxCollision")
			.kind(IEngine::Get().GetDetectCollisionPhase())
			.iter(TestBoxCollision);
	}

	bool DetectCollisionSys::IsColliding(const Transform& _t1, const SphereCollider& _s1, const Transform& _t2, const SphereCollider& _s2)
	{
		float distanceSqrd = (_t1.position - _t2.position).LengthSquared();
		return distanceSqrd <= (_s1.radius + _s2.radius) * (_s1.radius + _s2.radius);
	}
	bool DetectCollisionSys::IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Transform& _tBox, const BoxCollider& _b)
	{
		return true;

	}
	bool DetectCollisionSys::IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Constraint& _c, float& distance)
	{
		distance = std::abs(_c.normal.Dot(_tSphere.position) - _c.offset);
		return _s.radius > distance;
	}
	bool DetectCollisionSys::IsColliding(const Transform& _t1, const BoxCollider& _b1, const Transform& _t2, const BoxCollider& _b2)
	{
		return true;
	}
	bool DetectCollisionSys::IsColliding(const Transform& _tBox, const BoxCollider& _b, const Constraint& _c)
	{
		return true;
	}

	void DetectCollisionSys::TestSphereCollision(flecs::iter& _it, Transform* _transforms, SphereCollider* _spheres)
	{
		// Sphere vs. Sphere
		for (int i = 0; i < _it.count(); ++i)
		{
			for (int j = i + 1; j < _it.count(); ++j)
			{
				if (IsColliding(_transforms[i], _spheres[i], _transforms[j], _spheres[j]))
				{
					//TODO
				}
			}
		}

		// Sphere vs. Box
		_it.world().each<BoxCollider>([&_spheres, &_it, &_transforms](flecs::entity e, BoxCollider& box)
			{
				if (e.has<Transform>())
				{  // Check if the entity also has a Transform component
					const Transform* tBox = e.get<Transform>();
					for (int k = 0; k < _it.count(); ++k) {
						if (IsColliding(_transforms[k], _spheres[k], *tBox, box))
						{
							//TODO
						}
					}
				}
			});


		// Sphere vs. Constraint
		_it.world().each<Constraint>([&_spheres, &_it, &_transforms](flecs::entity e, Constraint& constraint) {
			for (int k = 0; k < _it.count(); ++k)
			{
				float distance{};
				if (IsColliding(_transforms[k], _spheres[k], constraint, distance))
				{
					CollisionData collisionData;
					collisionData.otherEntity = e;
					collisionData.collisionNormal = constraint.normal;
					collisionData.contactPoints = { 
						std::make_pair<Vector3,Vector3>(Vector3(_transforms[k].position - constraint.normal * distance),Vector3{}) 
					};
					collisionData.penetrationDepth = _spheres[k].radius-distance;
					collisionData.restitution = .18f; //   temp
					collisionData.friction = .65f;    //	"

					_it.entity(k).set<CollisionData>(collisionData);
				}
			}
			});

	}

	void DetectCollisionSys::TestBoxCollision(flecs::iter& _it, Transform* _transforms, BoxCollider* _boxes) {
		// Box vs. Box
		for (int i = 0; i < _it.count(); ++i)
		{
			for (int j = i + 1; j < _it.count(); ++j)
			{
				if (IsColliding(_transforms[i], _boxes[i], _transforms[j], _boxes[j]))
				{
					//TODO
				}
			}
		}

		// Box vs. Constraint
		_it.world().each<Constraint>([&_boxes, &_it, &_transforms](flecs::entity e, Constraint& constraint) {
			for (int k = 0; k < _it.count(); ++k)
			{
				if (IsColliding(_transforms[k], _boxes[k], constraint))
				{
					//TODO
				}
			}
			});
	}


}
