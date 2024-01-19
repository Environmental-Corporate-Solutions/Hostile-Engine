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
#include "PhysicsProperties.h"
#include "TransformSys.h"//Trnasform

namespace Hostile {

    ADD_SYSTEM(ResolveCollisionSys);

    void ResolveCollisionSys::ApplyImpulses(flecs::entity e1, flecs::entity e2, float jacobianImpulse, const Vector3& r1, const Vector3& r2, const Vector3& direction, Rigidbody* _rb1,Transform* _t1, Transform* _t2,bool isOtherEntityRigidBody) {
        Vector3 linearImpulse = direction * jacobianImpulse;
        Vector3 angularImpulse1 = r1.Cross(direction) * jacobianImpulse;
        Rigidbody* rb1 = e1.get_mut<Rigidbody>();
        const Transform* t1 = e1.get<Transform>();

        rb1->m_linearVelocity += linearImpulse * rb1->m_inverseMass;
        Vector3 localAngularVel = (ExtractRotationMatrix(t1->matrix) * rb1->m_angularVelocity) + rb1->m_inverseInertiaTensorWorld * angularImpulse1;        
        rb1->m_angularVelocity = ExtractRotationMatrix(t1->matrix).Transpose() * localAngularVel;

        if (isOtherEntityRigidBody) {
            Vector3 angularImpulse2 = r2.Cross(direction) * jacobianImpulse;
            Rigidbody* rb2 = e2.get_mut<Rigidbody>();
            const Transform* t2 = e2.get<Transform>();
            rb2->m_linearVelocity -= linearImpulse * rb2->m_inverseMass;
            localAngularVel = (ExtractRotationMatrix(t2->matrix) * rb2->m_angularVelocity) - rb2->m_inverseInertiaTensorWorld * angularImpulse2;
            rb2->m_angularVelocity = ExtractRotationMatrix(t2->matrix).Transpose() * localAngularVel;
        }
    }

    float ResolveCollisionSys::ComputeTangentialImpulses(const flecs::entity& _e1, const flecs::entity& _e2, const Vector3& _r1, const Vector3& _r2, const Vector3& _tangent, Rigidbody* _rb1, const Rigidbody* _rb2, Transform* _t1, Transform* _t2, bool _isOtherEntityRigidBody) {

        float inverseMassSum = _rb1->m_inverseMass;
        Vector3 termInDenominator1;
        if (_rb1) {
            termInDenominator1 = (_rb1->m_inverseInertiaTensorWorld * _r1.Cross(_tangent)).Cross(_r1);
        }

        Vector3 termInDenominator2;
        if (_isOtherEntityRigidBody) {
            inverseMassSum += _rb2->m_inverseMass;
            termInDenominator2 = (_rb2->m_inverseInertiaTensorWorld * _r2.Cross(_tangent)).Cross(_r2);
        }

        // Compute the effective mass for the friction/tangential direction
        float effectiveMassTangential = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(_tangent);
        if (fabs(effectiveMassTangential) < FLT_EPSILON) {
            return 0.f;
        }

		// Calculate relative velocities along the _tangent
		Vector3 relativeVel = _rb1->m_linearVelocity + (ExtractRotationMatrix(_t1->matrix) *_rb1->m_angularVelocity).Cross(_r1);
		if (_isOtherEntityRigidBody) {
			relativeVel -= (_rb2->m_linearVelocity + (ExtractRotationMatrix(_t2->matrix) * _rb2->m_angularVelocity).Cross(_r2));
		}

        float relativeSpeedTangential = relativeVel.Dot(_tangent);

        auto collisionData = _e1.get<CollisionData>();

        // Compute the frictional impulse
        float frictionImpulseMagnitude = -relativeSpeedTangential / effectiveMassTangential;

        // Clamp based on Coulomb's law
        if (collisionData) {
            float maxFriction = collisionData->friction * collisionData->accumulatedNormalImpulse;
            frictionImpulseMagnitude = std::clamp(frictionImpulseMagnitude, -maxFriction, maxFriction);
        }

        return frictionImpulseMagnitude;
    }

    void ResolveCollisionSys::ApplyFrictionImpulses(flecs::entity e1, flecs::entity e2, const Vector3& r1, const Vector3& r2, const Vector3& collisionNormal, Rigidbody* _rb1, const Rigidbody* _rb2, Transform* _t1, Transform* _t2, bool isOtherEntityRigidBody)
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

