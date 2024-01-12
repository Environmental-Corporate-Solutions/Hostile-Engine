//------------------------------------------------------------------------------
//
// File Name:	CollisionSys.cpp
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Engine.h"
#include "PhysicsSys.h"
#include "TransformSys.h"
#include "PhysicsProperties.h"

namespace Hostile {

	ADD_SYSTEM(CollisionSys);

	std::mutex CollisionSys::collisionDataMutex;
	std::vector<CollisionData> CollisionSys::collisionEvents;
	std::vector<TriggerEvent> CollisionSys::triggerEventQueue;
	std::unordered_set<std::pair<flecs::id_t, flecs::id_t>, PairHash> CollisionSys::currentTriggers;
	std::unordered_set<std::pair<flecs::id_t, flecs::id_t>, PairHash> CollisionSys::previousTriggers;

	void CollisionSys::OnCreate(flecs::world& _world)
	{
		_world.system("Collision PreUpdate")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter([this](flecs::iter const& _info){
					ClearCollisionData();
					currentTriggers.clear();
					triggerEventQueue.clear();
				});

		_world.system<Transform, SphereCollider>("TestSphereCollision")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter(TestSphereCollision);

		_world.system<Transform, BoxCollider>("TestBoxCollision")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter(TestBoxCollision);

		_world.system("ProcessTriggerEvents")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter([](flecs::iter& it) {
				CollisionSys::ProcessTriggerEvents();
				});

		_world.system("ResolveCollision")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter([&](flecs::iter& it)
				{
					float deltaTime = it.delta_time();
					ResolveCollisions(deltaTime);
				});

		_world.system<Transform, Rigidbody>("Integrate")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter(Integrate);

		REGISTER_TO_SERIALIZER(Rigidbody, this);
		REGISTER_TO_DESERIALIZER(Rigidbody, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"Rigidbody",
			std::bind(&CollisionSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) {
				_entity.add<Rigidbody>();
			});

		REGISTER_TO_SERIALIZER(PlaneCollider, this);
		REGISTER_TO_DESERIALIZER(PlaneCollider, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"PlaneCollider",
			std::bind(&CollisionSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<PlaneCollider>(); });

		REGISTER_TO_SERIALIZER(SphereCollider, this);
		REGISTER_TO_DESERIALIZER(SphereCollider, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"SphereCollider",
			std::bind(&CollisionSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<SphereCollider>(); });

		REGISTER_TO_SERIALIZER(BoxCollider, this);
		REGISTER_TO_DESERIALIZER(BoxCollider, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"BoxCollider",
			std::bind(&CollisionSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<BoxCollider>(); });
	}

	Vector3 CollisionSys::GetAxis(const Quaternion& orientation, int index) {
		if (index < 0 || index > 2) {
			Log::Error("GetAxis(): index out of bounds\n");
			throw std::runtime_error("GetAxis(): index out of bounds\n");
		}

		Matrix rotationMatrix = Matrix::CreateFromQuaternion(orientation);

		// Depending on the index, return the right, up, or forward vector
		Vector3 axis;
		switch (index) {
		case 0: // Right vector
			axis = Vector3(rotationMatrix._11, rotationMatrix._12, rotationMatrix._13);
			break;
		case 1: // Up vector
			axis = Vector3(rotationMatrix._21, rotationMatrix._22, rotationMatrix._23);
			break;
		case 2: // Forward vector
			axis = Vector3(rotationMatrix._31, rotationMatrix._32, rotationMatrix._33);
			break;
		}

		axis.Normalize();
		return axis;
	}

	//trigger
	void CollisionSys::UpdateTriggerState(flecs::id_t _triggerId, flecs::id_t _nonTriggerId)
	{
		std::pair<flecs::id_t, flecs::id_t> triggerPair(_triggerId, _nonTriggerId);

		if (previousTriggers.find(triggerPair) == previousTriggers.end()) 
		{
			triggerEventQueue.emplace_back(TriggerEvent::Type::Enter, _triggerId, _nonTriggerId);
		}
		else 
		{
			triggerEventQueue.emplace_back(TriggerEvent::Type::Stay, _triggerId, _nonTriggerId);
		}

		currentTriggers.insert(triggerPair);
	}

	void CollisionSys::ProcessTriggerEvents()
	{
		//test OnExit
		for (const auto& triggerPair : previousTriggers)
		{
			if (currentTriggers.find(triggerPair) == currentTriggers.end())
			{
				triggerEventQueue.emplace_back(TriggerEvent::Type::Exit, triggerPair.first, triggerPair.second);
			}
		}
		//semantics
		previousTriggers = std::move(currentTriggers);
		currentTriggers.clear();
	}

