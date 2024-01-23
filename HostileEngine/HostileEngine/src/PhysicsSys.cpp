//------------------------------------------------------------------------------
//
// File Name:	PhysicsSys.cpp
// Author(_s):	byeonggyu.park
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

namespace Hostile 
{

	ADD_SYSTEM(PhysicsSys);

	float PhysicsSys::m_accumulatedTime = 0;
	std::vector<CollisionData> PhysicsSys::m_collisionData;
	std::unordered_map<flecs::id_t, std::set<flecs::id_t>> PhysicsSys::m_previousFrameCollisions;
	std::unordered_map<flecs::id_t, std::set<flecs::id_t>> PhysicsSys::m_currentFrameCollisions;


	PhysicsSys::PhysicsSys()
	{
		m_collisionData.reserve(300);
	}

	/**
	 * Configures the physics system within the ECS framework.
	 * Sets up various systems for collision detection, resolution, and other physics-related tasks.
	 * @param _world Reference to the ECS world to which the system is being added.
	 */
	void PhysicsSys::OnCreate(flecs::world& _world)
	{
		_world.system("Collision PreUpdate")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter([this](flecs::iter const& _info){
					ClearCollisionData();
				});
		_world.system<Transform, BoxCollider>("TestBoxCollision")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter(TestBoxCollision);

		_world.system<Transform, SphereCollider>("TestSphereCollision")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter(TestSphereCollision);

		_world.system("ProcessCollisionEvents")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter([](flecs::iter& _it) 
				{
					PhysicsSys::UpdateCollisionEvents();
				});

		_world.system("ResolveCollision")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter([&](flecs::iter& _it)
				{
					float deltaTime = _it.delta_time();
					m_accumulatedTime += deltaTime;
					while (m_accumulatedTime >= PHYSICS_UPDATE_TARGET_FPS_INV) {
						ResolveCollisions();
						m_accumulatedTime -= deltaTime;
					}
				});

		_world.system<Transform, Rigidbody>("Integrate")
			.kind(IEngine::Get().GetPhysicsPhase())
			.iter(Integrate);

		REGISTER_TO_SERIALIZER(Rigidbody, this);
		REGISTER_TO_DESERIALIZER(Rigidbody, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"Rigidbody",
			std::bind(&PhysicsSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) {
				_entity.add<Rigidbody>();
			});

		REGISTER_TO_SERIALIZER(PlaneCollider, this);
		REGISTER_TO_DESERIALIZER(PlaneCollider, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"PlaneCollider",
			std::bind(&PhysicsSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<PlaneCollider>(); });

		REGISTER_TO_SERIALIZER(SphereCollider, this);
		REGISTER_TO_DESERIALIZER(SphereCollider, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"SphereCollider",
			std::bind(&PhysicsSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<SphereCollider>(); });

		REGISTER_TO_SERIALIZER(BoxCollider, this);
		REGISTER_TO_DESERIALIZER(BoxCollider, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"BoxCollider",
			std::bind(&PhysicsSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<BoxCollider>(); });
	}
	
