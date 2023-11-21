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
#include "PhysicsProperties.h"

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

		REGISTER_TO_SERIALIZER(Rigidbody, this);
		REGISTER_TO_DESERIALIZER(Rigidbody, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"Rigidbody",
			std::bind(&DetectCollisionSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { 
				_entity.add<Rigidbody>(); 
			});

		REGISTER_TO_SERIALIZER(PlaneCollider, this);
		REGISTER_TO_DESERIALIZER(PlaneCollider, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"PlaneCollider",
			std::bind(&DetectCollisionSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<PlaneCollider>(); });

		REGISTER_TO_SERIALIZER(SphereCollider, this);
		REGISTER_TO_DESERIALIZER(SphereCollider, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"SphereCollider",
			std::bind(&DetectCollisionSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<SphereCollider>(); });

		REGISTER_TO_SERIALIZER(BoxCollider, this);
		REGISTER_TO_DESERIALIZER(BoxCollider, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"BoxCollider",
			std::bind(&DetectCollisionSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<BoxCollider>(); });
	}

	Vector3 DetectCollisionSys::GetAxis(const Quaternion& orientation, int index) {
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

	bool DetectCollisionSys::IsColliding(const Transform& _t1, const Transform& _t2, const Vector3& distVector, const float& radSum, float& distSqrd)
	{
		distSqrd = distVector.LengthSquared();
		return distSqrd <= (radSum * radSum);
	}
	bool DetectCollisionSys::IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Transform& _tBox, const BoxCollider& _b)
	{
		return true;

	}
	bool DetectCollisionSys::IsColliding(const Transform& _tSphere, const Vector3& _constraintNormal, float _offsetFromOrigin, float& _distance)
	{
		_distance = std::abs(_constraintNormal.Dot(_tSphere.position) + _offsetFromOrigin - PLANE_OFFSET);
		return _tSphere.scale.x * 0.5f > _distance;//assuming uniform x,y,and z
	}

	bool DetectCollisionSys::IsColliding(const Transform& _t1, const BoxCollider& _b1, const Transform& _t2, const BoxCollider& _b2)
	{
		return true;
	}
	bool DetectCollisionSys::IsColliding(const Transform& _tBox, const BoxCollider& _b, const PlaneCollider& _c)
	{
		return true;
	}

	float DetectCollisionSys::CalcPenetration(const Transform& t1, const Transform& t2, const Vector3& axis) {
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
	void DetectCollisionSys::TestSphereCollision(flecs::iter& _it, Transform* _transforms, SphereCollider* _spheres)
	{
		// fetch all entities with a Transform and SphereCollider ~
		auto sphereEntities = _it.world().filter<Transform, SphereCollider>();
		// iterate over each entity in the boxEntities filter ~
		sphereEntities.each([&](flecs::entity e1, Transform& t, SphereCollider& s) {
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
			if (s.m_isTrigger || s2.m_isTrigger) 
			{
				//Log::Info("sphere-sphere trigger collision");
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
			IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
		}
			});
			});

		// Sphere vs. Box
		//not used the typical AABB method, which involves translating the sphere center to the box's local coords 
		//and clamping to find the nearest point to avoid calculating the inverse matrix every tick
		static constexpr int NUM_AXES = 3;
		_it.world().each<BoxCollider>([&_it, &_transforms,&_spheres](flecs::entity e, BoxCollider& box)
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
			IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
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

				IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
			}
		}
			});
	}
	Vector3 DetectCollisionSys::GetLocalContactVertex(Vector3 collisionNormal, const Transform& t, std::function<bool(const float&, const float&)> const cmp) {
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

	void DetectCollisionSys::TestBoxCollision(flecs::iter& _it, Transform* _transforms, BoxCollider* _boxes) {

		// Box vs. Box
		auto boxEntities = _it.world().filter<Transform, BoxCollider>();

		boxEntities.each([&](flecs::entity e1, Transform& t1, BoxCollider& b1) {
			Transform worldTransform1 = TransformSys::GetWorldTransform(e1);

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
				crossProduct.Normalize();
				axes.push_back(crossProduct);
			}
		}

		float minPenetration = FLT_MAX;
		int minAxisIdx = 0;

		for (int k{}; k < axes.size(); ++k) {
			float penetration = CalcPenetration(worldTransform1, worldTransform2, axes[k]);

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

		if (b1.m_isTrigger || b2.m_isTrigger)
		{
			//Log::Info("box-box trigger collision");
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

		CalcOBBsContactPoints(worldTransform1, worldTransform2, newContact, minAxisIdx);
		IEngine::Get().GetWorld().entity().set<CollisionData>(newContact);
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
					IEngine::Get().GetWorld().entity().set<CollisionData>(collisionData);
				}
			}
		}
			});
	}

	void DetectCollisionSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
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
					{"InverseInertiaTensor", WriteMat3(body->m_inverseInertiaTensor)},
					{"InverseInertiaTensorWorld", WriteMat3(body->m_inverseInertiaTensorWorld)}
				};
				_components.push_back(obj);
			}
		}
	}

	void DetectCollisionSys::Read(flecs::entity& _entity, nlohmann::json& _data, const std::string& type)
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
				body->m_inverseInertiaTensor = ReadMat3(_data["InverseInertiaTensor"]);
				body->m_inverseInertiaTensorWorld = ReadMat3(_data["InverseInertiaTensorWorld"]);
			}
		}
	}

	void DetectCollisionSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
	{
		if (type == "Rigidbody") 
		{
			bool hasRigidbody = _entity.has<Rigidbody>();

			if (ImGui::TreeNode("Rigidbody")) 
			{
				if (ImGui::Checkbox("Has Rigidbody", &hasRigidbody)) 
				{
					if (hasRigidbody) 
					{
						_entity.add<Rigidbody>();
					}
					else
					{
						_entity.remove<Rigidbody>();
					}
				}

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
						ImGui::DragFloat3("Linear Velocity", &rb->m_linearVelocity.x, 0.1f);
						ImGui::DragFloat3("Linear Acceleration", &rb->m_linearAcceleration.x, 0.1f);
						ImGui::DragFloat3("Force", &rb->m_force.x, 0.1f);
						ImGui::DragFloat3("Torque", &rb->m_torque.x, 0.1f);
						ImGui::DragFloat("Linear Damping", &rb->m_linearDamping, 0.01f, 0.0f, 1.f, "%.3f");
						ImGui::DragFloat("Angular Damping", &rb->m_angularDamping, 0.01f, 0.0f, 1.f, "%.3f");
						ImGui::Checkbox("Use Gravity", &rb->m_useGravity);
					}
				}
				ImGui::TreePop(); // End of Rigidbody section
			}
		}
		else if (type == "BoxCollider" || type == "SphereCollider" || type == "PlaneCollider") 
		{
			// General code for all colliders
			bool hasCollider = (type == "BoxCollider" && _entity.has<BoxCollider>()) ||
				(type == "SphereCollider" && _entity.has<SphereCollider>()) ||
				(type == "PlaneCollider" && _entity.has<PlaneCollider>());

			if (ImGui::TreeNode(type.c_str())) 
			{
				if (ImGui::Checkbox(("Has " + type).c_str(), &hasCollider)) 
				{
					if (hasCollider) 
					{
						if (type == "BoxCollider") _entity.add<BoxCollider>();
						else if (type == "SphereCollider") _entity.add<SphereCollider>();
						else if (type == "PlaneCollider") _entity.add<PlaneCollider>();
					}
					else 
					{
						if (type == "BoxCollider") _entity.remove<BoxCollider>();
						else if (type == "SphereCollider") _entity.remove<SphereCollider>();
						else if (type == "PlaneCollider") _entity.remove<PlaneCollider>();
					}
				}

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
				ImGui::TreePop(); // End of Collider section
			}
		}
	}
}