	bool CollisionSys::IsColliding(const Transform& _t1, const Transform& _t2, const Vector3& distVector, const float& radSum, float& distSqrd)
	{
		distSqrd = distVector.LengthSquared();
		return distSqrd <= (radSum * radSum);
	}
	bool CollisionSys::IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Transform& _tBox, const BoxCollider& _b)
	{
		return true;

	}
	bool CollisionSys::IsColliding(const Transform& _tSphere, const Vector3& _constraintNormal, float _offsetFromOrigin, float& _distance)
	{
		_distance = std::abs(_constraintNormal.Dot(_tSphere.position) + _offsetFromOrigin - PLANE_OFFSET);
		return _tSphere.scale.x * 0.5f > _distance;//assuming uniform x,y,and z
	}

	bool CollisionSys::IsColliding(const Transform& _t1, const BoxCollider& _b1, const Transform& _t2, const BoxCollider& _b2)
	{
		return true;
	}
	bool CollisionSys::IsColliding(const Transform& _tBox, const BoxCollider& _b, const PlaneCollider& _c)
	{
		return true;
	}

	float CollisionSys::CalcPenetration(const Transform& t1, const Transform& t2, const Vector3& colliderScale1, const Vector3& colliderScale2, const Vector3& colliderOffset1, const Vector3& colliderOffset2, const Vector3& axis) {
		Vector3 centerToCenter = t2.position - t1.position;
		Vector3 extents1 = t1.scale * 0.5f * colliderScale1;
		Vector3 extents2 = t2.scale * 0.5f * colliderScale2;
		float projectedCenterToCenter = abs(centerToCenter.Dot(axis));
		float projectedSum =
			abs((GetAxis(t1.orientation, 0) * extents1.x).Dot(axis))
			+ abs((GetAxis(t1.orientation, 1) * extents1.y).Dot(axis))
			+ abs((GetAxis(t1.orientation, 2) * extents1.z).Dot(axis))

			+ abs((GetAxis(t2.orientation, 0) * extents2.x).Dot(axis))
			+ abs((GetAxis(t2.orientation, 1) * extents2.y).Dot(axis))
			+ abs((GetAxis(t2.orientation, 2) * extents2.z).Dot(axis));

		return projectedSum - projectedCenterToCenter;
	}
	void CollisionSys::CalcOBBsContactPoints(const Transform& t1, const Transform& t2, CollisionData& newContact, int minPenetrationAxisIdx) {
		//I. vertex to face
		if (minPenetrationAxisIdx >= 0 && minPenetrationAxisIdx < 3)
		{
			Vector3 contactPoint = GetLocalContactVertex(newContact.collisionNormal, t2, std::less<float>());
			DirectX::SimpleMath::Vector4 temp = DirectX::SimpleMath::Vector4::Transform(DirectX::SimpleMath::Vector4{ contactPoint.x, contactPoint.y, contactPoint.z, 1.f }, t2.matrix);
			contactPoint = { temp.x,temp.y,temp.z };

			newContact.contactPoints = {
				{ contactPoint + newContact.collisionNormal * newContact.penetrationDepth },
				contactPoint
			};
		}
		else if (minPenetrationAxisIdx >= 3 && minPenetrationAxisIdx < 6) {
			Vector3 contactPoint = GetLocalContactVertex(newContact.collisionNormal, t1, std::greater<float>());

			DirectX::SimpleMath::Vector4 temp = DirectX::SimpleMath::Vector4::Transform(DirectX::SimpleMath::Vector4{ contactPoint.x, contactPoint.y, contactPoint.z, 1.f }, t1.matrix);
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

			edge1 = (vertexOneArr[testAxis1] < 0) ? GetAxis(t1.orientation, testAxis1) : GetAxis(t1.orientation, testAxis1) * -1.f;
			edge2 = (vertexTwoArr[testAxis2] < 0) ? GetAxis(t2.orientation, testAxis2) : GetAxis(t2.orientation, testAxis2) * -1.f;

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
	Vector3 CollisionSys::GetLocalContactVertex(Vector3 collisionNormal, const Transform& t, std::function<bool(const float&, const float&)> const cmp) {
		Vector3 contactPoint{ t.scale * 0.5f };
		if (cmp(GetAxis(t.orientation, 0).Dot(collisionNormal), 0)) {
			contactPoint.x = -contactPoint.x;
		}
		if (cmp(GetAxis(t.orientation, 1).Dot(collisionNormal), 0)) {
			contactPoint.y = contactPoint.y;
		}
		if (cmp(GetAxis(t.orientation, 2).Dot(collisionNormal), 0)) {
			contactPoint.z = -contactPoint.z;
		}
		return contactPoint;
	}

	void CollisionSys::TestBoxCollision(flecs::iter& _it, Transform* _transforms, BoxCollider* _boxes) {
		// Box vs. Box
		//auto boxEntities = _it.world().filter<Transform, BoxCollider>();
		//boxEntities.each([&](flecs::entity e1, Transform& t1, BoxCollider& b1) {
		const size_t Count = _it.count();
		for (size_t i = 0; i < Count; ++i) {
			Vector3 boxColliderScale1 = std::get<Vector3>(_boxes[i].GetScale());
			Vector3 boxColliderOffset1 = _boxes[i].GetOffset();
			Transform worldTransform1 = TransformSys::GetWorldTransform(_it.entity(i),boxColliderOffset1);

			for (size_t j = {i+1}; j < Count; ++j) {//j=0
			//boxEntities.each([&](flecs::entity e2, Transform& t2, BoxCollider& b2) {
				//if (i == j) continue; // Skip self-collision check

				Vector3 boxColliderScale2 = std::get<Vector3>(_boxes[j].GetScale());
				Vector3 boxColliderOffset2 = _boxes[j].GetOffset();
				Transform worldTransform2 = TransformSys::GetWorldTransform(_it.entity(j), boxColliderOffset2);

				bool isColliding{ true };
				std::vector<Vector3> axes;

				//"face(box1)" <-> vertex(box2)
				for (int k{}; k < 3; ++k) {
					auto axis = worldTransform1.orientation;
					axis.Normalize();
					axes.push_back(GetAxis(axis, k));
				}

				//"face(box2)" <-> vertex(box1)
				for (int k{}; k < 3; ++k) {
					auto axis = worldTransform2.orientation;
					axis.Normalize();
					axes.push_back(GetAxis(axis, k));
				}

				//edge-edge
				for (int p{}; p < 3; ++p) {
					for (int q{}; q < 3; ++q) {
						Vector3 crossProduct = axes[p].Cross(axes[3 + q]);

						crossProduct.Normalize();
						axes.push_back(crossProduct);
						//if (crossProduct.LengthSquared()>EPSILON*EPSILON) {  // Check if crossProduct is not a zero vector
						//}
					}
				}

				float minPenetration = FLT_MAX;
				int minAxisIdx = 0;
				const int AxesSize = axes.size();
				constexpr float EPSILON = 1e-5f;
				for (int k{}; k < AxesSize; ++k) {
					if (axes[k].LengthSquared() < EPSILON) { 
						continue;
					}

					float penetration = CalcPenetration(worldTransform1, worldTransform2, boxColliderScale1, boxColliderScale2,boxColliderOffset1, boxColliderOffset2, axes[k]);
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

				if (_boxes[i].m_isTrigger || _boxes[j].m_isTrigger)
				{
					//Log::Info("box-box trigger collision");
					flecs::id_t triggerEntityId = _boxes[i].m_isTrigger ? _it.entity(i).raw_id() : _it.entity(j).raw_id();
					flecs::id_t nonTriggerEntityId = _boxes[i].m_isTrigger ? _it.entity(j).raw_id(): _it.entity(i).raw_id();
					UpdateTriggerState(triggerEntityId, nonTriggerEntityId);
					return;
				}
 				CollisionData newContact;
				newContact.entity1 = _it.entity(i);
				newContact.entity2 = _it.entity(j);
				newContact.penetrationDepth = minPenetration;
				newContact.restitution = std::fmin(_boxes[i].m_restitution, _boxes[j].m_restitution);
				newContact.friction = 0.5f*(_boxes[i].m_friction+_boxes[j].m_friction);

				//vector pointing from the center of box2 to the center of box1
				Vector3 box2ToBox1 = worldTransform1.position - worldTransform2.position;

				//ensures the collisionNormal to always point from box2 towards box1.
				newContact.collisionNormal = (axes[minAxisIdx].Dot(box2ToBox1) < 0) ? -axes[minAxisIdx] : axes[minAxisIdx];
				//Log::Trace(newContact.penetrationDepth);

				CalcOBBsContactPoints(worldTransform1, worldTransform2, newContact, minAxisIdx);
				AddCollisionData(newContact);

				//Log::Debug("(detection) "+std::to_string(cnt++) + "th : " + std::to_string(_it.entity(i).raw_id()) + " <-> " + std::to_string(_it.entity(j).raw_id()));
				//Log::Debug("normal : " + std::to_string(newContact.collisionNormal.x) + ", " + std::to_string(newContact.collisionNormal.y) + ", " + std::to_string(newContact.collisionNormal.z));
				}

			//});
			}
		// Box vs. PlaneCollider
		auto constraints = _it.world().filter<PlaneCollider>();
		if (!constraints.count()) {
			return;
		}

		constraints.each([&](flecs::entity e, PlaneCollider& planeCollider) {
			const Transform* constraintTransform = e.get<Transform>();
			if (!constraintTransform) {
				return;
			}

			Vector3 constraintNormal = Vector3::Transform(UP_VECTOR, constraintTransform->orientation);
			constraintNormal.Normalize();
			float constraintOffsetFromOrigin = -constraintNormal.Dot(constraintTransform->position);

			for (int k = 0; k < _it.count(); ++k)
			{
				Vector3 vertices[8];
				Vector3 boxColliderScale = std::get<Vector3>(_boxes[k].GetScale());
				Vector3 boxColliderOffset = _boxes[k].GetOffset();
				Vector3 extents = Vector3{ 0.5f,0.5f,0.5f }*boxColliderScale-boxColliderOffset;

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

				Matrix inverseTransform = constraintTransform->matrix.Invert();

				for (int i = 0; i < 8; ++i)
				{
					float distance = constraintNormal.Dot(vertices[i]) + constraintOffsetFromOrigin;

					if (distance < PLANE_OFFSET)
					{
						Vector3 localCollisionPoint = Vector3::Transform(vertices[i], inverseTransform);

						// Check boundaries
						if ((localCollisionPoint.x < -0.5f || localCollisionPoint.x > 0.5f) ||
							(localCollisionPoint.z > 0.5f || localCollisionPoint.z < -0.5f))
						{
							continue;
						}

						if (_boxes[k].m_isTrigger)
						{
							//no trigger events between plane & triggers
							//UpdateTriggerState(e.raw_id(), _it.entity(k).raw_id());
							return;
						}


						CollisionData collisionData;
						collisionData.entity1 = _it.entity(k);
						collisionData.entity2 = e;
						collisionData.collisionNormal = constraintNormal;
						collisionData.contactPoints = {
						std::make_pair<Vector3,Vector3>(
							Vector3(vertices[i]),
							Vector3{}
						) };
						collisionData.penetrationDepth = PLANE_OFFSET - distance;
						collisionData.restitution = std::fmin(_boxes[k].m_restitution, planeCollider.m_restitution);//0.2
						collisionData.friction = 0.5f * (_boxes[k].m_friction +  planeCollider.m_friction);//0.6
						collisionData.accumulatedNormalImpulse = 0.f;
						AddCollisionData(collisionData);
					}
				}
			}
			});
	}
	void CollisionSys::TestSphereCollision(flecs::iter& _it, Transform* _transforms, SphereCollider* _spheres)
	{
		// fetch all entities with a Transform and SphereCollider ~
		auto sphereEntities = _it.world().filter<Transform, SphereCollider>();
		// iterate over each entity in the boxEntities filter ~
		sphereEntities.each([&](flecs::entity e1, Transform& t, SphereCollider& s) {
			if (e1.has<Rigidbody>() && e1.get<Rigidbody>()->m_isStatic) return; //forcing non-collidables to the second entity

			float colliderScale1 = std::get<float>(s.GetScale());
			Vector3 sphereColliderOffset1 = s.GetOffset();
			Transform sphereWorldTransform1 = TransformSys::GetWorldTransform(e1,sphereColliderOffset1);

			// Sphere vs. Sphere
			sphereEntities.each([&](flecs::entity e2, Transform& t2, SphereCollider& s2) {
				if (e1 == e2) return; // Skip self-collision check.

				float colliderScale2 = std::get<float>(s2.GetScale());
				Vector3 sphereColliderOffset2 = s2.GetOffset();
				Transform sphereWorldTransform2 = TransformSys::GetWorldTransform(e2, sphereColliderOffset2);

				float radSum = sphereWorldTransform1.scale.x * colliderScale1 + sphereWorldTransform2.scale.x * colliderScale2;

				radSum *= 0.5f;
				Vector3 distVector = sphereWorldTransform1.position - sphereWorldTransform2.position;
				float distSqrd = distVector.LengthSquared();

				if (distSqrd <= (radSum * radSum))
				{
					if (s.m_isTrigger || s2.m_isTrigger) //assuming non trigger vs trigger collision
					{
						flecs::id_t triggerEntityId = s.m_isTrigger ? e1.raw_id(): e2.raw_id();
						flecs::id_t nonTriggerEntityId = s.m_isTrigger ? e2.raw_id() : e1.raw_id();
						UpdateTriggerState(triggerEntityId, nonTriggerEntityId);
						return;
					}
					distVector.Normalize();

					CollisionData collisionData;
					collisionData.entity1 = e1;
					collisionData.entity2 = e2;
					collisionData.collisionNormal = distVector;
					collisionData.contactPoints = {
						std::make_pair<Vector3, Vector3>(
							sphereWorldTransform1.position - distVector * sphereWorldTransform1.scale.x* colliderScale1 * 0.5f,
							sphereWorldTransform2.position + distVector * sphereWorldTransform2.scale.x* colliderScale2 * 0.5f)
					};
					collisionData.penetrationDepth = radSum - sqrtf(distSqrd);
					collisionData.restitution = std::fmin(s.m_restitution, s2.m_restitution);//0.5
					collisionData.friction = 0.5f * (s.m_friction + s2.m_friction);//0.65
					collisionData.accumulatedNormalImpulse = 0.f;
					AddCollisionData(collisionData);
				}
				});
			});

		// Sphere vs. Box
		//not used the typical AABB method, which involves translating the sphere center to the box's local coords 
		//and clamping to find the nearest point to avoid calculating the inverse matrix every tick
		static constexpr int NUM_AXES = 3;
		_it.world().each<BoxCollider>([&_it, &_transforms, &_spheres](flecs::entity e, BoxCollider& box)
			{
				Vector3 boxColliderOffset = box.GetOffset();
				Transform boxTransform = TransformSys::GetWorldTransform(e, boxColliderOffset);
				Vector3 boxColliderScale = std::get<Vector3>(box.GetScale());

				const int Cnt = _it.count();
				for (int k = 0; k < Cnt; ++k)
				{
					Vector3 sphereColliderOffset = _spheres[k].GetOffset();
					Transform sphereTransform = TransformSys::GetWorldTransform(_it.entity(k),sphereColliderOffset);
					float sphereColliderScale = std::get<float>(_spheres[k].GetScale());

					float sphereRad = sphereTransform.scale.x*sphereColliderScale * 0.5f;
					Vector3 sphereCenter = sphereTransform.position;

					Vector3 centerToCenter = sphereCenter - boxTransform.position;
					Vector3 extents = boxTransform.scale*boxColliderScale * 0.5f;
					Vector3 closestPoint = boxTransform.position;

					//for the X,Y,Z axis

					for (int i = 0; i < NUM_AXES; ++i) {
						Vector3 axis = GetAxis(boxTransform.orientation, i);
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
					if (_spheres[k].m_isTrigger || box.m_isTrigger)
					{
						//Log::Info("sphere-box trigger collision");
						flecs::id_t triggerEntityId = _spheres[k].m_isTrigger ? _it.entity(k).raw_id(): e.raw_id();
						flecs::id_t nonTriggerEntityId = _spheres[k].m_isTrigger ? e.raw_id() : _it.entity(k).raw_id();
						UpdateTriggerState(triggerEntityId, nonTriggerEntityId);
						return;
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
					collisionData.restitution = std::fmin(box.m_restitution, _spheres[k].m_restitution);//0.5
					collisionData.friction = 0.5f * (box.m_friction + _spheres[k].m_friction);//0.6
					collisionData.accumulatedNormalImpulse = 0.f;
					AddCollisionData(collisionData);
				}
			});


		// Sphere vs. PlaneCollider
		auto constraints = _it.world().filter<PlaneCollider>();
		if (!constraints.count()) {
			return;
		}

		constraints.each([&](flecs::entity e, PlaneCollider& planeCollider) {
			const Transform* constraintTransform = e.get<Transform>();
			if (!constraintTransform) {
				return;
			}
			Vector3 constraintNormal = Vector3::Transform(UP_VECTOR, constraintTransform->orientation);
			constraintNormal.Normalize();
			float constraintOffsetFromOrigin = -constraintNormal.Dot(constraintTransform->position);

			for (int k = 0; k < _it.count(); ++k)
			{
				Vector3 sphereColliderOffset = _spheres[k].GetOffset();
				Transform sphereTransform = TransformSys::GetWorldTransform(_it.entity(k),sphereColliderOffset);
				float sphereColliderScale = std::get<float>(_spheres[k].GetScale());

				float distance = std::abs(constraintNormal.Dot(sphereTransform.position) + constraintOffsetFromOrigin) - PLANE_OFFSET;// -constraintNormal.Dot(Vector3{ 0.f,PLANE_OFFSET,0.f });
				if (sphereTransform.scale.x* sphereColliderScale * 0.5f > distance)//assuming uniform x,y,and z
				{
					//Vector3 collisionPoint = _transforms[k].position - constraintNormal * distance;
					Vector3 collisionPoint = sphereTransform.position - constraintNormal * distance;

					// Transform the collision point to the plane's local space
					Matrix inverseTransform = constraintTransform->matrix.Invert();
					Vector3 localCollisionPoint = Vector3::Transform(collisionPoint, inverseTransform);

					//check boundaries
					if ((localCollisionPoint.x < -0.5f || localCollisionPoint.x > 0.5f) ||
						(localCollisionPoint.z > 0.5f || localCollisionPoint.z < -0.5f))
					{
						continue;
					}

					if (_spheres[k].m_isTrigger)
					{
						//no trigger events between plane & triggers

						//UpdateTriggerState(e.raw_id(), _it.entity(k).raw_id());
						return;
					}

					CollisionData collisionData;
					collisionData.entity1 = _it.entity(k);
					collisionData.entity2 = e;
					collisionData.collisionNormal = constraintNormal;
					collisionData.contactPoints = {
						std::make_pair<Vector3,Vector3>(Vector3(sphereTransform.position - constraintNormal * distance),Vector3{})
					};
					collisionData.penetrationDepth = sphereTransform.scale.x* sphereColliderScale * 0.5f - distance;
					collisionData.restitution = std::fmin(_spheres[k].m_restitution, planeCollider.m_restitution);//0.18
					collisionData.friction = 0.5f * (_spheres[k].m_friction + planeCollider.m_friction);//0.65
					collisionData.accumulatedNormalImpulse = 0.f;
					AddCollisionData(collisionData);
				}
			}
			});
	}

	void CollisionSys::Integrate(flecs::iter& _it, Transform* _transform, Rigidbody* _rigidbody)
	{
		auto dt = _it.delta_time();
		size_t Cnt = _it.count();
		for (int i{}; i < Cnt; i++)
		{
			if (_rigidbody[i].m_isStatic)
			{
				continue;
			}

			// 1. linear Velocity
			Vector3 linearAcceleration = _rigidbody[i].m_force * _rigidbody[i].m_inverseMass;
			_rigidbody[i].m_linearVelocity += linearAcceleration * dt;
			_rigidbody[i].m_linearVelocity *= powf(_rigidbody[i].m_linearDamping, dt);

			// 2. angular Velocity
			Vector3 angularAcceleration = { _rigidbody[i].m_inverseInertiaTensorWorld * _rigidbody[i].m_torque };
			_rigidbody[i].m_angularVelocity += angularAcceleration * dt;
			//_rigidbody[i].m_angularVelocity *= powf(_rigidbody[i].m_angularDamping, dt);
			_rigidbody[i].m_angularVelocity *= powf(0.001f, dt);

			// apply axis locking
			if (_rigidbody[i].m_lockRotationX) _rigidbody[i].m_angularVelocity.x = 0.f;
			if (_rigidbody[i].m_lockRotationY) _rigidbody[i].m_angularVelocity.y = 0.f;
			if (_rigidbody[i].m_lockRotationZ) _rigidbody[i].m_angularVelocity.z = 0.f;

			// 3. calculate the new world position and orientation for the entity
			Transform worldTransform = TransformSys::GetWorldTransform(_it.entity(i));
			worldTransform.position += _rigidbody[i].m_linearVelocity * dt;

			Quaternion deltaRotation = Quaternion::Identity;
			if (_rigidbody[i].m_angularVelocity.LengthSquared() > FLT_EPSILON) {
				Vector3 angularVelocityNormalized = _rigidbody[i].m_angularVelocity;//
				angularVelocityNormalized.Normalize();
				float angularSpeed = _rigidbody[i].m_angularVelocity.Length();
				deltaRotation = Quaternion::CreateFromAxisAngle(angularVelocityNormalized, angularSpeed * dt);
			}
			worldTransform.orientation = deltaRotation * worldTransform.orientation; // World orientation update
			worldTransform.orientation.Normalize();

			// 4. if the entity has a parent, calculate the local transform
			if (_it.entity(i).parent().is_valid() && !_it.entity(i).parent().has<IsScene>()) {
				Transform parentWorldTransform = TransformSys::GetWorldTransform(_it.entity(i).parent());

				// Convert world position to parent-relative position
				Vector3 relativePosition = Vector3::Transform(worldTransform.position, parentWorldTransform.matrix.Invert());

				// Convert world orientation to parent-relative orientation
				Quaternion parentInverseOrientation;
				parentWorldTransform.orientation.Inverse(parentInverseOrientation);
				Quaternion relativeOrientation = parentInverseOrientation*worldTransform.orientation;

				// Update the local transform of the entity
				_transform[i].position = relativePosition;
				_transform[i].orientation = relativeOrientation;
			}
			else {
				// If there's no parent, the entity's transform is the world transform
				_transform[i] = worldTransform;
			}

			// 5. update inertia tensor
			Matrix3 rotationMatrix;
			Matrix mat = XMMatrixRotationQuaternion(_transform[i].orientation);

			for (int col = 0; col < 3; ++col) {//decompose
				for (int row = 0; row < 3; ++row) {
					rotationMatrix[row * 3 + col] = mat.m[row][col];
				}
			}

			_rigidbody[i].m_inverseInertiaTensorWorld
				= (_rigidbody[i].m_inverseInertiaTensor * rotationMatrix) * rotationMatrix.Transpose();

			// 6. clear
			_rigidbody[i].m_force = Vector3::Zero;
			_rigidbody[i].m_torque = Vector3::Zero;
		}
	}

	//mutex - working on it
	void CollisionSys::AddCollisionData(const CollisionData& data) 
	{
		//std::lock_guard<std::mutex> lock(collisionDataMutex);
		collisionEvents.push_back(data);
	}

	void CollisionSys::ClearCollisionData() 
	{
		//std::lock_guard<std::mutex> lock(collisionDataMutex);
		collisionEvents.clear();
	}

	void CollisionSys::ResolveCollisions(float dt)
	{
		if (collisionEvents.empty() == true) 
		{
			return;
		}
		//static int cnt = 0;
		//Log::Debug("(resolusion) " + std::to_string(cnt++) + "th");

		for (int iter{}; iter < SOLVER_ITERS; ++iter)
		{
			for (auto& collision : collisionEvents) 
			{
				flecs::entity e1 = collision.entity1;
				flecs::entity e2 = collision.entity2;
				bool isResolvableE1 = e1.has<Rigidbody>();
				bool isResolvableE2 = e2.has<Rigidbody>();
				Rigidbody* rb1 = isResolvableE1 ? e1.get_mut<Rigidbody>() : nullptr;
				Rigidbody* rb2 = isResolvableE2 ? e2.get_mut<Rigidbody>() : nullptr;
				if (rb1)
				{
					isResolvableE1 = rb1->m_isStatic == false;
				}
				if (rb2)
				{
					isResolvableE2 = rb2->m_isStatic == false;
				}
				std::optional<Transform> t1;
				std::optional<Transform> t2;

				if (isResolvableE1)
				{
					t1 = TransformSys::GetWorldTransform(e1);
				}

				if (isResolvableE2)
				{
					t2 = TransformSys::GetWorldTransform(e2);
				}
				float inverseMassSum = t1 ? rb1->m_inverseMass : 0.f;

				if (t2.has_value())
				{
					if (rb2->m_isStatic==false) 
					{
						inverseMassSum += rb2->m_inverseMass;
					}
					else 
					{
						inverseMassSum = 0.f;
					}
				}
				//if (fabs(inverseMassSum) < FLT_EPSILON) {
				//	continue;
				//}

				// Contact point relative to the body's position
				Vector3 r1;
				Vector3 r2;
				if (t1.has_value())
				{
					r1 = collision.contactPoints.first - t1->position;
				}
				if (t2.has_value()) {
					r2 = collision.contactPoints.second - t2->position;
				}

				// Inverse inertia tensors
				Matrix3 i1{};
				Matrix3 i2{};
				if (t1.has_value() && rb1->m_isStatic==false)
				{
					i1 = rb1->m_inverseInertiaTensorWorld;
				}
				if (t2.has_value() && rb2->m_isStatic==false)
				{
					i2 = rb2->m_inverseInertiaTensorWorld;
				}

				// Denominator terms                
				Vector3 termInDenominator1;
				Vector3 termInDenominator2;
				if (t1.has_value())
				{
					termInDenominator1 = (i1 * r1.Cross(collision.collisionNormal)).Cross(r1);
				}
				if (t2.has_value())
				{
					termInDenominator2 = (i2 * r2.Cross(collision.collisionNormal)).Cross(r2);
				}

				// Compute the final effective mass
				float effectiveMass = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(collision.collisionNormal);

				if (fabs(effectiveMass) < FLT_EPSILON) {
					return;
				}


				// Relative velocities
				Vector3 relativeVel{};
				if (t1.has_value() && rb1->m_isStatic==false)
				{
					relativeVel = rb1->m_linearVelocity + (ExtractRotationMatrix(t1->matrix) * rb1->m_angularVelocity).Cross(r1);
				}
				if (t2.has_value() && rb2->m_isStatic==false)
				{
					relativeVel -= rb2->m_linearVelocity + (ExtractRotationMatrix(t2->matrix) * rb2->m_angularVelocity).Cross(r2);
				}

				//less sensitive
				static constexpr float RELATIVE_SPEED_SENSITIVITY = 0.1f;
				float relativeSpeed = relativeVel.Dot(collision.collisionNormal)*RELATIVE_SPEED_SENSITIVITY;

				static constexpr float PENETRATION_TOLERANCE = 0.0005f;

				static constexpr float CORRECTION_RATIO = 0.18f;
				// Baumgarte Stabilization (for penetration resolution)
				float baumgarte = 0.0f;
				if (collision.penetrationDepth > PENETRATION_TOLERANCE) {
					baumgarte = static_cast<float>(
						(collision.penetrationDepth - PENETRATION_TOLERANCE) * (CORRECTION_RATIO / dt)
						);
				}

				static constexpr float CLOSING_SPEED_TOLERANCE = 0.001f; 
				float restitutionTerm = 0.0f;
				if (relativeSpeed > CLOSING_SPEED_TOLERANCE) {
					restitutionTerm = collision.restitution * (relativeSpeed - CLOSING_SPEED_TOLERANCE);
				}

				// Compute the impulse
				float jacobianImpulse = ((-(1 + restitutionTerm) * relativeSpeed) + baumgarte) / effectiveMass;
				if (isnan(jacobianImpulse)) {
					Log::Debug("impulse NAN");
					return;
				}

				// Compute the total impulse applied so far to maintain non-penetration
				float prevImpulseSum = collision.accumulatedNormalImpulse;
				collision.accumulatedNormalImpulse += jacobianImpulse;
				if (collision.accumulatedNormalImpulse < 0.0f) {//std::max
					collision.accumulatedNormalImpulse = 0.0f;
				}

				jacobianImpulse = collision.accumulatedNormalImpulse - prevImpulseSum;


				// apply impulses to the bodies
				ApplyImpulses(e1, e2, jacobianImpulse, r1, r2, collision.collisionNormal, rb1, rb2, t1, t2);

				// compute and apply frictional impulses using the two tangents
				ApplyFrictionImpulses(e1, e2, r1, r2, collision, rb1, rb2, t1, t2);
			}
		}
	}


	void CollisionSys::ApplyImpulses(flecs::entity e1, flecs::entity e2, float jacobianImpulse, const Vector3& r1, const Vector3& r2, const Vector3& direction, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2) {
		Vector3 linearImpulse = direction * jacobianImpulse;
		if (_t1.has_value() && _rb1->m_isStatic==false)
		{
			Vector3 angularImpulse1 = r1.Cross(direction) * jacobianImpulse;
			Vector3 linearVelDelta = linearImpulse * _rb1->m_inverseMass;
			_rb1->m_linearVelocity += linearVelDelta;

			Vector3 angVelDelta = _rb1->m_inverseInertiaTensorWorld * angularImpulse1;
			Vector3 localAngularVel = (ExtractRotationMatrix(_t1->matrix) * _rb1->m_angularVelocity) + angVelDelta;
			_rb1->m_angularVelocity = ExtractRotationMatrix(_t1->matrix).Transpose() * localAngularVel;


			//Log::Debug(std::to_string(e1.raw_id()) + " : (linear vel) "
			//	+ std::to_string(linearVelDelta.x) + ", "
			//	+ std::to_string(linearVelDelta.y) + ", "
			//	+ std::to_string(linearVelDelta.z)
			//);

			//angVelDelta = ExtractRotationMatrix(_t1->matrix).Transpose() * angVelDelta;
			//Log::Debug(std::to_string(e1.raw_id()) + " : (ang vel) "
			//	+ std::to_string(angVelDelta.x) + ", "
			//	+ std::to_string(angVelDelta.y) + ", "
			//	+ std::to_string(angVelDelta.z)
			//);
		}

		if (_t2.has_value() && _rb2->m_isStatic==false) {
			Vector3 angularImpulse2 = r2.Cross(direction) * jacobianImpulse;
			Vector3 linearVelDelta = linearImpulse * _rb2->m_inverseMass;
			_rb2->m_linearVelocity -= linearVelDelta;

			Vector3 angVelDelta = _rb2->m_inverseInertiaTensorWorld * angularImpulse2;
			Vector3 localAngularVel = (ExtractRotationMatrix(_t2->matrix) * _rb2->m_angularVelocity) - angVelDelta;
			_rb2->m_angularVelocity = ExtractRotationMatrix(_t2->matrix).Transpose() * localAngularVel;

			//Log::Debug(std::to_string(e2.raw_id()) + " : (linear vel) "
			//	+ std::to_string(-linearVelDelta.x) + ", "
			//	+ std::to_string(-linearVelDelta.y) + ", "
			//	+ std::to_string(-linearVelDelta.z)
			//);

			//angVelDelta = ExtractRotationMatrix(_t2->matrix).Transpose() * angVelDelta;
			//Log::Debug(std::to_string(e2.raw_id()) + " : (ang vel) "
			//	+ std::to_string(-angVelDelta.x) + ", "
			//	+ std::to_string(-angVelDelta.y) + ", "
			//	+ std::to_string(-angVelDelta.z)
			//);
		}
	}

	float CollisionSys::ComputeTangentialImpulses(const flecs::entity& _e1, const flecs::entity& _e2, const Vector3& _r1, const Vector3& _r2, const Vector3& _tangent, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2, const CollisionData& _collision) {

		float inverseMassSum{};
		Vector3 termInDenominator1{};
		if (_rb1)
		{
			if (_rb1->m_isStatic == false) 
			{
				inverseMassSum = _rb1->m_inverseMass;
			}
			termInDenominator1 = (_rb1->m_inverseInertiaTensorWorld * _r1.Cross(_tangent)).Cross(_r1);
		}

		Vector3 termInDenominator2{};
		if (_rb2)
		{
			if (_rb2->m_isStatic == false) 
			{
				inverseMassSum += _rb2->m_inverseMass;
			}
			termInDenominator2 = (_rb2->m_inverseInertiaTensorWorld * _r2.Cross(_tangent)).Cross(_r2);
		}

		// Compute the effective mass for the friction/tangential direction
		float effectiveMassTangential = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(_tangent);
		if (fabs(effectiveMassTangential) < FLT_EPSILON) {
			return 0.f;
		}

		// Calculate relative velocities along the tangent
		Vector3 relativeVel{};
		if (_t1.has_value() && _rb1->m_isStatic==false)
		{
			relativeVel = _rb1->m_linearVelocity + (ExtractRotationMatrix(_t1->matrix) * _rb1->m_angularVelocity).Cross(_r1);
		}
		if (_t2.has_value() && _rb2->m_isStatic==false)
		{
			relativeVel -= (_rb2->m_linearVelocity + (ExtractRotationMatrix(_t2->matrix) * _rb2->m_angularVelocity).Cross(_r2));
		}

		float relativeSpeedTangential = relativeVel.Dot(_tangent);

		//evenly distributed max friction for each iter
		static constexpr float MAX_FRICTION = 0.05f/SOLVER_ITERS;

		// Compute the frictional impulse
		float frictionImpulse = -relativeSpeedTangential / effectiveMassTangential;
		frictionImpulse = std::clamp(frictionImpulse, -MAX_FRICTION, MAX_FRICTION);

		// Clamp based on Coulomb's law
		//Log::Debug("friction = " + std::to_string(frictionImpulse));
		float maxFriction = _collision.friction * _collision.accumulatedNormalImpulse;

		//Log::Debug("max friction = "+std::to_string(maxFriction));
		frictionImpulse = std::clamp(frictionImpulse, -maxFriction, maxFriction);
		

		return frictionImpulse;
	}

	//void CollisionSys::ApplyFrictionImpulses(flecs::entity e1, flecs::entity e2, const Vector3& r1, const Vector3& r2, const CollisionData& _collision, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2)
	//{
	//	Vector3 tangent1, tangent2;

	//	//erin catto - Box2D
	//	if (abs(_collision.collisionNormal.x) >= 0.57735f) {
	//		tangent1 = Vector3(_collision.collisionNormal.y, -_collision.collisionNormal.x, 0.0f);
	//	}
	//	else {
	//		tangent1 = Vector3(0.0f, _collision.collisionNormal.z, -_collision.collisionNormal.y);
	//	}
	//	tangent1.Normalize();
	//	tangent2 = _collision.collisionNormal.Cross(tangent1);
	//	tangent2.Normalize();

	//	// Compute the impulses in each direction and apply
	//	float jacobianImpulseT1 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent1, _rb1, _rb2, _t1, _t2, _collision);
	//	ApplyImpulses(e1, e2, jacobianImpulseT1, r1, r2, tangent1, _rb1, _rb2, _t1, _t2);

	//	float jacobianImpulseT2 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent2, _rb1, _rb2, _t1, _t2, _collision);
	//	ApplyImpulses(e1, e2, jacobianImpulseT2, r1, r2, tangent2, _rb1, _rb2, _t1, _t2);
	//}
	void CollisionSys::ApplyFrictionImpulses(flecs::entity e1, flecs::entity e2, const Vector3& r1, const Vector3& r2, const CollisionData& _collision, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2)
	{
		Vector3 collisionNormal = _collision.collisionNormal;
		collisionNormal.Normalize();

		Vector3 tangent1, tangent2;

		// select an arbitrary vector that is not aligned with the collision normal
		Vector3 arbitraryVec;

		if (fabs(collisionNormal.x) < 0.57735f) {
			arbitraryVec = Vector3(1.0f, 0.0f, 0.0f);
		}
		else {
			arbitraryVec = Vector3(0.0f, 1.0f, 0.0f);
		}

		// using Gram-Schmidt to obtain orthogonal vectors
		tangent1 = (arbitraryVec - collisionNormal * arbitraryVec.Dot(collisionNormal));
		tangent1.Normalize();
		tangent2 = collisionNormal.Cross(tangent1);
		tangent2.Normalize();

		// compute the impulses in each tangential direction
		float jacobianImpulseT1 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent1, _rb1, _rb2, _t1, _t2, _collision);
		ApplyImpulses(e1, e2, jacobianImpulseT1, r1, r2, tangent1, _rb1, _rb2, _t1, _t2);

		float jacobianImpulseT2 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent2, _rb1, _rb2, _t1, _t2, _collision);
		ApplyImpulses(e1, e2, jacobianImpulseT2, r1, r2, tangent2, _rb1, _rb2, _t1, _t2);
	}

	void CollisionSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
	{
		using namespace nlohmann;
		using namespace Hostile;

		if (type == "BoxCollider")
		{
			if (auto* collider = _entity.get<BoxCollider>(); collider)
			{
				json obj = json::object();
				obj["Type"] = "BoxCollider";
				obj["IsTrigger"] = collider->m_isTrigger;
				_components.push_back(obj);
			}
		}
		else if (type == "SphereCollider")
		{
			if (auto* collider = _entity.get<SphereCollider>(); collider)
			{
				json obj = json::object();
				obj["Type"] = "SphereCollider";
				obj["IsTrigger"] = collider->m_isTrigger;
				_components.push_back(obj);
			}
		}
		else if (type == "PlaneCollider")
		{
			if (auto* collider = _entity.get<PlaneCollider>(); collider)
			{
				json obj = json::object();
				obj["Type"] = "PlaneCollider";
				obj["IsTrigger"] = collider->m_isTrigger;
				_components.push_back(obj);
			}
		}
		else if (type == "Rigidbody")
		{
			if (auto* body = _entity.get<Rigidbody>(); body)
			{
				json obj = {
					{"Type", "Rigidbody"},
					{"InverseMass", body->m_inverseMass},
					{"LinearVelocity", WriteVec3(body->m_linearVelocity)},
					{"LinearAcceleration", WriteVec3(body->m_linearAcceleration)},
					{"AngularVelocity", WriteVec3(body->m_angularVelocity)},
					{"AngularAcceleration", WriteVec3(body->m_angularAcceleration)},
					{"Force", WriteVec3(body->m_force)},
					{"Torque", WriteVec3(body->m_torque)},
					{"LinearDamping", body->m_linearDamping},
					{"AngularDamping", body->m_angularDamping},
					{"UseGravity", body->m_useGravity},
					{"IsStatic", body->m_isStatic},
					{"InverseInertiaTensor", WriteMat3(body->m_inverseInertiaTensor)},
					{"InverseInertiaTensorWorld", WriteMat3(body->m_inverseInertiaTensorWorld)}
				};
				_components.push_back(obj);
			}
		}
	}

	void CollisionSys::Read(flecs::entity& _entity, nlohmann::json& _data, const std::string& type)
	{
		using namespace nlohmann;
		using namespace Hostile;

		if (type == "BoxCollider")
		{
			_entity.add<BoxCollider>();
			BoxCollider* collider = _entity.get_mut<BoxCollider>();
			if (collider) {
				collider->m_isTrigger = _data.value("IsTrigger", false);
			}
		}
		else if (type == "SphereCollider")
		{
			_entity.add<SphereCollider>();
			SphereCollider* collider = _entity.get_mut<SphereCollider>();
			if (collider) {
				collider->m_isTrigger = _data.value("IsTrigger", false);
			}
		}
		else if (type == "PlaneCollider")
		{
			_entity.add<PlaneCollider>();
			PlaneCollider* collider = _entity.get_mut<PlaneCollider>();
			if (collider) {
				collider->m_isTrigger = _data.value("IsTrigger", false);
			}
		}
		else if (type == "Rigidbody")
		{
			_entity.add<Rigidbody>();
			Rigidbody* body = _entity.get_mut<Rigidbody>();
			if (body) {
				body->m_inverseMass = _data.value("InverseMass", 0.0f);
				body->m_linearVelocity = ReadVec3(_data["LinearVelocity"]);
				body->m_linearAcceleration = ReadVec3(_data["LinearAcceleration"]);
				body->m_angularVelocity = ReadVec3(_data["AngularVelocity"]);
				body->m_angularAcceleration = ReadVec3(_data["AngularAcceleration"]);
				body->m_force = ReadVec3(_data["Force"]);
				body->m_torque = ReadVec3(_data["Torque"]);
				body->m_linearDamping = _data.value("LinearDamping", 0.0f);
				body->m_angularDamping = _data.value("AngularDamping", 0.0f);
				body->m_useGravity = _data.value("UseGravity", true);
				body->m_isStatic = _data.value("IsStatic", false);
				body->m_inverseInertiaTensor = ReadMat3(_data["InverseInertiaTensor"]);
				body->m_inverseInertiaTensorWorld = ReadMat3(_data["InverseInertiaTensorWorld"]);
			}
		}
	}

	void CollisionSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
	{
		if (type == "Rigidbody")
		{
			bool hasRigidbody = _entity.has<Rigidbody>();
			bool is_open = ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen);
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				ImGui::OpenPopup("Rigidbody Popup");
			}
			if (ImGui::BeginPopup("Rigidbody Popup"))
			{
				if (ImGui::Button("Remove Component"))
				{
					_entity.remove<Rigidbody>();
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}
				ImGui::EndPopup();
			}
			if (is_open && hasRigidbody)
			{
				Rigidbody* rb = _entity.get_mut<Rigidbody>();
				
				float mass = rb->m_inverseMass != 0.0f ? 1.0f / rb->m_inverseMass : FLT_MAX;

				// Display and edit mass
				if (ImGui::DragFloat("Mass", &mass, 0.01f, 0.5f, 20.f, "%.3f"))
				{
					rb->m_inverseMass = mass != 0.0f ? 1.0f / mass : 0.0f; // Update inverse mass
				}

				ImGui::DragFloat3("Vel", &rb->m_linearVelocity.x, 0.1f);
				ImGui::DragFloat3("Acc", &rb->m_linearAcceleration.x, 0.1f);
				ImGui::DragFloat3("Force", &rb->m_force.x, 0.1f);
				ImGui::DragFloat3("Torque", &rb->m_torque.x, 0.1f);
				ImGui::DragFloat("Glide", &rb->m_linearDamping, 0.01f, 0.0f, 1.f, "%.3f");
				ImGui::DragFloat("Spin", &rb->m_angularDamping, 0.01f, 0.0f, 1.f, "%.3f");
				ImGui::Checkbox("Use Gravity", &rb->m_useGravity);
				ImGui::Checkbox("Is Static", &rb->m_isStatic);
			
				ImGui::Checkbox("Lock Rotation X", &rb->m_lockRotationX);
				ImGui::Checkbox("Lock Rotation Y", &rb->m_lockRotationY);
				ImGui::Checkbox("Lock Rotation Z", &rb->m_lockRotationZ);
				ImGui::Text("");
			}
		}
		else if (type == "BoxCollider" || type == "SphereCollider" || type == "PlaneCollider")
		{
			// General code for all colliders
			bool hasCollider = (type == "BoxCollider" && _entity.has<BoxCollider>()) ||
				(type == "SphereCollider" && _entity.has<SphereCollider>()) ||
				(type == "PlaneCollider" && _entity.has<PlaneCollider>());

			bool isOpen = ImGui::CollapsingHeader(type.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				ImGui::OpenPopup("Collider Popup");
			}
			if (ImGui::BeginPopup("Collider Popup"))
			{
				if (ImGui::Button("Remove Component"))
				{
					if (type == "BoxCollider") _entity.remove<BoxCollider>();
					else if (type == "SphereCollider") _entity.remove<SphereCollider>();
					else if (type == "PlaneCollider") _entity.remove<PlaneCollider>();
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}
				ImGui::EndPopup();
			}
			if (isOpen)
			{
				if (hasCollider)
				{
					Collider* collider = nullptr;
					if (type == "BoxCollider") 
					{
						collider = _entity.get_mut<BoxCollider>();
						ImGui::DragFloat3("scale", & dynamic_cast<BoxCollider*>(collider)->m_scale.x, 0.1f);
					}
					else if (type == "SphereCollider") 
					{
						collider = _entity.get_mut<SphereCollider>();
						ImGui::DragFloat("scale", &dynamic_cast<SphereCollider*>(collider)->radius, 0.1f);
					}
					else if (type == "PlaneCollider") 
					{
						collider = _entity.get_mut<PlaneCollider>();
					}

					if (collider)
					{
						ImGui::DragFloat("restitution", &collider->m_restitution, 0.1f, 0.0f, 1.0f, "%.2f");
						ImGui::DragFloat("friction", &collider->m_friction, 0.1f, 0.0f, 1.0f, "%.2f");
						ImGui::DragFloat3("offset", &collider->m_offset.x, 0.1f);
						ImGui::Checkbox("Is Trigger", &collider->m_isTrigger);
					}
				}
				ImGui::Text("");
			}
		}
	}
}