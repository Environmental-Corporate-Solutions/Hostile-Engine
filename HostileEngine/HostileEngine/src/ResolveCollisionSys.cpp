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
#include "ResolveCollisionSys.h"
#include "DetectCollisionSys.h"//CollisionData
#include "GravitySys.h"//Velocity, Force, ModelMatrix
#include "TransformSys.h"//Trnasform
#include "Rigidbody.h"//Rigidbody

namespace { //anonymous ns, only in ResolveCollisionSys.cpp
	struct CachedComponents
	{
		Hostile::MassProperties massProps;
		Hostile::Velocity velocity;
		DirectX::SimpleMath::Matrix modelMatrix;
	};

	template<typename T>
	T* safe_get(flecs::entity e) {
		if (e.has<T>()) {
			return e.get_mut<T>();
		}
		return nullptr;
	}
}

namespace Hostile {

	ADD_SYSTEM(ResolveCollisionSys);

	void ResolveCollisionSys::ApplyImpulses(flecs::entity _e1, flecs::entity _e2, float _jacobianImpulse, const Vector3& _r1, const Vector3& _r2, const Vector3& _direction, bool _isOtherEntityRigidBody) {
		Vector3 linearImpulse = _direction * _jacobianImpulse;
		Vector3 angularImpulse1 = _r1.Cross(_direction) * _jacobianImpulse;
		Vector3 angularImpulse2 = _r2.Cross(_direction) * _jacobianImpulse;

		const MassProperties* massProps1 = _e1.get<MassProperties>();
		const InertiaTensor* inertiaTensor1 = _e1.get<InertiaTensor>();
		Velocity* velPtr = _e1.get_mut<Velocity>();
		Transform* trnsPtr = _e1.get_mut<Transform>();

		Velocity updatedVel;

		Vector3 s, t;
		Quaternion rot;
		trnsPtr->matrix.Decompose(s, rot, t);
		Matrix rotMat = Matrix::CreateFromQuaternion(rot);
		Vector3 localAngularVel = Vector3::Transform(velPtr->angular, rotMat);
		localAngularVel += inertiaTensor1->inverseInertiaTensorWorld * angularImpulse1;
		velPtr->linear += massProps1->inverseMass * linearImpulse;
		velPtr->angular = Vector3::Transform(localAngularVel, rotMat.Transpose());

		if (_isOtherEntityRigidBody) {
			const MassProperties* massProps2 = _e2.get<MassProperties>();
			const InertiaTensor* inertiaTensor2 = _e2.get<InertiaTensor>();
			velPtr = _e2.get_mut<Velocity>();
			trnsPtr = _e2.get_mut<Transform>();

			trnsPtr->matrix.Decompose(s, rot, t);
			rotMat = Matrix::CreateFromQuaternion(rot);
			localAngularVel = Vector3::Transform(velPtr->angular, rotMat);
			localAngularVel -= inertiaTensor2->inverseInertiaTensorWorld * angularImpulse2;
			velPtr->linear -= massProps2->inverseMass * linearImpulse;
			velPtr->angular = Vector3::Transform(localAngularVel, rotMat.Transpose());
		}
	}

