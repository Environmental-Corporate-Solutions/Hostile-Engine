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

    void ResolveCollisionSys::ApplyImpulses(flecs::entity e1, flecs::entity e2, float jacobianImpulse, const Vector3& r1, const Vector3& r2, const Vector3& direction, bool isOtherEntityRigidBody) {
        Vector3 linearImpulse = direction * jacobianImpulse;
        Vector3 angularImpulse1 = r1.Cross(direction) * jacobianImpulse;
        Vector3 angularImpulse2 = r2.Cross(direction) * jacobianImpulse;

        const Velocity* vel1Ptr = e1.get<Velocity>();
        const MassProperties* massProps1 = e1.get<MassProperties>();
        const InertiaTensor* inertiaTensor1 = e1.get<InertiaTensor>();

        Velocity updatedVel1;
        updatedVel1.linear = vel1Ptr->linear + linearImpulse * massProps1->inverseMass;
        updatedVel1.angular = vel1Ptr->angular + inertiaTensor1->inverseInertiaTensorWorld * angularImpulse1;

        e1.set<Velocity>(updatedVel1);

        if (isOtherEntityRigidBody) { 
            const Velocity* vel2Ptr = e2.get<Velocity>();
            const MassProperties* massProps2 = e2.get<MassProperties>();
            const InertiaTensor* inertiaTensor2 = e2.get<InertiaTensor>();

            Velocity updatedVel2;
            updatedVel2.linear = vel2Ptr->linear - linearImpulse * massProps2->inverseMass;
            updatedVel2.angular = vel2Ptr->angular - inertiaTensor2->inverseInertiaTensorWorld * angularImpulse2;

            e2.set<Velocity>(updatedVel2);
        }
    }

    float ResolveCollisionSys::ComputeTangentialImpulses(const flecs::entity& e1, const flecs::entity& e2, const Vector3& r1, const Vector3& r2, const Vector3& tangent, bool isOtherEntityRigidBody) {

        auto massProp1 = safe_get<MassProperties>(e1);
        auto inertiaTensor1 = safe_get<InertiaTensor>(e1);
        auto vel1 = safe_get<Velocity>(e1);

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

        // Compute the effective mass for the friction/tangential direction
        float effectiveMassTangential = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(tangent);
        if (effectiveMassTangential == 0.0f) {
            return 0.0f;
        }

        // Calculate relative velocities along the tangent
        Vector3 relativeVel = (vel1 ? vel1->linear : Vector3()) + (vel1 ? vel1->angular.Cross(r1) : Vector3());
        if (isOtherEntityRigidBody) {
            auto vel2 = e2.get<Velocity>();
            relativeVel -= (vel2->linear + vel2->angular.Cross(r2));
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

    void ResolveCollisionSys::ApplyFrictionImpulses(flecs::entity e1, flecs::entity e2, const Vector3& r1, const Vector3& r2, bool isOtherEntityRigidBody)
    {
        auto collisionData = e1.get<CollisionData>();
        if (!collisionData) {
            // Log the error
            return;
        }

        Vector3 tangent1, tangent2;

        //erin catto - Box2D
        if (abs(collisionData->collisionNormal.x) >= 0.57735f) {
            tangent1 = Vector3(collisionData->collisionNormal.y, -collisionData->collisionNormal.x, 0.0f);
        }
        else {
            tangent1 = Vector3(0.0f, collisionData->collisionNormal.z, -collisionData->collisionNormal.y);
        }
        tangent2 = collisionData->collisionNormal.Cross(tangent1);

        // Compute the impulses in each direction and apply
        float jacobianImpulseT1 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent1, isOtherEntityRigidBody);
        ApplyImpulses(e1, e2, jacobianImpulseT1, r1, r2, tangent1, isOtherEntityRigidBody);

        float jacobianImpulseT2 = ComputeTangentialImpulses(e1, e2, r1, r2, tangent2, isOtherEntityRigidBody);
        ApplyImpulses(e1, e2, jacobianImpulseT2, r1, r2, tangent2, isOtherEntityRigidBody);
    }

    void ResolveCollisionSys::OnCreate(flecs::world& _world)
    {
		//_world.system<CollisionData,Transform, MassProperties, Velocity, Force, Matrix, InertiaTensor>("ResolveCollisionSys")
		//	.kind(IEngine::Get().GetResolveCollisionPhase())
		//	.iter(OnUpdate);
        _world.system<CollisionData, Transform, MassProperties, Velocity, Force, Matrix, InertiaTensor>("ResolveCollisionSys")
            .kind(IEngine::Get().GetResolveCollisionPhase())
            .iter(ResolveCollisionSys::OnUpdate);
    }

    void ResolveCollisionSys::OnUpdate(flecs::iter& _it,
        CollisionData* _collisionDatas,
        Transform* _transform,
        MassProperties* _massProps,
        Velocity* _velocities,
        Force* _forces,
        Matrix* _modelMatrices,
        InertiaTensor* _inertiaTensors)
    {
        float dt = _it.delta_time();
        constexpr int SOLVER_ITERS = 25;

        for (int iter{}; iter < SOLVER_ITERS; ++iter)
        {
            for (int i{}; i < _it.count(); i++)
            {
                float inverseMassSum = _massProps[i].inverseMass;
                bool isOtherEntityRigidBody = !_collisionDatas[i].otherEntity.has<Constraint>();
                if (isOtherEntityRigidBody)
                {
                    inverseMassSum += _collisionDatas[i].otherEntity.get<MassProperties>()->inverseMass;
                }
                if (inverseMassSum == 0.f) {//TODO::Epsilon
                    continue;
                }

                // Contact point relative to the body's position
                Vector3 r1 = _collisionDatas->contactPoints.first - _transform->position;
                Vector3 r2;
                if (isOtherEntityRigidBody) {
                    r2 = _collisionDatas->contactPoints.second - _collisionDatas[i].otherEntity.get<Transform>()->position;
                }

                // Inverse inertia tensors
                Matrix3 i1 = _inertiaTensors->inverseInertiaTensorWorld;
                Matrix3 i2;
                if (isOtherEntityRigidBody) {
                    i2=_collisionDatas[i].otherEntity.get<InertiaTensor>()->inverseInertiaTensorWorld;
                }

                // Denominator terms                
                Vector3 termInDenominator1 = (i1 * r1.Cross(_collisionDatas->collisionNormal)).Cross(r1);
                Vector3 termInDenominator2;
                if (isOtherEntityRigidBody) {
                    termInDenominator2 = (i2 * r2.Cross(_collisionDatas->collisionNormal)).Cross(r2);
                }

                // Compute the final effective mass
                float effectiveMass = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(_collisionDatas->collisionNormal);
                if (effectiveMass == 0.0f) {
                    return;
                }

                // Relative velocities
                Vector3 relativeVel = _velocities->linear + _velocities->angular.Cross(r1);
                if (isOtherEntityRigidBody) {
                    relativeVel -= _collisionDatas[i].otherEntity.get<Velocity>()->linear + _collisionDatas[i].otherEntity.get<Velocity>()->angular.Cross(r2);
                }

                float relativeSpeed = relativeVel.Dot(_collisionDatas->collisionNormal);

                // Baumgarte Stabilization (for penetration resolution)
                static constexpr float PENETRATION_TOLERANCE = 0.0005f; //temp
                float baumgarte = 0.0f;
                if (_collisionDatas->penetrationDepth > PENETRATION_TOLERANCE) {
                    baumgarte = static_cast<float>(
                        (-0.1f / dt) * (_collisionDatas->penetrationDepth - PENETRATION_TOLERANCE)
                        );
                }

                static constexpr float CLOSING_SPEED_TOLERANCE = 0.005f; //temp
                float restitutionTerm = 0.0f;
                if (relativeSpeed > CLOSING_SPEED_TOLERANCE) {
                    restitutionTerm = _collisionDatas->restitution * (relativeSpeed - CLOSING_SPEED_TOLERANCE);
                }

                float bias = baumgarte - restitutionTerm;

                // Compute the impulse
                float jacobianImpulse = -(relativeSpeed + bias) / effectiveMass;

                if (isnan(jacobianImpulse)) {
                    //TODO:: log NAN impulse error
                    return;
                }

                // Compute the total impulse applied so far to maintain non-penetration
                float prevImpulseSum = _collisionDatas->accumulatedNormalImpulse;
                _collisionDatas->accumulatedNormalImpulse += jacobianImpulse;
                if (_collisionDatas->accumulatedNormalImpulse < 0.0f) {
                    _collisionDatas->accumulatedNormalImpulse = 0.0f;
                }
                jacobianImpulse = _collisionDatas->accumulatedNormalImpulse - prevImpulseSum;

                // Apply impulses to the bodies
                ApplyImpulses(_it.entity(i), _collisionDatas[i].otherEntity, jacobianImpulse, r1, r2, _collisionDatas[i].collisionNormal, isOtherEntityRigidBody);

                // Compute and apply frictional impulses using the two tangents
                ApplyFrictionImpulses(_it.entity(i), _collisionDatas[i].otherEntity, r1, r2, isOtherEntityRigidBody);

            }
        }
    }


}
