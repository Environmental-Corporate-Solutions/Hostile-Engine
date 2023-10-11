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

	bool DetectCollisionSys::IsColliding(const Transform& _t1, const Transform& _t2, const Vector3& distVector, const float& radSum, float& distSqrd)
	{
		distSqrd = distVector.LengthSquared();
		return distSqrd <= (radSum * radSum);
	}
	bool DetectCollisionSys::IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Transform& _tBox, const BoxCollider& _b)
	{
		return true;

	}
	bool DetectCollisionSys::IsColliding(const Transform& _tSphere, const Constraint& _c,float& distance)
	{
		distance = std::abs(_c.normal.Dot(_tSphere.position) - _c.offset);
		return _tSphere.scale.x*0.5f> distance;//assuming uniform x,y,and z
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
				//assuming Scale has uniform x,y,and z
				float radSum{ _transforms[i].scale.x + _transforms[j].scale.x };
				radSum *= 0.5f;
				float distSqrd{};
				Vector3 distVector{ _transforms[i].position - _transforms[j].position };
				if (IsColliding(_transforms[i],_transforms[j],distVector,radSum,distSqrd))
				{
					distVector.Normalize();

					CollisionData collisionData;
					collisionData.entity1 = _it.entity(i);
					collisionData.entity2 = _it.entity(j);
					collisionData.collisionNormal = distVector;
					collisionData.contactPoints = {
						std::make_pair<Vector3, Vector3>(
							_transforms[i].position - distVector * _transforms[i].scale.x*0.5f,
							_transforms[j].position + distVector * _transforms[j].scale.x*0.5f)
					};
					collisionData.penetrationDepth = radSum - sqrtf(distSqrd);
					collisionData.restitution = .5f;//temp
					collisionData.friction = .65f;    // temp
					collisionData.accumulatedNormalImpulse = 0.f;
					IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
				}
			}
		}

		// Sphere vs. Box
		//not used the typical AABB method, which involves translating the sphere center to the box's local coords 
		//and clamping to find the nearest point to avoid calculating the inverse matrix every tick
		static constexpr int NUM_AXES = 3;
		_it.world().each<BoxCollider>([&_spheres, &_it, &_transforms](flecs::entity e, BoxCollider& box)
			{
				for (int k = 0; k < _it.count(); ++k) 
				{
					const Transform* boxTransform = e.get<Transform>();
					if (!boxTransform) {
						return;
					}

					//assuming uniform x,y,and z for the sphere 
					const float sphereRad = _transforms[k].scale.x*0.5f;
					const Vector3 sphereCenter = _transforms[k].position;
					Vector3 centerToCenter = sphereCenter - boxTransform->position;
					Vector3 extents = boxTransform->scale*0.5f;
					Vector3 closestPoint = boxTransform->position;

					//for the X,Y,Z axis
					for (int i = 0; i < NUM_AXES; ++i) {
						Vector3 axis{
							boxTransform->matrix.m[i][0],
							boxTransform->matrix.m[i][1],
							boxTransform->matrix.m[i][2] 
						};
						axis.Normalize();//double check

						float extent = extents.x;
						if (i == 1) {
							extent = extents.y;
						}
						else if (i == 2) {
							extent = extents.z;
						}

						float projectionLength = centerToCenter.Dot(axis);
						// clamp the projection along the current axis
						projectionLength = std::clamp(projectionLength, -extent, extent);

						//accumulate for the closest point
						closestPoint += axis * projectionLength;
					}

					float distanceSquared = (closestPoint - sphereCenter).LengthSquared();

					if (distanceSquared > sphereRad * sphereRad) {
						continue; // no collision
					}

					//deal with collision
					CollisionData collisionData;
					collisionData.entity1 = _it.entity(k);
					collisionData.entity2 = e;
					collisionData.collisionNormal = sphereCenter - closestPoint;
					collisionData.collisionNormal.Normalize();
					collisionData.contactPoints = {
						std::make_pair<Vector3, Vector3>(
						Vector3(sphereCenter - collisionData.collisionNormal * sphereRad)
							, Vector3(closestPoint))
					};
					collisionData.penetrationDepth = sphereRad - sqrtf(distanceSquared);
					collisionData.restitution = .5f;//temp
					collisionData.friction = .6f;//temp
					collisionData.accumulatedNormalImpulse = 0.f;
					IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
				}
			});


		// Sphere vs. Constraint
		_it.world().each<Constraint>([&_spheres, &_it, &_transforms](flecs::entity e, Constraint& constraint) {
			for (int k = 0; k < _it.count(); ++k)
			{
				float distance{};
				if (IsColliding(_transforms[k], constraint, distance))
				{
					CollisionData collisionData;
					collisionData.entity1 = _it.entity(k);
					collisionData.entity2 = e;
					collisionData.collisionNormal = constraint.normal;
					collisionData.contactPoints = { 
						std::make_pair<Vector3,Vector3>(Vector3(_transforms[k].position - constraint.normal * distance),Vector3{}) 
					};
					collisionData.penetrationDepth = _transforms[k].scale.x*0.5-distance;
					collisionData.restitution = .18f; //   temp
					collisionData.friction = .65f;    //	"
					collisionData.accumulatedNormalImpulse = 0.f;

					IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
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
				Vector3 vertices[8];
				Vector3 extents = _transforms[k].scale * 1.0f;
				vertices[0] = Vector3(-extents.x, extents.y, extents.z);
				vertices[1] = Vector3(-extents.x, -extents.y, extents.z);
				vertices[2] = Vector3(extents.x, -extents.y, extents.z);
				vertices[3] = Vector3(extents.x, extents.y, extents.z);

				vertices[4] = Vector3(-extents.x, extents.y, -extents.z);
				vertices[5] = Vector3(-extents.x, -extents.y, -extents.z);
				vertices[6] = Vector3(extents.x, -extents.y, -extents.z);
				vertices[7] = Vector3(extents.x, extents.y, -extents.z);

				for (int i = 0; i < 8; ++i) {
					DirectX::SimpleMath::Vector4 temp = DirectX::SimpleMath::Vector4::Transform(
						DirectX::SimpleMath::Vector4{ vertices[i].x, vertices[i].y, vertices[i].z, 1.f },
						_transforms[k].matrix
					);
					vertices[i] = { temp.x,temp.y,temp.z };
				}

				for (int i = 0; i < 8; ++i)
				{
					float distance = constraint.normal.Dot(vertices[i]);

					if (distance < constraint.offset)
					{
						CollisionData collisionData;
						collisionData.entity1 = _it.entity(k);
						collisionData.entity2 = e;
						collisionData.collisionNormal = constraint.normal;
						collisionData.contactPoints = {
						std::make_pair<Vector3,Vector3>(
							Vector3(vertices[i]),
							Vector3{}
						) };
						collisionData.penetrationDepth = constraint.offset - distance;
						collisionData.restitution = .2f; //   temp
						collisionData.friction = .6f;    //	"
						collisionData.accumulatedNormalImpulse = 0.f;
						IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
					}
				}
			}
			});
	}

	void DetectCollisionSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components)
	{
	}


}
