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
#include "CollisionSys.h"
#include "TransformSys.h"
#include "PhysicsProperties.h"

namespace Hostile {

	ADD_SYSTEM(CollisionSys);
	std::vector<CollisionData> CollisionSys::collisionEvents;
	std::vector<TriggerData> CollisionSys::triggerEvents;
	std::mutex CollisionSys::collisionDataMutex;

	void CollisionSys::OnCreate(flecs::world& _world)
	{
		_world.system("Collision PreUpdate")
			.kind(IEngine::Get().GetCollisionPhase())
			.iter([this](flecs::iter const& _info)
				{
					ClearCollisionData();
					triggerEvents.clear();
				});

		_world.system<Transform, SphereCollider>("TestSphereCollision")
			.kind(IEngine::Get().GetCollisionPhase())
			.iter(TestSphereCollision);

		_world.system<Transform, BoxCollider>("TestBoxCollision")
			.kind(IEngine::Get().GetCollisionPhase())
			.iter(TestBoxCollision);

		_world.system("ResolveCollisionSys")
			.kind(IEngine::Get().GetCollisionPhase())
			.iter([&](flecs::iter& it) 
				{
					float deltaTime = it.delta_time();
					ResolveCollisions(deltaTime);
				});
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

	void CollisionSys::AddTriggerData(flecs::id_t triggerId, flecs::id_t nonTriggerId)
	{
		triggerEvents.emplace_back(triggerId, nonTriggerId);
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

	float CollisionSys::CalcPenetration(const Transform& t1, const Transform& t2, const Vector3& axis) {
		Vector3 centerToCenter = t2.position - t1.position;
		Vector3 extents1 = t1.scale * 0.5;
		Vector3 extents2 = t2.scale * 0.5f;
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
	void CollisionSys::TestSphereCollision(flecs::iter& _it, Transform* _transforms, SphereCollider* _spheres)
	{
		// fetch all entities with a Transform and SphereCollider ~
		auto sphereEntities = _it.world().filter<Transform, SphereCollider>();
		// iterate over each entity in the boxEntities filter ~
		sphereEntities.each([&](flecs::entity e1, Transform& t, SphereCollider& s) {
			if (e1.has<Rigidbody>() && e1.get<Rigidbody>()->m_isStatic) return; //forcing non-collidables to the second entity
			
			Transform sphereWorldTransform1 = TransformSys::GetWorldTransform(e1);

			// Sphere vs. Sphere
			sphereEntities.each([&](flecs::entity e2, Transform& t2, SphereCollider& s2) {
				if (e1 == e2) return; // Skip self-collision check.
				Transform sphereWorldTransform2 = TransformSys::GetWorldTransform(e2);

				float radSum = sphereWorldTransform1.scale.x + sphereWorldTransform2.scale.x;
				radSum *= 0.5f;
				Vector3 distVector = sphereWorldTransform1.position - sphereWorldTransform2.position;
				float distSqrd = distVector.LengthSquared();

				if (distSqrd <= (radSum * radSum))
				{
					if (s.m_isTrigger || s2.m_isTrigger) //assuming non trigger vs trigger collision
					{
						flecs::id_t triggerEntityId = s.m_isTrigger ? e1.raw_id(): e2.raw_id();
						flecs::id_t nonTriggerEntityId = s.m_isTrigger ? e2.raw_id() : e1.raw_id();
						AddTriggerData(triggerEntityId, nonTriggerEntityId);
						return;
						//TODO::OnTriggerEnter,OnTriggerStay, OnTriggerExit
					}
					distVector.Normalize();

					CollisionData collisionData;
					collisionData.entity1 = e1;
					collisionData.entity2 = e2;
					collisionData.collisionNormal = distVector;
					collisionData.contactPoints = {
						std::make_pair<Vector3, Vector3>(
							sphereWorldTransform1.position - distVector * sphereWorldTransform1.scale.x * 0.5f,
							sphereWorldTransform2.position + distVector * sphereWorldTransform2.scale.x * 0.5f)
					};
					collisionData.penetrationDepth = radSum - sqrtf(distSqrd);
					collisionData.restitution = .5f; // temp
					collisionData.friction = .65f; // temp
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
				Transform boxTransform = TransformSys::GetWorldTransform(e);

				for (int k = 0; k < _it.count(); ++k)
				{
					Transform sphereTransform = TransformSys::GetWorldTransform(_it.entity(k));

					float sphereRad = sphereTransform.scale.x * 0.5f;
					Vector3 sphereCenter = sphereTransform.position;

					Vector3 centerToCenter = sphereCenter - boxTransform.position;
					Vector3 extents = boxTransform.scale * 0.5f;
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
						AddTriggerData(triggerEntityId, nonTriggerEntityId);
						return;
						//TODO::OnTriggerEnter,OnTriggerStay, OnTriggerExit
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
					AddCollisionData(collisionData);
				}
			});


		// Sphere vs. PlaneCollider
		auto constraints = _it.world().filter<PlaneCollider>();
		if (!constraints.count()) {
			return;
		}

		constraints.each([&](flecs::entity e, PlaneCollider& constraint) {
			const Transform* constraintTransform = e.get<Transform>();
			if (!constraintTransform) {
				return;
			}
			Vector3 constraintNormal = Vector3::Transform(UP_VECTOR, constraintTransform->orientation);
			constraintNormal.Normalize();
			float constraintOffsetFromOrigin = -constraintNormal.Dot(constraintTransform->position);

			for (int k = 0; k < _it.count(); ++k)
			{
				Transform sphereTransform = TransformSys::GetWorldTransform(_it.entity(k));

				float distance = std::abs(constraintNormal.Dot(sphereTransform.position) + constraintOffsetFromOrigin) - PLANE_OFFSET;// -constraintNormal.Dot(Vector3{ 0.f,PLANE_OFFSET,0.f });
				if (sphereTransform.scale.x * 0.5f > distance)//assuming uniform x,y,and z
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

					if (_spheres[k].m_isTrigger || constraint.m_isTrigger)
					{
						//Log::Info("sphere-plane trigger collision");
						AddTriggerData(e.raw_id(), _it.entity(k).raw_id());
						return;
						//TODO::OnTriggerEnter,OnTriggerStay, OnTriggerExit
					}

					CollisionData collisionData;
					collisionData.entity1 = _it.entity(k);
					collisionData.entity2 = e;
					collisionData.collisionNormal = constraintNormal;
					collisionData.contactPoints = {
						std::make_pair<Vector3,Vector3>(Vector3(sphereTransform.position - constraintNormal * distance),Vector3{})
					};
					collisionData.penetrationDepth = sphereTransform.scale.x * 0.5f - distance;
					collisionData.restitution = .18f; //   temp
					collisionData.friction = .65f;    //	"
					collisionData.accumulatedNormalImpulse = 0.f;
					AddCollisionData(collisionData);
				}
			}
			});
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
		auto boxEntities = _it.world().filter<Transform, BoxCollider>();

		boxEntities.each([&](flecs::entity e1, Transform& t1, BoxCollider& b1) {
			Transform worldTransform1 = TransformSys::GetWorldTransform(e1);
			if (e1.has<Rigidbody>() && e1.get<Rigidbody>()->m_isStatic) return; //forcing non-collidables to the second entity

			boxEntities.each([&](flecs::entity e2, Transform& t2, BoxCollider& b2) {
				if (e1 == e2) return; // Skip self-collision check

				Transform worldTransform2 = TransformSys::GetWorldTransform(e2);


				bool isColliding{ true };
				std::vector<Vector3> axes;

				//"face(box1)" <-> vertex(box2)
				for (int k{}; k < 3; ++k) {
					axes.push_back(GetAxis(worldTransform1.orientation, k));
				}

				//"face(box2)" <-> vertex(box1)
				for (int k{}; k < 3; ++k) {
					axes.push_back(GetAxis(worldTransform2.orientation, k));
				}

				//edge-edge
				for (int p{}; p < 3; ++p) {
					for (int q{}; q < 3; ++q) {
						Vector3 crossProduct = axes[p].Cross(axes[3 + q]);
						constexpr float EPSILON = 1e-5f;
						if (crossProduct.LengthSquared()>EPSILON*EPSILON) {  // Check if crossProduct is not a zero vector
							crossProduct.Normalize();
							axes.push_back(crossProduct);
						}
					}
				}

				float minPenetration = FLT_MAX;
				int minAxisIdx = 0;
				const int AxesSize = axes.size();
				for (int k{}; k < AxesSize; ++k) {
					float penetration = CalcPenetration(worldTransform1, worldTransform2, axes[k]);
					if (penetration <= 0.f) {
						isColliding = false;
						break;
					}

					if (penetration < minPenetration) {
						minPenetration = penetration;
						minAxisIdx = k;
					}
				}
				if (isColliding == false) {
					return;
				}

				if (b1.m_isTrigger || b2.m_isTrigger)
				{
					//Log::Info("box-box trigger collision");
					flecs::id_t triggerEntityId = b1.m_isTrigger ? e1.raw_id():e2.raw_id();
					flecs::id_t nonTriggerEntityId = b1.m_isTrigger ? e2.raw_id():e1.raw_id();
					AddTriggerData(triggerEntityId, nonTriggerEntityId);
					return;
					//TODO::OnTriggerEnter,OnTriggerStay, OnTriggerExit
				}

				CollisionData newContact;
				newContact.entity1 = e1;
				newContact.entity2 = e2;
				newContact.penetrationDepth = minPenetration;
				newContact.restitution = 0.1f;  //temp
				newContact.friction = 0.6f;		//temp

				//vector pointing from the center of box2 to the center of box1
				Vector3 box2ToBox1 = worldTransform1.position - worldTransform2.position;

				//ensures the collisionNormal to always point from box2 towards box1.
				newContact.collisionNormal = (axes[minAxisIdx].Dot(box2ToBox1) < 0) ? -axes[minAxisIdx] : axes[minAxisIdx];
				//Log::Trace(newContact.penetrationDepth);

				CalcOBBsContactPoints(worldTransform1, worldTransform2, newContact, minAxisIdx);
				AddCollisionData(newContact);
				});
			});

		// Box vs. PlaneCollider
		auto constraints = _it.world().filter<PlaneCollider>();
		if (!constraints.count()) {
			return;
		}

		constraints.each([&](flecs::entity e, PlaneCollider& constraint) {
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

						if (_boxes[k].m_isTrigger || constraint.m_isTrigger)
						{
							//Log::Info("box-plane trigger collision");
							AddTriggerData(e.raw_id(), _it.entity(k).raw_id());
							return;
							//TODO::OnTriggerEnter,OnTriggerStay, OnTriggerExit
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
						collisionData.restitution = .2f; //   temp
						collisionData.friction = .6f;    //	"
						collisionData.accumulatedNormalImpulse = 0.f;
						AddCollisionData(collisionData);
					}
				}
			}
			});
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
		constexpr int SOLVER_ITERS = 15;
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
					inverseMassSum += rb2->m_inverseMass;
				}
				if (fabs(inverseMassSum) < FLT_EPSILON) {
					continue;
				}

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
				Matrix3 i1;
				Matrix3 i2;
				if (t1.has_value())
				{
					i1 = rb1->m_inverseInertiaTensorWorld;
				}
				if (t2.has_value())
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
				Vector3 relativeVel;
				if (t1.has_value())
				{
					relativeVel = rb1->m_linearVelocity + (ExtractRotationMatrix(t1->matrix) * rb1->m_angularVelocity).Cross(r1);
				}
				if (t2.has_value())
				{
					relativeVel -= rb2->m_linearVelocity + (ExtractRotationMatrix(t2->matrix) * rb2->m_angularVelocity).Cross(r2);
				}

				float relativeSpeed = relativeVel.Dot(collision.collisionNormal);
				// Baumgarte Stabilization (for penetration resolution)
				static constexpr float PENETRATION_TOLERANCE = 0.000075f; //temp
				//fewer solver iteration, higher precision
				static constexpr float CORRECTION_RATIO = 0.25f;
				float baumgarte = 0.0f;
				if (collision.penetrationDepth > PENETRATION_TOLERANCE) {
					baumgarte = static_cast<float>(
						(collision.penetrationDepth - PENETRATION_TOLERANCE) * (CORRECTION_RATIO / dt)
						);
				}
				static constexpr float CLOSING_SPEED_TOLERANCE = 0.00005f; //temp
				float restitutionTerm = 0.0f;
				if (relativeSpeed > CLOSING_SPEED_TOLERANCE) {
					restitutionTerm = collision.restitution * (relativeSpeed - CLOSING_SPEED_TOLERANCE);
				}

				// Compute the impulse
				float jacobianImpulse = ((-(1 + restitutionTerm) * relativeSpeed) + baumgarte) / effectiveMass;

				if (isnan(jacobianImpulse)) {
					return;
				}

				// Compute the total impulse applied so far to maintain non-penetration
				float prevImpulseSum = collision.accumulatedNormalImpulse;
				collision.accumulatedNormalImpulse += jacobianImpulse;
				if (collision.accumulatedNormalImpulse < 0.0f) {//std::max
					collision.accumulatedNormalImpulse = 0.0f;
				}

				jacobianImpulse = collision.accumulatedNormalImpulse - prevImpulseSum;

				// Apply impulses to the bodies
				ApplyImpulses(e1, e2, jacobianImpulse, r1, r2, collision.collisionNormal, rb1, rb2, t1, t2);

				// Compute and apply frictional impulses using the two tangents
				ApplyFrictionImpulses(e1, e2, r1, r2, collision, rb1, rb2, t1, t2);
			}
		}
	}