	/**
	 * Extracts one of the primary axes (right, up, or forward) from a quaternion.
	 * @param _orientation Quaternion representing an object's orientation.
	 * @param _index Index specifying which axis to extract (0 for right, 1 for up, and 2 for forward).
	 * @return The requested axis as a Vector3.
	 */
	Vector3 PhysicsSys::GetAxis(const Quaternion& _orientation, int _index) 
	{
		if (_index < 0 || _index > 2) {
			Log::Error("GetAxis(): index out of bounds\n");
			throw std::runtime_error("GetAxis(): index out of bounds\n");
		}

		Matrix rotationMatrix = Matrix::CreateFromQuaternion(_orientation);

		// Depending on the index, return the right, up, or forward vector
		Vector3 axis;
		switch (_index) {
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

	/**
	 * Manages and updates collision event data between frames.
	 * Handles the transition of collision states from one frame to the next.
	 */
	void PhysicsSys::UpdateCollisionEvents()
	{
		for (const auto& prevPair : m_previousFrameCollisions) 
		{
			flecs::id_t entity = prevPair.first;
			if (m_currentFrameCollisions.find(entity) == m_currentFrameCollisions.end()) 
			{
				// this entity had collisions previously but is not colliding in the current frame
				for (flecs::id_t prevCollidingEntity : prevPair.second) {
					HandleCollisionEnd(entity, prevCollidingEntity);
				}
			}
		}

		// update previous frame collisions for the next frame
		m_previousFrameCollisions = m_currentFrameCollisions;
		m_currentFrameCollisions.clear();
	}

	bool PhysicsSys::IsColliding(const Transform& _t1, const Transform& _t2, const Vector3& _distVector, const float& _radSum, float& _distSqrd)
	{
		_distSqrd = _distVector.LengthSquared();
		return _distSqrd <= (_radSum * _radSum);
	}
	bool PhysicsSys::IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Transform& _tBox, const BoxCollider& _b)
	{
		return true;
	}
	bool PhysicsSys::IsColliding(const Transform& _tSphere, const Vector3& _constraintNormal, float _offsetFromOrigin, float& _distance)
	{
		_distance = std::abs(_constraintNormal.Dot(_tSphere.position) + _offsetFromOrigin - PLANE_OFFSET);
		return _tSphere.scale.x * 0.5f > _distance;//assuming uniform x,y,and z
	}
	bool PhysicsSys::IsColliding(const Transform& _t1, const BoxCollider& _b1, const Transform& _t2, const BoxCollider& _b2)
	{
		return true;
	}
	bool PhysicsSys::IsColliding(const Transform& _tBox, const BoxCollider& _b, const PlaneCollider& _c)
	{
		return true;
	}

	/**
	 * Calculates the penetration depth between two colliding OBBs.
	 * Used in collision resolution to adjust positions and apply forces.
	 * @return The calculated penetration depth.
	 */
	float PhysicsSys::CalcPenetration(const Transform& _t1, const Transform& _t2, const Vector3& _colliderScale1, const Vector3& _colliderScale2, const Vector3& _colliderOffset1, const Vector3& _colliderOffset2, const Vector3& _axis) 
	{
		Vector3 centerToCenter = _t2.position - _t1.position;
		Vector3 extents1 = _t1.scale * 0.5f * _colliderScale1;
		Vector3 extents2 = _t2.scale * 0.5f * _colliderScale2;
		float projectedCenterToCenter = abs(centerToCenter.Dot(_axis));
		float projectedSum =
			  abs((GetAxis(_t1.orientation, 0) * extents1.x).Dot(_axis))
			+ abs((GetAxis(_t1.orientation, 1) * extents1.y).Dot(_axis))
			+ abs((GetAxis(_t1.orientation, 2) * extents1.z).Dot(_axis))

			+ abs((GetAxis(_t2.orientation, 0) * extents2.x).Dot(_axis))
			+ abs((GetAxis(_t2.orientation, 1) * extents2.y).Dot(_axis))
			+ abs((GetAxis(_t2.orientation, 2) * extents2.z).Dot(_axis));

		return projectedSum - projectedCenterToCenter;
	}
	
	/**
	 * Calculates the exact contact points between two colliding OBBs.
	 * Essential for accurate impulse calculations during collision resolution.
	 * @param _t1 Transform of the first OBB.
	 * @param _t2 Transform of the second OBB.
	 * @param _newContact Reference to CollisionData to store calculated contact points.
	 * @param _minPenetrationAxisIdx Index of the axis with minimum penetration.
	 */
	void PhysicsSys::CalcOBBsContactPoints(const Transform& _t1, const Transform& _t2, CollisionData& _newContact, int _minPenetrationAxisIdx) 
	{
		//I.
		if (_minPenetrationAxisIdx >= 0 && _minPenetrationAxisIdx < 3)
		{
			Vector3 contactPoint = GetLocalContactVertex(_newContact.collisionNormal, _t2, std::less<float>());
			DirectX::SimpleMath::Vector4 temp = DirectX::SimpleMath::Vector4::Transform(DirectX::SimpleMath::Vector4{ contactPoint.x, contactPoint.y, contactPoint.z, 1.f }, _t2.matrix);
			contactPoint = { temp.x,temp.y,temp.z };

			_newContact.contactPoints = {
				{ contactPoint + _newContact.collisionNormal * _newContact.penetrationDepth },
				contactPoint
			};
		}
		else if (_minPenetrationAxisIdx >= 3 && _minPenetrationAxisIdx < 6) 
		{
   			Vector3 contactPoint = GetLocalContactVertex(_newContact.collisionNormal, _t1, std::greater<float>());

			DirectX::SimpleMath::Vector4 temp = DirectX::SimpleMath::Vector4::Transform(DirectX::SimpleMath::Vector4{ contactPoint.x, contactPoint.y, contactPoint.z, 1.f }, _t1.matrix);
			contactPoint = { temp.x,temp.y,temp.z };

			_newContact.contactPoints = {
				contactPoint,
				{ contactPoint - _newContact.collisionNormal * _newContact.penetrationDepth }
			};
		}
		//II. edge to edge
		else //need further updates
		{
			// Determine the local contact vertex on box1 based on the collision'_s hit normal.
			Vector3 vertexOne = GetLocalContactVertex(_newContact.collisionNormal, _t1, std::greater<float>());
			Vector3 vertexTwo = GetLocalContactVertex(_newContact.collisionNormal, _t2, std::less<float>());

			static int EDGES_COLLISON_AXIS_IDX{ 6 };
			static int NUM_AXIS = 3; //x,y,z
			int penetrationAxisStart = _minPenetrationAxisIdx - EDGES_COLLISON_AXIS_IDX;
			int testAxis1 = penetrationAxisStart / NUM_AXIS;
			int testAxis2 = penetrationAxisStart % NUM_AXIS;

			//orientation of the colliding edges
			Vector3 edge1, edge2;

			std::array<float, 3> vertexOneArr{ vertexOne.x, vertexOne.y, vertexOne.z };
			std::array<float, 3> vertexTwoArr{ vertexTwo.x, vertexTwo.y, vertexTwo.z };

			edge1 = (vertexOneArr[testAxis1] < 0) ? GetAxis(_t1.orientation, testAxis1) : GetAxis(_t1.orientation, testAxis1) * -1.f;
			edge2 = (vertexTwoArr[testAxis2] < 0) ? GetAxis(_t2.orientation, testAxis2) : GetAxis(_t2.orientation, testAxis2) * -1.f;

			//local -> world (vert1)
			DirectX::SimpleMath::Vector4 temp = DirectX::SimpleMath::Vector4::Transform(
				DirectX::SimpleMath::Vector4{ vertexOne.x,vertexOne.y,vertexOne.z, 1.f },
				_t1.matrix
			);
			vertexOne = { temp.x,temp.y,temp.z };

			//local -> world (vert2)
			temp = DirectX::SimpleMath::Vector4::Transform(
				DirectX::SimpleMath::Vector4{ vertexTwo.x,vertexTwo.y,vertexTwo.z, 1.f },
				_t2.matrix
			);
			vertexTwo = { temp.x,temp.y,temp.z };

			//1. calculate the dot product between edge1 and edge2:
			float k = edge1.Dot(edge2);//cosine

			//2.  point on the edge of box1 closest to the initial contact point on box2
			//    The calculation involves projecting the vector from contactPoint1 to contactPoint2 onto the _direction of edge1 - edge2 * k.
			Vector3 closestPointOne = { vertexOne + edge1 * ((vertexTwo - vertexOne).Dot(edge1 - edge2 * k) / (1 - k * k)) };

			//3. point on the edge of box2 closest to 
			//projecting the vector from closestPointOne to vertexTwo onto direction2.
			Vector3 closestPointTwo{ vertexTwo + edge2 * ((closestPointOne - vertexTwo).Dot(edge2)) };

			_newContact.contactPoints = { closestPointOne, closestPointTwo};
		}
	}
	
	/**
	 * Determines the closest point on a collider's surface to the collision point.
	 * @param _collisionNormal Normal vector at the point of collision.
	 * @param _t Transform of the collider.
	 * @param _cmp Comparison function to determine the contact point.
	 * @return The closest point on the collider's surface.
	 */
	Vector3 PhysicsSys::GetLocalContactVertex(Vector3 _collisionNormal, const Transform& _t, std::function<bool(const float&, const float&)> const _cmp) 
	{
		Vector3 contactPoint{ _t.scale * 0.5f };

		if (_cmp(GetAxis(_t.orientation, 0).Dot(_collisionNormal), 0)) {
			contactPoint.x = -contactPoint.x;
		}
		if (_cmp(GetAxis(_t.orientation, 1).Dot(_collisionNormal), 0)) {
			contactPoint.y = -contactPoint.y;
		}
		if (_cmp(GetAxis(_t.orientation, 2).Dot(_collisionNormal), 0)) {
			contactPoint.z = -contactPoint.z;
		}
		return contactPoint;
	}

	/**
	 * Performs collision detection for box colliders.
	 * Checks for collisions with other boxes and planes, generating collision data.
	 * @param _it Iterator for the ECS system.
	 * @param _transforms Array of Transform components.
	 * @param _boxes Array of BoxCollider components.
	 */
	void PhysicsSys::TestBoxCollision(flecs::iter& _it, Transform* _transforms, BoxCollider* _boxes) 
	{
		// Box vs. Box
		auto boxEntities = _it.world().filter<Transform, BoxCollider>();
		boxEntities.each([&](flecs::entity _e1, Transform& _t1, BoxCollider& _b1) {
			Vector3 boxColliderScale1 = std::get<Vector3>(_b1.GetScale());
			Vector3 boxColliderOffset1 = _b1.GetOffset();
			Transform worldTransform1 = TransformSys::GetWorldTransform(_e1,boxColliderOffset1);

			boxEntities.each([&](flecs::entity _e2, Transform& _t2, BoxCollider& _b2) {
				if (_e1 == _e2) return; // skip self-collision check
				Vector3 boxColliderScale2 = std::get<Vector3>(_b2.GetScale());
				Vector3 boxColliderOffset2 = _b2.GetOffset();
				Transform worldTransform2 = TransformSys::GetWorldTransform(_e2, boxColliderOffset2);

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

					float penetration = CalcPenetration(worldTransform1, worldTransform2, boxColliderScale1, boxColliderScale2, boxColliderOffset1, boxColliderOffset2, axes[k]);
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
					return;
				}

				HandleCollisionStart(_e1.raw_id(), _e2.raw_id());

				if (_b1.m_isTrigger || _b2.m_isTrigger)
				{
					flecs::id_t triggerEntityId = _b1.m_isTrigger ? _e1.raw_id() : _e2.raw_id();
					flecs::id_t nonTriggerEntityId = _b1.m_isTrigger ? _e2.raw_id() : _e1.raw_id();
					return;
				}

				CollisionData newContact;
				newContact.entity1 = _e1;
				newContact.entity2 = _e2;
				newContact.penetrationDepth = minPenetration;
				newContact.restitution = std::fmin(_b1.m_restitution, _b2.m_restitution);
				newContact.friction = 0.5f * (_b1.m_friction + _b2.m_friction);

				//vector pointing from the center of box2 to the center of box1
				Vector3 box2ToBox1 = worldTransform1.position - worldTransform2.position;

				//ensures the _collisionNormal to always point from box2 towards box1.
				newContact.collisionNormal = (axes[minAxisIdx].Dot(box2ToBox1) < 0) ? -axes[minAxisIdx] : axes[minAxisIdx];

				CalcOBBsContactPoints(worldTransform1, worldTransform2, newContact, minAxisIdx);
				AddCollisionData(newContact);
				});

			});

		// Box vs. PlaneCollider
		auto constraints = _it.world().filter<PlaneCollider>();
		if (!constraints.count()) {
			return;
		}

		constraints.each([&](flecs::entity _e, PlaneCollider& _planeCollider) {
			const Transform* constraintTransform = _e.get<Transform>();
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

						//assuming no trigger events between plane & triggers
						if (_boxes[k].m_isTrigger)
						{
							return;
						}


						CollisionData collisionData;
						collisionData.entity1 = _it.entity(k);
						collisionData.entity2 = _e;
						collisionData.collisionNormal = constraintNormal;
						collisionData.contactPoints = {
						std::make_pair<Vector3,Vector3>(
							Vector3(vertices[i]),
							Vector3{}
						) };
						collisionData.penetrationDepth = PLANE_OFFSET - distance;
						collisionData.restitution = std::fmin(_boxes[k].m_restitution, _planeCollider.m_restitution);//0.2
						collisionData.friction = 0.5f * (_boxes[k].m_friction +  _planeCollider.m_friction);//0.6
						collisionData.accumulatedNormalImpulse = 0.f;
						AddCollisionData(collisionData);
					}
				}
			}
			});
	}
	
	/**
	 * Performs collision detection for sphere colliders.
	 * Similar to `TestBoxCollision` but for spheres, checking collisions with other spheres, boxes, and planes.
	 * @param _it Iterator for the ECS system.
	 * @param _transforms Array of Transform components.
	 * @param _spheres Array of SphereCollider components.
	 */
	void PhysicsSys::TestSphereCollision(flecs::iter& _it, Transform* _transforms, SphereCollider* _spheres)
	{
		// fetch all entities with a Transform and SphereCollider ~
		auto sphereEntities = _it.world().filter<Transform, SphereCollider>();
		// iterate over each entity in the boxEntities filter ~
		sphereEntities.each([&](flecs::entity _e1, Transform& _t, SphereCollider& _s) {
			if (_e1.has<Rigidbody>() && _e1.get<Rigidbody>()->m_isStatic) return; //forcing non-collidables to the second entity

			float colliderScale1 = std::get<float>(_s.GetScale());
			Vector3 sphereColliderOffset1 = _s.GetOffset();
			Transform sphereWorldTransform1 = TransformSys::GetWorldTransform(_e1,sphereColliderOffset1);

			// Sphere vs. Sphere
			sphereEntities.each([&](flecs::entity _e2, Transform& _t2, SphereCollider& _s2) {
				if (_e1 == _e2) return; // Skip self-collision check.

				float colliderScale2 = std::get<float>(_s2.GetScale());
				Vector3 sphereColliderOffset2 = _s2.GetOffset();
				Transform sphereWorldTransform2 = TransformSys::GetWorldTransform(_e2, sphereColliderOffset2);

				float radSum = sphereWorldTransform1.scale.x * colliderScale1 + sphereWorldTransform2.scale.x * colliderScale2;

				radSum *= 0.5f;
				Vector3 distVector = sphereWorldTransform1.position - sphereWorldTransform2.position;
				float distSqrd = distVector.LengthSquared();

				if (distSqrd <= (radSum * radSum))
				{
					HandleCollisionStart(_e1.raw_id(),_e2.raw_id());
					if (_s.m_isTrigger || _s2.m_isTrigger) //assuming non trigger vs trigger collision
					{
						flecs::id_t triggerEntityId = _s.m_isTrigger ? _e1.raw_id(): _e2.raw_id();
						flecs::id_t nonTriggerEntityId = _s.m_isTrigger ? _e2.raw_id() : _e1.raw_id();
						return;
					}
					distVector.Normalize();

					CollisionData collisionData;
					collisionData.entity1 = _e1;
					collisionData.entity2 = _e2;
					collisionData.collisionNormal = distVector;
					collisionData.contactPoints = {
						std::make_pair<Vector3, Vector3>(
							sphereWorldTransform1.position - distVector * sphereWorldTransform1.scale.x* colliderScale1 * 0.5f,
							sphereWorldTransform2.position + distVector * sphereWorldTransform2.scale.x* colliderScale2 * 0.5f)
					};
					collisionData.penetrationDepth = radSum - sqrtf(distSqrd);
					collisionData.restitution = std::fmin(_s.m_restitution, _s2.m_restitution);//0.5
					collisionData.friction = 0.5f * (_s.m_friction + _s2.m_friction);//0.65
					collisionData.accumulatedNormalImpulse = 0.f;
					AddCollisionData(collisionData);
				}
				});
			});

		// Sphere vs. Box
		//not used the typical AABB method, which involves translating the sphere center to the _box'_s local coords 
		//and clamping to find the nearest point to avoid calculating the inverse matrix every tick
		static constexpr int NUM_AXES = 3;
		_it.world().each<BoxCollider>([&_it, &_transforms, &_spheres](flecs::entity _e, BoxCollider& _box)
			{
				Vector3 boxColliderOffset = _box.GetOffset();
				Transform boxTransform = TransformSys::GetWorldTransform(_e, boxColliderOffset);
				Vector3 boxColliderScale = std::get<Vector3>(_box.GetScale());

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

					//for the X,Y,Z _axis

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
						// clamp the projection along the current _axis
						projectionLength = std::clamp(projectionLength, -extent, extent);

						//accumulate for the closest point
						closestPoint += axis * projectionLength;
					}

					float distanceSquared = (closestPoint - sphereCenter).LengthSquared();

					if (distanceSquared > sphereRad * sphereRad) {
						continue; // no collision
					}
					HandleCollisionStart(_it.entity(k).raw_id(), _e.raw_id());
					if (_spheres[k].m_isTrigger || _box.m_isTrigger)
					{
						//Log::Info("sphere-_box trigger collision");
						flecs::id_t triggerEntityId = _spheres[k].m_isTrigger ? _it.entity(k).raw_id(): _e.raw_id();
						flecs::id_t nonTriggerEntityId = _spheres[k].m_isTrigger ? _e.raw_id() : _it.entity(k).raw_id();
						return;
					}

					//deal with collision
					CollisionData collisionData;
					collisionData.entity1 = _it.entity(k);
					collisionData.entity2 = _e;
					collisionData.collisionNormal = sphereCenter - closestPoint;
					collisionData.collisionNormal.Normalize();
					collisionData.contactPoints = {
						std::make_pair<Vector3, Vector3>(
						Vector3(sphereCenter - collisionData.collisionNormal * sphereRad)
							, Vector3(closestPoint))
					};
					collisionData.penetrationDepth = sphereRad - sqrtf(distanceSquared);
					collisionData.restitution = std::fmin(_box.m_restitution, _spheres[k].m_restitution);//0.5
					collisionData.friction = 0.5f * (_box.m_friction + _spheres[k].m_friction);//0.6
					collisionData.accumulatedNormalImpulse = 0.f;
					AddCollisionData(collisionData);
				}
			});


		// Sphere vs. PlaneCollider
		auto constraints = _it.world().filter<PlaneCollider>();
		if (!constraints.count()) {
			return;
		}

		constraints.each([&](flecs::entity _e, PlaneCollider& _planeCollider) {
			const Transform* constraintTransform = _e.get<Transform>();
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

					// Transform the collision point to the plane'_s local space
					Matrix inverseTransform = constraintTransform->matrix.Invert();
					Vector3 localCollisionPoint = Vector3::Transform(collisionPoint, inverseTransform);

					//check boundaries
					if ((localCollisionPoint.x < -0.5f || localCollisionPoint.x > 0.5f) ||
						(localCollisionPoint.z > 0.5f || localCollisionPoint.z < -0.5f))
					{
						continue;
					}

					//assuminig no collision between trigger & plane
					if (_spheres[k].m_isTrigger)
					{
						return;
					}

					CollisionData collisionData;
					collisionData.entity1 = _it.entity(k);
					collisionData.entity2 = _e;
					collisionData.collisionNormal = constraintNormal;
					collisionData.contactPoints = {
						std::make_pair<Vector3,Vector3>(Vector3(sphereTransform.position - constraintNormal * distance),Vector3{})
					};
					collisionData.penetrationDepth = sphereTransform.scale.x* sphereColliderScale * 0.5f - distance;
					collisionData.restitution = std::fmin(_spheres[k].m_restitution, _planeCollider.m_restitution);//0.18
					collisionData.friction = 0.5f * (_spheres[k].m_friction + _planeCollider.m_friction);//0.65
					collisionData.accumulatedNormalImpulse = 0.f;
					AddCollisionData(collisionData);
				}
			}
			});
	}

	/**
	 * Updates the physics state of entities based on their current velocities and accelerations.
	 * @param _it Iterator for the ECS system.
	 * @param _transform Array of Transform components.
	 * @param _rigidbody Array of Rigidbody components.
	 */
	void PhysicsSys::Integrate(flecs::iter& _it, Transform* _transform, Rigidbody* _rigidbody)
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
			_rigidbody[i].m_angularVelocity *= powf(0.01f, dt);

			// apply _axis locking
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
				// If there'_s no parent, the entity'_s transform is the world transform
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

	/**
	 * Stores collision data from a detected collision for later processing.
	 * @param _data The collision data to be added.
	 */
	void PhysicsSys::AddCollisionData(const CollisionData& _data)
	{
		//std::lock_guard<std::mutex> lock(collisionDataMutex);
		m_collisionData.push_back(_data);
	}

	/**
	 * Clears the current frame's collision data in preparation for the next frame's detection phase.
	 */
	void PhysicsSys::ClearCollisionData() 
	{
		m_collisionData.clear();
	}

	/**
	 * Processes all detected collisions, computing and applying appropriate impulses.
	 */
	void PhysicsSys::ResolveCollisions()
	{
		if (m_collisionData.empty() == true) 
		{
			return;
		}

		for (int iter{}; iter < SOLVER_ITERS; ++iter)
		{
			for (auto& collision : m_collisionData) 
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

				// Contact point relative to the body'_s position
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
					relativeVel -= (rb2->m_linearVelocity + (ExtractRotationMatrix(t2->matrix) * rb2->m_angularVelocity).Cross(r2));
				}

				float relativeSpeed = relativeVel.Dot(collision.collisionNormal);

				static constexpr float PENETRATION_TOLERANCE = 0.0005f;

				static constexpr float CORRECTION_RATIO = 0.1f;
				// Baumgarte Stabilization (for penetration resolution)
				float baumgarte = 0.0f;
				if (collision.penetrationDepth > PENETRATION_TOLERANCE) {
					baumgarte = static_cast<float>(
						(collision.penetrationDepth - PENETRATION_TOLERANCE) * (CORRECTION_RATIO / PHYSICS_UPDATE_TARGET_FPS_INV)
						);
				}

				static constexpr float RELATIVE_SPEED_SENSITIVITY = 2.5f;
				relativeSpeed *= RELATIVE_SPEED_SENSITIVITY*collision.restitution;

				static constexpr float CLOSING_SPEED_TOLERANCE = 0.0005f; 
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
				if (collision.accumulatedNormalImpulse < 0.0f) {
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

	/**
	 * Applies computed impulses to rigidbodies involved in a collision.
	 * @param _e1 First entity involved in the collision.
	 * @param _e2 Second entity involved in the collision.
	 * @param _jacobianImpulse Computed impulse magnitude.
	 * @param _r1 Relative position vector from the first entity's center to the contact point.
	 * @param _r2 Relative position vector from the second entity's center to the contact point.
	 * @param _direction Direction vector of the applied impulse.
	 * @param _rb1 Pointer to the first entity's Rigidbody component.
	 * @param _rb2 Pointer to the second entity's Rigidbody component.
	 * @param _t1 Optional transform of the first entity.
	 * @param _t2 Optional transform of the second entity.
	 */
	void PhysicsSys::ApplyImpulses(flecs::entity _e1, flecs::entity _e2, float _jacobianImpulse, const Vector3& _r1, const Vector3& _r2, const Vector3& _direction, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2) 
	{
		Vector3 linearImpulse = _direction * _jacobianImpulse;
		if (_t1.has_value() && _rb1->m_isStatic==false)
		{
			Vector3 dir1 = _r1.Cross(_direction);
			Vector3 angularImpulse1 = dir1 * _jacobianImpulse;
 			Vector3 linearVelDelta = linearImpulse * _rb1->m_inverseMass;
			_rb1->m_linearVelocity += linearVelDelta;

			Vector3 angVelDelta = _rb1->m_inverseInertiaTensorWorld * angularImpulse1;
			Vector3 localAngularVel = (ExtractRotationMatrix(_t1->matrix) * _rb1->m_angularVelocity) + angVelDelta;
			_rb1->m_angularVelocity = ExtractRotationMatrix(_t1->matrix).Transpose() * localAngularVel;
		}

		if (_t2.has_value() && _rb2->m_isStatic==false) {
			Vector3 angularImpulse2 = _r2.Cross(_direction) * _jacobianImpulse;
			Vector3 linearVelDelta = linearImpulse * _rb2->m_inverseMass;
			_rb2->m_linearVelocity -= linearVelDelta;

			Vector3 angVelDelta = _rb2->m_inverseInertiaTensorWorld * angularImpulse2;
			Vector3 localAngularVel = (ExtractRotationMatrix(_t2->matrix) * _rb2->m_angularVelocity) - angVelDelta;
			_rb2->m_angularVelocity = ExtractRotationMatrix(_t2->matrix).Transpose() * localAngularVel;
		}
	}

	/**
	 * Calculates frictional impulses based on tangential forces at the point of collision.
	 * @param _e1 First entity involved in the collision.
	 * @param _e2 Second entity involved in the collision.
	 * @param _r1 Relative position vector from the first entity's center to the contact point.
	 * @param _r2 Relative position vector from the second entity's center to the contact point.
	 * @param _tangent Tangent vector at the point of collision.
	 * @param _rb1 Pointer to the first entity's Rigidbody component.
	 * @param _rb2 Pointer to the second entity's Rigidbody component.
	 * @param _t1 Optional transform of the first entity.
	 * @param _t2 Optional transform of the second entity.
	 * @param _collision Reference to the collision data.
	 * @return Computed tangential impulse.
	 */
	float PhysicsSys::ComputeTangentialImpulses(const flecs::entity& _e1, const flecs::entity& _e2, const Vector3& _r1, const Vector3& _r2, const Vector3& _tangent, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2, const CollisionData& _collision) 
	{
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

		// Compute the effective mass for the friction/tangential _direction
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

		// Clamp based on Coulomb'_s law
		float maxFriction = _collision.friction * _collision.accumulatedNormalImpulse;		

		return frictionImpulse;
	}

	/**
	 * Applies computed frictional impulses to rigidbodies, affecting their motion due to surface friction.
	 * @param _e1 First entity involved in the collision.
	 * @param _e2 Second entity involved in the collision.
	 * @param _r1 Relative position vector from the first entity's center to the contact point.
	 * @param _r2 Relative position vector from the second entity's center to the contact point.
	 * @param _collision Reference to the collision data.
	 * @param _rb1 Pointer to the first entity's Rigidbody component.
	 * @param _rb2 Pointer to the second entity's Rigidbody component.
	 * @param _t1 Optional transform of the first entity.
	 * @param _t2 Optional transform of the second entity.
	 */
	void PhysicsSys::ApplyFrictionImpulses(flecs::entity _e1, flecs::entity _e2, const Vector3& _r1, const Vector3& _r2, const CollisionData& _collision, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2)
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

		// compute the impulses in each tangential _direction
		float jacobianImpulseT1 = ComputeTangentialImpulses(_e1, _e2, _r1, _r2, tangent1, _rb1, _rb2, _t1, _t2, _collision);
		ApplyImpulses(_e1, _e2, jacobianImpulseT1, _r1, _r2, tangent1, _rb1, _rb2, _t1, _t2);

		float jacobianImpulseT2 = ComputeTangentialImpulses(_e1, _e2, _r1, _r2, tangent2, _rb1, _rb2, _t1, _t2, _collision);
		ApplyImpulses(_e1, _e2, jacobianImpulseT2, _r1, _r2, tangent2, _rb1, _rb2, _t1, _t2);
	}

	/**
	 * Serializes the state of physics components into JSON format.
	 * @param _entity The entity whose components are being serialized.
	 * @param _components Reference to a vector of JSON objects where the serialized data will be stored.
	 * @param _type String indicating the type of component to serialize.
	 */
	void PhysicsSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& _type)
	{
		using namespace nlohmann;
		using namespace Hostile;

		if (_type == "BoxCollider")
		{
			if (auto* collider = _entity.get<BoxCollider>(); collider)
			{
				json obj = json::object();
				obj["Type"] = "BoxCollider";
				obj["IsTrigger"] = collider->m_isTrigger;
				_components.push_back(obj);
			}
		}
		else if (_type == "SphereCollider")
		{
			if (auto* collider = _entity.get<SphereCollider>(); collider)
			{
				json obj = json::object();
				obj["Type"] = "SphereCollider";
				obj["IsTrigger"] = collider->m_isTrigger;
				_components.push_back(obj);
			}
		}
		else if (_type == "PlaneCollider")
		{
			if (auto* collider = _entity.get<PlaneCollider>(); collider)
			{
				json obj = json::object();
				obj["Type"] = "PlaneCollider";
				obj["IsTrigger"] = collider->m_isTrigger;
				_components.push_back(obj);
			}
		}
		else if (_type == "Rigidbody")
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

	/**
	 * Deserializes physics components from JSON, restoring their state in the physics system.
	 * @param _entity The entity to which the deserialized components will be added.
	 * @param _data JSON object containing the serialized component data.
	 * @param _type String indicating the type of component to deserialize.
	 */
	void PhysicsSys::Read(flecs::entity& _entity, nlohmann::json& _data, const std::string& _type)
	{
		using namespace nlohmann;
		using namespace Hostile;

		if (_type == "BoxCollider")
		{
			_entity.add<BoxCollider>();
			BoxCollider* collider = _entity.get_mut<BoxCollider>();
			if (collider) 
			{
				collider->m_isTrigger = _data.value("IsTrigger", false);
			}
		}
		else if (_type == "SphereCollider")
		{
			_entity.add<SphereCollider>();
			SphereCollider* collider = _entity.get_mut<SphereCollider>();
			if (collider) 
			{
				collider->m_isTrigger = _data.value("IsTrigger", false);
			}
		}
		else if (_type == "PlaneCollider")
		{
			_entity.add<PlaneCollider>();
			PlaneCollider* collider = _entity.get_mut<PlaneCollider>();
			if (collider) 
			{
				collider->m_isTrigger = _data.value("IsTrigger", false);
			}
		}
		else if (_type == "Rigidbody")
		{
			_entity.add<Rigidbody>();
			Rigidbody* body = _entity.get_mut<Rigidbody>();
			if (body) 
			{
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
	
	/**
	 * Provides a graphical user interface for viewing and editing physics components' properties.
	 * Useful for debugging and game development.
	 * @param _entity The entity whose components are being displayed or edited.
	 * @param _type String indicating the type of component to display.
	 */
	void PhysicsSys::GuiDisplay(flecs::entity& _entity, const std::string& _type)
	{
		if (_type == "Rigidbody")
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
		else if (_type == "BoxCollider" || _type == "SphereCollider" || _type == "PlaneCollider")
		{
			// General code for all colliders
			bool hasCollider = (_type == "BoxCollider" && _entity.has<BoxCollider>()) ||
				(_type == "SphereCollider" && _entity.has<SphereCollider>()) ||
				(_type == "PlaneCollider" && _entity.has<PlaneCollider>());

			bool isOpen = ImGui::CollapsingHeader(_type.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				ImGui::OpenPopup("Collider Popup");
			}
			if (ImGui::BeginPopup("Collider Popup"))
			{
				if (ImGui::Button("Remove Component"))
				{
					if (_type == "BoxCollider") _entity.remove<BoxCollider>();
					else if (_type == "SphereCollider") _entity.remove<SphereCollider>();
					else if (_type == "PlaneCollider") _entity.remove<PlaneCollider>();
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
					if (_type == "BoxCollider") 
					{
						collider = _entity.get_mut<BoxCollider>();
						ImGui::DragFloat3("scale", & dynamic_cast<BoxCollider*>(collider)->m_scale.x, 0.1f);
					}
					else if (_type == "SphereCollider") 
					{
						collider = _entity.get_mut<SphereCollider>();
						ImGui::DragFloat("scale", &dynamic_cast<SphereCollider*>(collider)->m_radius, 0.1f);
					}
					else if (_type == "PlaneCollider") 
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


	/**
	 * Marks the beginning of a collision event between two entities.
	 * @param _entity1 ID of the first entity involved in the collision.
	 * @param _entity2 ID of the second entity involved in the collision.
	 */
	void PhysicsSys::HandleCollisionStart(flecs::id_t _entity1, flecs::id_t _entity2)
	{
		m_currentFrameCollisions[_entity1].insert(_entity2);

		auto& world = IEngine::Get().GetWorld();
		CollisionEventData* eData = world.entity(_entity1).get_mut<CollisionEventData>();
		eData->m_collidingEntities.insert(_entity2);

		m_currentFrameCollisions[_entity2].insert(_entity1);

		eData = world.entity(_entity2).get_mut<CollisionEventData>();
		eData->m_collidingEntities.insert(_entity1);
	}

	/**
	 * Handles the end of a collision event, updating data structures to reflect that the entities are no longer colliding.
	 * @param _entity1 ID of the first entity involved in the collision.
	 * @param _entity2 ID of the second entity involved in the collision.
	 */
	void PhysicsSys::HandleCollisionEnd(flecs::id_t _entity1, flecs::id_t _entity2)
	{
		auto& world = IEngine::Get().GetWorld();
		CollisionEventData* eData=world.entity(_entity1).get_mut<CollisionEventData>();
		eData->m_collidingEntities.erase(_entity2);	

		eData = world.entity(_entity2).get_mut<CollisionEventData>();
		eData->m_collidingEntities.erase(_entity1);
	}
}