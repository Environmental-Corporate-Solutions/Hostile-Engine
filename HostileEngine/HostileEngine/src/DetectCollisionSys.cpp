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

	inline Vector3 DetectCollisionSys::GetAxis(const Matrix& model, int index)
	{
		if (index < 0 || index > 2) {
			Log::Error("RigidBody::getAxis(): index, out of bounds\n");
			throw std::runtime_error("RigidBody::getAxis(): index, out of bounds\n");
		}

		Vector3 result(
			model.m[index][0],
			model.m[index][1],
			model.m[index][2]
		);
		result.Normalize();

		return result;
	}

	float DetectCollisionSys::CalcPenetration(const Transform& t1, const Transform& t2, const Vector3& axis) {
		Vector3 centerToCenter = t2.position - t1.position;
		Vector3 extents1 = t1.scale*0.5;
		Vector3 extents2 = t2.scale*0.5f;

		float projectedCenterToCenter = abs(centerToCenter.Dot(axis));
		float projectedSum =
			  abs((GetAxis(t1.matrix, 0) * extents1.x).Dot(axis))
			+ abs((GetAxis(t1.matrix, 1) * extents1.y).Dot(axis))
			+ abs((GetAxis(t1.matrix, 2) * extents1.z).Dot(axis))

			+ abs((GetAxis(t2.matrix, 0) * extents2.x).Dot(axis))
			+ abs((GetAxis(t2.matrix, 1) * extents2.y).Dot(axis))
			+ abs((GetAxis(t2.matrix, 2) * extents2.z).Dot(axis));

		return projectedSum - projectedCenterToCenter;
	}
	void DetectCollisionSys::CalcOBBsContactPoints(const Transform& t1, const Transform& t2, CollisionData& newContact, int minPenetrationAxisIdx) {
		//I. vertex to face
		if (minPenetrationAxisIdx >= 0 && minPenetrationAxisIdx < 3)
		{
			Vector3 contactPoint = GetLocalContactVertex(newContact.collisionNormal, t2, std::less<float>());
			DirectX::SimpleMath::Vector4 temp = DirectX::SimpleMath::Vector4::Transform(DirectX::SimpleMath::Vector4{ contactPoint.x, contactPoint.y, contactPoint.z, 1.f }, t2.matrix);
			contactPoint = { temp.x,temp.y,temp.z };

			newContact.contactPoints = {
				{ contactPoint - newContact.collisionNormal * newContact.penetrationDepth },
				contactPoint
			};
		}
		else if (minPenetrationAxisIdx >= 3 && minPenetrationAxisIdx < 6) {
			Vector3 contactPoint = GetLocalContactVertex(newContact.collisionNormal, t1, std::greater<float>());

			DirectX::SimpleMath::Vector4 temp = DirectX::SimpleMath::Vector4::Transform(DirectX::SimpleMath::Vector4{ contactPoint.x, contactPoint.y, contactPoint.z, 1.f }, t2.matrix);
			contactPoint = { temp.x,temp.y,temp.z };

			newContact.contactPoints = {
				contactPoint,
				{ contactPoint - newContact.collisionNormal * newContact.penetrationDepth }
			};
		}
		//II. edge to edge
		else //need further updates
		{
			// Determine the local contact vertex on box1 based on the collision's hit normal.
			Vector3 vertexOne = GetLocalContactVertex(newContact.collisionNormal, t1, std::greater<float>());
			Vector3 vertexTwo = GetLocalContactVertex(newContact.collisionNormal, t2, std::less<float>());

			static int EDGES_COLLISON_AXIS_IDX{ 6 };
			static int NUM_AXIS = 3; //x,y,z
			int penetrationAxisStart = minPenetrationAxisIdx - EDGES_COLLISON_AXIS_IDX;
			int testAxis1 = penetrationAxisStart / NUM_AXIS;
			int testAxis2 = penetrationAxisStart % NUM_AXIS;

			//orientation of the colliding edges
			Vector3 edge1, edge2;

			std::array<float, 3> vertexOneArr{ vertexOne.x, vertexOne.y, vertexOne.z };
			std::array<float, 3> vertexTwoArr{ vertexTwo.x, vertexTwo.y, vertexTwo.z };

			edge1 = (vertexOneArr[testAxis1] < 0) ? GetAxis(t1.matrix, testAxis1) : GetAxis(t1.matrix, testAxis1) * -1.f;
			edge2 = (vertexTwoArr[testAxis2] < 0) ? GetAxis(t2.matrix, testAxis2) : GetAxis(t2.matrix, testAxis2) * -1.f;

			// Compute coefficients
			float a = edge1.Dot(edge1);
			float b = edge1.Dot(edge2);
			float c = edge2.Dot(edge2);
			float d = edge1.Dot(vertexOne - vertexTwo);
			float e = edge2.Dot(vertexOne - vertexTwo);

			float det = a * c - b * b; // Determinant of the 2x2 matrix
			float s = (b * e - c * d) / det;
			float t = (a * e - b * d) / det;

			// Compute the closest points on the two edges
			Vector3 closestPointOne = vertexOne + s * edge1;
			Vector3 closestPointTwo = vertexTwo + t * edge2;

			newContact.contactPoints = {
				closestPointOne,
				closestPointTwo
			};
		}
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
					collisionData.restitution = 0.5f;//temp
					collisionData.friction = .6f;//temp
					collisionData.accumulatedNormalImpulse = 0.f;
					IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
				}
			});


		// Sphere vs. Constraint
		_it.world().each<Constraint>([&_spheres, &_it, &_transforms](flecs::entity e, Constraint& _constraint) {
			for (int k = 0; k < _it.count(); ++k)
			{
				float distance{};
				if (IsColliding(_transforms[k], _constraint, distance))
				{
					CollisionData collisionData;
					collisionData.entity1 = _it.entity(k);
					collisionData.entity2 = e;
					collisionData.collisionNormal = _constraint.normal;
					collisionData.contactPoints = { 
						std::make_pair<Vector3,Vector3>(Vector3(_transforms[k].position - _constraint.normal * distance),Vector3{})
					};
					collisionData.penetrationDepth = _transforms[k].scale.x*0.5f-distance;
					collisionData.restitution = .18f; //   temp
					collisionData.friction = .65f;    //	"
					collisionData.accumulatedNormalImpulse = 0.f;

					IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
				}
			}
			});

	}
	Vector3 DetectCollisionSys::GetLocalContactVertex(Vector3 collisionNormal, const Transform& t, std::function<bool(const float&, const float&)> const cmp) {
		Vector3 contactPoint{ t.scale*0.5f };
		if (cmp(GetAxis(t.matrix, 0).Dot(collisionNormal), 0)) {
			contactPoint.x = -contactPoint.x;
		}
		if (cmp(GetAxis(t.matrix, 1).Dot(collisionNormal), 0)) {
			contactPoint.y = contactPoint.y;
		}
		if (cmp(GetAxis(t.matrix, 2).Dot(collisionNormal), 0)) {
			contactPoint.z = -contactPoint.z;
		}
		return contactPoint;
	}

	void DetectCollisionSys::TestBoxCollision(flecs::iter& _it, Transform* _transforms, BoxCollider* _boxes) {
		// Box vs. Box
		for (int i = 0; i < _it.count(); ++i)
		{
			for (int j = i + 1; j < _it.count(); ++j)
			{
				bool isColliding{ true };
				std::vector<Vector3> axes;

				//"face(box1)" <-> vertex(box2)
				for (int k{}; k < 3; ++k) {
					axes.push_back(GetAxis(_transforms[i].matrix, k));
				}

				//"face(box2)" <-> vertex(box1)
				for (int k{}; k < 3; ++k) {
					axes.push_back(GetAxis(_transforms[j].matrix, k));
				}

				//edge-edge
				for (int p{}; p < 3; ++p) {
					for (int q{}; q < 3; ++q) {
						Vector3 crossProduct = axes[p].Cross(axes[3 + q]);
						crossProduct.Normalize();
						axes.push_back(crossProduct);
					}
				}

				float minPenetration = FLT_MAX;
				int minAxisIdx = 0;

				for (int k{}; k < axes.size(); ++k) {
					float penetration = CalcPenetration(_transforms[i], _transforms[j], axes[k]);

					if (penetration <= 0.f) {
						isColliding = false;
						break;
					}

					if (penetration <= minPenetration) {
						minPenetration = penetration;
						minAxisIdx = k;
					}
				}
				if (isColliding == false) {
					continue;
				}

				CollisionData newContact;
				newContact.entity1 = _it.entity(i);
				newContact.entity2 = _it.entity(j);
				newContact.penetrationDepth = minPenetration;
				newContact.restitution = 0.1f;  //temp
				newContact.friction = 0.6f;		//temp

				//vector pointing from the center of box2 to the center of box1
				Vector3 box2ToBox1 = _transforms[i].position - _transforms[j].position;

				//ensures the collisionNormal to always point from box2 towards box1.
				newContact.collisionNormal = (axes[minAxisIdx].Dot(box2ToBox1) < 0) ? -axes[minAxisIdx]: axes[minAxisIdx];

				CalcOBBsContactPoints(_transforms[i], _transforms[j], newContact, minAxisIdx);
				IEngine::Get().GetWorld().entity().set<CollisionData>(newContact);
			}
		}

		// Box vs. Constraint
		_it.world().each<Constraint>([&_boxes, &_it, &_transforms](flecs::entity e, Constraint& constraint) {
			for (int k = 0; k < _it.count(); ++k)
			{
				Vector3 vertices[8];
				Vector3 extents = { 0.5f,0.5f,0.5f };
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
					vertices[i] = { temp.x,temp.y,temp.z};
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

	void DetectCollisionSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
	{
	}

	void DetectCollisionSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
	{
	}

	void DetectCollisionSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
	{
	}


}