	void CollisionSys::ApplyImpulses(flecs::entity e1, flecs::entity e2, float jacobianImpulse, const Vector3& r1, const Vector3& r2, const Vector3& direction, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2) {
		Vector3 linearImpulse = direction * jacobianImpulse;
		if (_t1.has_value())
		{
			Vector3 angularImpulse1 = r1.Cross(direction) * jacobianImpulse;
			_rb1->m_linearVelocity += linearImpulse * _rb1->m_inverseMass;
			Vector3 localAngularVel = (ExtractRotationMatrix(_t1->matrix) * _rb1->m_angularVelocity) + _rb1->m_inverseInertiaTensorWorld * angularImpulse1;
			_rb1->m_angularVelocity = ExtractRotationMatrix(_t1->matrix).Transpose() * localAngularVel;
		}

		if (_t2.has_value()) {
			Vector3 angularImpulse2 = r2.Cross(direction) * jacobianImpulse;
			_rb2->m_linearVelocity -= linearImpulse * _rb2->m_inverseMass;
			Vector3 localAngularVel = (ExtractRotationMatrix(_t2->matrix) * _rb2->m_angularVelocity) - _rb2->m_inverseInertiaTensorWorld * angularImpulse2;
			_rb2->m_angularVelocity = ExtractRotationMatrix(_t2->matrix).Transpose() * localAngularVel;
		}
	}