	float ResolveCollisionSys::ComputeTangentialImpulses(const flecs::entity& e1, const flecs::entity& e2, const Vector3& r1, const Vector3& r2, const Vector3& tangent, bool isOtherEntityRigidBody) {

		auto massProp1 = safe_get<MassProperties>(e1);
		auto inertiaTensor1 = safe_get<InertiaTensor>(e1);
		auto vel1 = safe_get<Velocity>(e1);
		auto t1 = safe_get< Transform>(e1);

		float inverseMassSum = massProp1 ? massProp1->inverseMass : 0.0f;
		Vector3 termInDenominator1;
		if (inertiaTensor1) {
			termInDenominator1 = (inertiaTensor1->inverseInertiaTensorWorld * r1.Cross(tangent)).Cross(r1);
		}

		Vector3 termInDenominator2;
		if (isOtherEntityRigidBody) {
			auto massProp2 = e2.get<MassProperties>();
			auto inertiaTensor2 = e2.get<InertiaTensor>();
			auto vel2 = e2.get<Velocity>();

			inverseMassSum += massProp2->inverseMass;
			termInDenominator2 = (inertiaTensor2->inverseInertiaTensorWorld * r2.Cross(tangent)).Cross(r2);
		}

		// Compute the effective mass for the friction/tangential _direction
		float effectiveMassTangential = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(tangent);
		if (fabs(effectiveMassTangential) < FLT_EPSILON) {
			return 0.f;
		}

		// Calculate relative velocities along the tangent
		Vector3 relativeVel = vel1->linear + (Extract3x3Matrix(t1->orientation) * vel1->angular).Cross(r1);
		if (isOtherEntityRigidBody) {
			auto t2 = safe_get< Transform>(e2);
			auto vel2 = e2.get<Velocity>();
			relativeVel -= (vel2->linear + (Extract3x3Matrix(t2->orientation) * vel2->angular).Cross(r2));
		}

		float relativeSpeedTangential = relativeVel.Dot(tangent);

		auto collisionData = safe_get<CollisionData>(e1);

		// Compute the frictional impulse
		float frictionImpulseMagnitude = -relativeSpeedTangential / effectiveMassTangential;

		// Clamp based on Coulomb's law
		if (collisionData) {
			float maxFriction = collisionData->friction * collisionData->accumulatedNormalImpulse;
			frictionImpulseMagnitude = std::clamp(frictionImpulseMagnitude, -maxFriction, maxFriction);
		}

		return frictionImpulseMagnitude;
	}

	void ResolveCollisionSys::ApplyFrictionImpulses(flecs::entity e1, flecs::entity e2, const Vector3& r1, const Vector3& r2, const Vector3& collisionNormal, bool isOtherEntityRigidBody)
	{
		Vector3 tangent1, tangent2;

		//erin catto - Box2D
		if (abs(collisionNormal.x) >= 0.57735f) {
			tangent1 = Vector3(collisionNormal.y, -collisionNormal.x, 0.0f);
		}
		else {
			tangent1 = Vector3(0.0f, collisionNormal.z, -collisionNormal.y);
		}
		tangent2 = collisionNormal.Cross(tangent1);

		// Compute the impulses in each _direction and apply
		float jacobianImpulseT1 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent1, isOtherEntityRigidBody);
		ApplyImpulses(e1, e2, jacobianImpulseT1, r1, r2, tangent1, isOtherEntityRigidBody);