        // Compute the impulses in each direction and apply
        float jacobianImpulseT1 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent1, _rb1,_rb2,_t1,_t2,isOtherEntityRigidBody);
        ApplyImpulses(e1, e2, jacobianImpulseT1, r1, r2, tangent1, _rb1,_t1,_t2,isOtherEntityRigidBody);

        float jacobianImpulseT2 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent2, _rb1, _rb2, _t1, _t2, isOtherEntityRigidBody);
        ApplyImpulses(e1, e2, jacobianImpulseT2, r1, r2, tangent2, _rb1, _t1, _t2, isOtherEntityRigidBody);
    }

    void ResolveCollisionSys::OnCreate(flecs::world& _world)
    {
        _world.system<CollisionData>("ResolveCollisionSys")
            .kind(IEngine::Get().GetResolveCollisionPhase())
            .rate(PHYSICS_TARGET_FPS_INV) //sys is being updated at a rate of 60 times per second
            .iter(ResolveCollisionSys::OnUpdate);

        //systems are executed in the order they are added. so, after OnUpdate().
        _world.system<CollisionData>("SendAndCleanupCollisions")
            .kind(IEngine::Get().GetResolveCollisionPhase())
            .rate(PHYSICS_TARGET_FPS_INV) //sys is being updated at a rate of 60 times per second
            .iter(SendAndCleanupCollisionData);
    }

	void ResolveCollisionSys::OnUpdate(flecs::iter& _it,
		CollisionData* _collisionDatas)
	{
		constexpr int SOLVER_ITERS = 3;
		for (int iter{}; iter < SOLVER_ITERS; ++iter)
		{
            const size_t Cnt = _it.count();
			for (int i{}; i < Cnt; ++i)
			{
				flecs::entity e1 = _collisionDatas[i].entity1;
				flecs::entity e2 = _collisionDatas[i].entity2;
                if (!e1.is_valid() || !e2.is_valid())
                {
                    return;
                }
                Rigidbody* rb1 = e1.get_mut<Rigidbody>();
                const Rigidbody* rb2 = e2.get<Rigidbody>();
				Transform* t1 = e1.get_mut<Transform>();
				Transform* t2 = e2.get_mut<Transform>();
				Vector3 scl1, pos1, scl2, pos2;
				Quaternion ori1, ori2;
				t1->matrix.Decompose(scl1, ori1, pos1);
				t2->matrix.Decompose(scl2, ori2, pos2);

				float inverseMassSum = rb1->m_inverseMass;
                bool isOtherEntityRigidBody = e2.has<Rigidbody>();

                if (isOtherEntityRigidBody)
                {
                    inverseMassSum += rb2->m_inverseMass;
                }
                if (fabs(inverseMassSum) < FLT_EPSILON) {
                    continue;
                }

				// Contact point relative to the body's position
				Vector3 r1 = _collisionDatas[i].contactPoints.first - pos1;
				Vector3 r2;
				if (isOtherEntityRigidBody) {
					r2 = _collisionDatas[i].contactPoints.second - pos2;
				}

                // Inverse inertia tensors
                Matrix3 i1 = rb1->m_inverseInertiaTensorWorld;
                Matrix3 i2;
                if (isOtherEntityRigidBody) {
                    i2 = rb2->m_inverseInertiaTensorWorld;
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
				Vector3 relativeVel = rb1->m_linearVelocity + (ExtractRotationMatrix(t1->matrix) * rb1->m_angularVelocity).Cross(r1);
				if (isOtherEntityRigidBody) {
					relativeVel -= rb2->m_linearVelocity + (ExtractRotationMatrix(t2->matrix) * rb2->m_angularVelocity).Cross(r2);
				}

				float relativeSpeed = relativeVel.Dot(_collisionDatas[i].collisionNormal);
				// Baumgarte Stabilization (for penetration resolution)
				static constexpr float PENETRATION_TOLERANCE = 0.000075f; //temp
				//fewer solver iteration, higher precision
				static constexpr float CORRECTION_RATIO = 0.25f;
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
                ApplyImpulses(e1, e2, jacobianImpulse, r1, r2, _collisionDatas[i].collisionNormal,rb1,t1,t2, isOtherEntityRigidBody);

                // Compute and apply frictional impulses using the two tangents
                ApplyFrictionImpulses(e1, e2, r1, r2, _collisionDatas[i].collisionNormal, rb1, rb2, t1, t2, isOtherEntityRigidBody);
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