	float CollisionSys::ComputeTangentialImpulses(const flecs::entity& _e1, const flecs::entity& _e2, const Vector3& _r1, const Vector3& _r2, const Vector3& _tangent, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2, const CollisionData& _collision) {

		float inverseMassSum{};
		Vector3 termInDenominator1{};
		if (_rb1)
		{
			inverseMassSum = _rb1->m_inverseMass;
			termInDenominator1 = (_rb1->m_inverseInertiaTensorWorld * _r1.Cross(_tangent)).Cross(_r1);
		}

		Vector3 termInDenominator2;
		if (_rb2)
		{
			inverseMassSum += _rb2->m_inverseMass;
			termInDenominator2 = (_rb2->m_inverseInertiaTensorWorld * _r2.Cross(_tangent)).Cross(_r2);
		}

		// Compute the effective mass for the friction/tangential direction
		float effectiveMassTangential = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(_tangent);
		if (fabs(effectiveMassTangential) < FLT_EPSILON) {
			return 0.f;
		}

		// Calculate relative velocities along the tangent
		Vector3 relativeVel{};
		if (_t1.has_value())
		{
			relativeVel = _rb1->m_linearVelocity + (ExtractRotationMatrix(_t1->matrix) * _rb1->m_angularVelocity).Cross(_r1);
		}
		if (_t2.has_value())
		{
			relativeVel -= (_rb2->m_linearVelocity + (ExtractRotationMatrix(_t2->matrix) * _rb2->m_angularVelocity).Cross(_r2));
		}

		float relativeSpeedTangential = relativeVel.Dot(_tangent);

		// Compute the frictional impulse
		float frictionImpulseMagnitude = -relativeSpeedTangential / effectiveMassTangential;

		// Clamp based on Coulomb's law
		float maxFriction = _collision.friction * _collision.accumulatedNormalImpulse;
		frictionImpulseMagnitude = std::clamp(frictionImpulseMagnitude, -maxFriction, maxFriction);

		return frictionImpulseMagnitude;
	}