		float jacobianImpulseT2 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent2, isOtherEntityRigidBody);
		ApplyImpulses(e1, e2, jacobianImpulseT2, r1, r2, tangent2, isOtherEntityRigidBody);
	}

	void ResolveCollisionSys::OnCreate(flecs::world& _world)
	{
		_world.system<CollisionData>("ResolveCollisionSys")
			.kind(IEngine::Get().GetResolveCollisionPhase())
			.iter(ResolveCollisionSys::OnUpdate);

		//systems are executed in the order they are added. so, after OnUpdate().
		_world.system<CollisionData>("SendAndCleanupCollisions")
			.kind(IEngine::Get().GetResolveCollisionPhase())
			.iter(SendAndCleanupCollisionData);
	}

	void ResolveCollisionSys::OnUpdate(flecs::iter& _it,
		CollisionData* _collisionDatas)
	{
		constexpr int SOLVER_ITERS = 5;
		for (int iter{}; iter < SOLVER_ITERS; ++iter)
		{
			for (int i{}; i < _it.count(); i++)
			{
				flecs::entity e1 = _collisionDatas[i].entity1;
				flecs::entity e2 = _collisionDatas[i].entity2;
				const MassProperties* m1 = e1.get<MassProperties>();
				const MassProperties* m2 = e2.get<MassProperties>();
				const Transform* t1 = e1.get<Transform>();
				const Transform* t2 = e2.get<Transform>();
				const InertiaTensor* inertia1 = e1.get<InertiaTensor>();
				const InertiaTensor* inertia2 = e2.get<InertiaTensor>();
				const Velocity* vel1 = e1.get<Velocity>();
				const Velocity* vel2 = e2.get<Velocity>();

				float inverseMassSum = m1->inverseMass;
				bool isOtherEntityRigidBody = e2.has<Rigidbody>();

				if (isOtherEntityRigidBody)
				{
					inverseMassSum += m2->inverseMass;
				}
				if (fabs(inverseMassSum) < FLT_EPSILON) {
					continue;
				}

				// Contact point relative to the body's position
				Vector3 r1 = _collisionDatas[i].contactPoints.first - t1->position;
				Vector3 r2;
				if (isOtherEntityRigidBody) {
					r2 = _collisionDatas[i].contactPoints.second - t2->position;
				}

				// Inverse inertia tensors
				Matrix3 i1 = inertia1->inverseInertiaTensorWorld;
				Matrix3 i2;
				if (isOtherEntityRigidBody) {
					i2 = inertia2->inverseInertiaTensorWorld;
				}

				// Denominator terms                
				Vector3 termInDenominator1 = (i1 * r1.Cross(_collisionDatas[i].collisionNormal)).Cross(r1);
				Vector3 termInDenominator2;
				if (isOtherEntityRigidBody) {
					termInDenominator2 = (i2 * r2.Cross(_collisionDatas[i].collisionNormal)).Cross(r2);
				}

				// Compute the final effective mass
				float effectiveMass = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(_collisionDatas[i].collisionNormal);
				if (fabs(effectiveMass) < FLT_EPSILON) {
					return;
				}


				// Relative velocities
				Vector3 relativeVel = vel1->linear + (Extract3x3Matrix(t1->orientation) * vel1->angular).Cross(r1);
				if (isOtherEntityRigidBody) {
					relativeVel -= vel2->linear + (Extract3x3Matrix(t2->orientation) * vel2->angular).Cross(r2);
				}

				float relativeSpeed = relativeVel.Dot(_collisionDatas[i].collisionNormal);
				// Baumgarte Stabilization (for penetration resolution)
				static constexpr float PENETRATION_TOLERANCE = 0.000075f; //temp
				//fewer solver iteration, higher precision
				static constexpr float CORRECTION_RATIO = 0.2f;
				float baumgarte = 0.0f;
				if (_collisionDatas[i].penetrationDepth > PENETRATION_TOLERANCE) {
					baumgarte = static_cast<float>(
						(_collisionDatas[i].penetrationDepth - PENETRATION_TOLERANCE) * (CORRECTION_RATIO / _it.delta_time())
						);
				}
				static constexpr float CLOSING_SPEED_TOLERANCE = 0.00005f; //temp
				float restitutionTerm = 0.0f;
				if (relativeSpeed > CLOSING_SPEED_TOLERANCE) {
					restitutionTerm = _collisionDatas[i].restitution * (relativeSpeed - CLOSING_SPEED_TOLERANCE);
				}

				// Compute the impulse
				float jacobianImpulse = ((-(1 + restitutionTerm) * relativeSpeed) + baumgarte) / effectiveMass;

				if (isnan(jacobianImpulse)) {
					return;
				}

				// Compute the total impulse applied so far to maintain non-penetration
				float prevImpulseSum = _collisionDatas[i].accumulatedNormalImpulse;
				_collisionDatas[i].accumulatedNormalImpulse += jacobianImpulse;
				if (_collisionDatas[i].accumulatedNormalImpulse < 0.0f) {//std::max
					_collisionDatas[i].accumulatedNormalImpulse = 0.0f;
				}

				jacobianImpulse = _collisionDatas[i].accumulatedNormalImpulse - prevImpulseSum;

				// Apply impulses to the bodies
				ApplyImpulses(e1, e2, jacobianImpulse, r1, r2, _collisionDatas[i].collisionNormal, isOtherEntityRigidBody);

				// Compute and apply frictional impulses using the two tangents
				ApplyFrictionImpulses(e1, e2, r1, r2, _collisionDatas[i].collisionNormal, isOtherEntityRigidBody);
			}
		}
	}
	void ResolveCollisionSys::SendAndCleanupCollisionData(flecs::iter& _it, CollisionData* _collisionDatas)
	{
		for (auto e : _it)
		{
			_it.entity(e).remove<CollisionData>();
		}
	}

	void ResolveCollisionSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
	{
	}

	void ResolveCollisionSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
	{
	}

	void ResolveCollisionSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
	{
	}


}