	void CollisionSys::ApplyFrictionImpulses(flecs::entity e1, flecs::entity e2, const Vector3& r1, const Vector3& r2, const CollisionData& _collision, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2)
	{
		Vector3 tangent1, tangent2;

		//erin catto - Box2D
		if (abs(_collision.collisionNormal.x) >= 0.57735f) {
			tangent1 = Vector3(_collision.collisionNormal.y, -_collision.collisionNormal.x, 0.0f);
		}
		else {
			tangent1 = Vector3(0.0f, _collision.collisionNormal.z, -_collision.collisionNormal.y);
		}
		tangent2 = _collision.collisionNormal.Cross(tangent1);

		// Compute the impulses in each direction and apply
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
			if (is_open)
			{

				if (hasRigidbody)
				{
					Rigidbody* rb = _entity.get_mut<Rigidbody>();
					if (rb)
					{
						float mass = rb->m_inverseMass != 0.0f ? 1.0f / rb->m_inverseMass : FLT_MAX;

						// Display and edit mass
						if (ImGui::DragFloat("Mass", &mass, 0.01f, 0.5f, 20.f, "%.3f"))
						{
							rb->m_inverseMass = mass != 0.0f ? 1.0f / mass : 0.0f; // Update inverse mass
						}

						// Add similar controls for other properties
						ImGui::DragFloat3("Vel", &rb->m_linearVelocity.x, 0.1f);
						ImGui::DragFloat3("Acc", &rb->m_linearAcceleration.x, 0.1f);
						ImGui::DragFloat3("Force", &rb->m_force.x, 0.1f);
						ImGui::DragFloat3("Torque", &rb->m_torque.x, 0.1f);
						ImGui::DragFloat("Glide", &rb->m_linearDamping, 0.01f, 0.0f, 1.f, "%.3f");
						ImGui::DragFloat("Spin", &rb->m_angularDamping, 0.01f, 0.0f, 1.f, "%.3f");
						ImGui::Checkbox("Use Gravity", &rb->m_useGravity);
						ImGui::Checkbox("Is Static", &rb->m_isStatic);
						ImGui::Text("");
					}
				}
			}
		}
		else if (type == "BoxCollider" || type == "SphereCollider" || type == "PlaneCollider")
		{
			// General code for all colliders
			bool hasCollider = (type == "BoxCollider" && _entity.has<BoxCollider>()) ||
				(type == "SphereCollider" && _entity.has<SphereCollider>()) ||
				(type == "PlaneCollider" && _entity.has<PlaneCollider>());

			bool is_open = ImGui::CollapsingHeader(type.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
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
			if (is_open)
			{
				if (hasCollider)
				{
					Collider* collider = nullptr;
					if (type == "BoxCollider") collider = _entity.get_mut<BoxCollider>();
					else if (type == "SphereCollider") collider = _entity.get_mut<SphereCollider>();
					else if (type == "PlaneCollider") collider = _entity.get_mut<PlaneCollider>();

					if (collider)
					{
						ImGui::Checkbox("Is Trigger", &collider->m_isTrigger);
					}
				}
				ImGui::Text("");
			}
		}
	}
}