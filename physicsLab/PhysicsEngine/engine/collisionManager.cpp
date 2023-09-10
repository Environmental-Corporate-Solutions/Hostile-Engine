#include <cmath>
#include <cfloat>
#include <typeinfo>
#include <algorithm>//std::clamp
#include <array>
#include <iostream>
#include "collisionManager.h"
#include "simulator/object.h"

using namespace physics;

void CollisionSystem::DetectCollision(const std::vector<std::unique_ptr<RigidObject>>& objects,const std::vector<std::unique_ptr<Constraint>>& constraints){
    for (auto i = objects.begin(); i != objects.end(); ++i)
    {
        Collider* collider1 = (*i)->collider;
        //(1) Rigid Bodies
        for (auto j = std::next(i, 1); j != objects.end(); ++j)
        {
            Collider* collider2 = (*j)->collider;

            FindCollisionFeatures(collider1, collider2);
        }

        //(2) constraints     
        for (auto& constraint : constraints) {
            FindCollisionFeatures(collider1, constraint.get());
        }
    }

}

bool physics::CollisionSystem::FindCollisionFeatures(const Collider* shape1, const Collider* shape2){
    if (typeid(*shape1) == typeid(SphereCollider))
    {
        const SphereCollider* sphere = static_cast<const SphereCollider*>(shape1);
        if (typeid(*shape2) == typeid(SphereCollider))
        {
            return FindCollisionFeatures(sphere, static_cast<const SphereCollider*>(shape2));
        }
    }
    return false;
}

bool CollisionSystem::FindCollisionFeatures(const SphereCollider* sphere1,const SphereCollider* sphere2,bool isForBroadPhaseTest){
    float distanceSquared = (sphere1->rigidBody->m_position - sphere2->rigidBody->m_position).LengthSquared();

    float radiusSum = sphere1->m_radius + sphere2->m_radius;
    if (distanceSquared > radiusSum * radiusSum) {
        return false;
    }
    if (isForBroadPhaseTest == true) {//no need to calc details
        return true;
    }

	Vector3 normal = sphere1->rigidBody->m_position - sphere2->rigidBody->m_position;
	normal.Normalize();

    CollisionData newContact;
    newContact.m_bodies[0] = sphere1->rigidBody;
	newContact.m_bodies[1] = sphere2->rigidBody;
	newContact.m_collisionNormal = normal;
    newContact.m_contactPoints.push_back({Vector3(sphere1->rigidBody->m_position - normal * sphere1->m_radius),Vector3(sphere2->rigidBody->m_position + normal * sphere2->m_radius)});
	newContact.m_penetrationDepth = radiusSum - sqrtf(distanceSquared);
	newContact.m_restitution = m_objectRestitution;
	newContact.m_friction = m_friction;
    m_contacts.push_back(newContact);

    return true;
}

bool CollisionSystem::FindCollisionFeatures(const SphereCollider* sphere, const Plane* plane){
    float distance = std::abs(plane->m_normal.Dot(sphere->rigidBody->m_position)-plane->m_offset);

    if (distance > sphere->m_radius) {
        return false;
    }

	CollisionData newContact;
	newContact.m_bodies[0] = sphere->rigidBody;
	newContact.m_bodies[1] = nullptr;
	newContact.m_collisionNormal = plane->m_normal;
	newContact.m_contactPoints.push_back({Vector3(sphere->rigidBody->m_position - plane->m_normal * distance),Vector3{}});
	newContact.m_penetrationDepth = sphere->m_radius - distance;
	newContact.m_restitution = m_groundRestitution;
	newContact.m_friction = m_friction;
	m_contacts.push_back(newContact);
	return true;
}

bool physics::CollisionSystem::FindCollisionFeatures(const Collider* collider, const Constraint* constraint){
    if (typeid(*collider) == typeid(SphereCollider)){
        const SphereCollider* sphere = static_cast<const SphereCollider*>(collider);
        if (typeid(*constraint) == typeid(Plane)) {
            return FindCollisionFeatures(sphere, static_cast<const Plane*>(constraint));
        }
    }
    return false;
}

//https://allenchou.net/2013/12/game-physics-constraints-sequential-impulse/
void CollisionSystem::SequentialImpulse(CollisionData contact, double dt, int j) {
    // Compute the effective mass
    float inverseMassSum = contact.m_bodies[0]->m_inverseMass;
    if (contact.m_bodies[1]) {
        inverseMassSum += contact.m_bodies[1]->m_inverseMass;
    }
    if (inverseMassSum == 0.0f) {
        return;
    }

    // Contact point relative to the body's position
    Vector3 r1 = contact.m_contactPoints[j].first - contact.m_bodies[0]->m_position;
    Vector3 r2;
    if (contact.m_bodies[1]) {
        r2 = contact.m_contactPoints[j].second - contact.m_bodies[1]->m_position;
    }

    // Inverse inertia tensors
    Matrix3 i1 = contact.m_bodies[0]->m_inverseInertiaTensorWorld;
    Matrix3 i2;
    if (contact.m_bodies[1]) {
        i2 = contact.m_bodies[1]->m_inverseInertiaTensorWorld;
    }

    // Denominator terms
    Vector3 termInDenominator1 = (i1 * r1.Cross(contact.m_collisionNormal)).Cross(r1);
    Vector3 termInDenominator2;
    if (contact.m_bodies[1]) {
        termInDenominator2 = (i2 * r2.Cross(contact.m_collisionNormal)).Cross(r2);
    }

    // Compute the final effective mass
    float effectiveMass = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(contact.m_collisionNormal);
    if (effectiveMass == 0.0f) {
        return;
    }

    // Relative velocities
    Vector3 relativeVel = contact.m_bodies[0]->m_velocity + contact.m_bodies[0]->GetAngularVelocity().Cross(r1);
    if (contact.m_bodies[1]) {
        relativeVel -= (contact.m_bodies[1]->m_velocity + contact.m_bodies[1]->GetAngularVelocity().Cross(r2));
    }

    float relativeSpeed = relativeVel.Dot(contact.m_collisionNormal);

    // Baumgarte Stabilization (for penetration resolution)
    float baumgarte = 0.0f;
    if (contact.m_penetrationDepth > m_penetrationTolerance) {
        baumgarte = static_cast<float>(
            (-0.1f / dt) * (contact.m_penetrationDepth - m_penetrationTolerance)
            );
    }

    float restitutionTerm = 0.0f;
    if (relativeSpeed > m_closingSpeedTolerance) {
        restitutionTerm = contact.m_restitution * (relativeSpeed - m_closingSpeedTolerance);
    }

    float bias = baumgarte - restitutionTerm;

    // Compute the impulse
    float jacobianImpulse = -(relativeSpeed + bias) / effectiveMass;

    if (isnan(jacobianImpulse)) {
        //std::cout << "CollisionResolver::SequentialImpulse(), impulse NAN" << std::endl;
        return;
    }

    if (contact.m_contactPoints.size() > 1 && jacobianImpulse != 0.0f) {
        jacobianImpulse /= (float)contact.m_contactPoints.size();
    }

    // Compute the total impulse applied so far to maintain non-penetration
    float prevImpulseSum = contact.m_accumulatedNormalImpulse;
    contact.m_accumulatedNormalImpulse += jacobianImpulse;
    if (contact.m_accumulatedNormalImpulse < 0.0f) {
        contact.m_accumulatedNormalImpulse = 0.0f;
    }
    jacobianImpulse = contact.m_accumulatedNormalImpulse - prevImpulseSum;

    // Apply impulses to the bodies
    ApplyImpulses(contact, jacobianImpulse, r1, r2, contact.m_collisionNormal);

    // Compute and apply frictional impulses using the two tangents
    ApplyFrictionImpulses(contact, r1, r2);
}

void CollisionSystem::ResolveCollision(double deltaTime){
    for (int i = 0; i < m_solverIterations; ++i){
        for (auto& contact : m_contacts){
            const int contactsSize = static_cast<int>(contact.m_contactPoints.size());
            for (int j{}; j < contactsSize; ++j) {
                SequentialImpulse(contact, deltaTime, j);
            }
        }
    }
}

float CollisionSystem::ComputeTangentialImpulses(CollisionData& contact, const Vector3& r1, const Vector3& r2, const Vector3& tangent) {
    float inverseMassSum = contact.m_bodies[0]->m_inverseMass;
    if (contact.m_bodies[1]) {
        inverseMassSum += contact.m_bodies[1]->m_inverseMass;
    }

    Vector3 termInDenominator1 = (contact.m_bodies[0]->m_inverseInertiaTensorWorld * r1.Cross(tangent)).Cross(r1);
    Vector3 termInDenominator2;
    if (contact.m_bodies[1]) {
        termInDenominator2 = (contact.m_bodies[1]->m_inverseInertiaTensorWorld * r2.Cross(tangent)).Cross(r2);
    }

    // Compute the effective mass for the friction/tangential direction
    float effectiveMassTangential = inverseMassSum + (termInDenominator1 + termInDenominator2).Dot(tangent);
    if (effectiveMassTangential == 0.0f) {
        return 0.0f;
    }

    // Calculate relative velocities along the tangent
    Vector3 relativeVel = contact.m_bodies[0]->m_velocity + contact.m_bodies[0]->GetAngularVelocity().Cross(r1);
    if (contact.m_bodies[1]) {
        relativeVel -= (contact.m_bodies[1]->m_velocity + contact.m_bodies[1]->GetAngularVelocity().Cross(r2));
    }

    float relativeSpeedTangential = relativeVel.Dot(tangent);

    // Compute the frictional impulse
    float frictionImpulseMagnitude = -relativeSpeedTangential / effectiveMassTangential;

    // Coulomb's law: The frictional impulse should not be greater than the friction coefficient times the normal impulse
    float maxFriction = contact.m_friction * contact.m_accumulatedNormalImpulse;
    frictionImpulseMagnitude = std::clamp(frictionImpulseMagnitude, -maxFriction, maxFriction);

    return frictionImpulseMagnitude;
}

void CollisionSystem::ApplyFrictionImpulses(CollisionData& contact, const Vector3& r1, const Vector3& r2) {

    // Compute the two friction directions
    Vector3 tangent1, tangent2;

    //erin catto - Box2D
    if (abs(contact.m_collisionNormal.x) >= 0.57735f) {
        tangent1 = Vector3(contact.m_collisionNormal.y, -contact.m_collisionNormal.x, 0.0f);
    }
    else {
        tangent1 = Vector3(0.0f, contact.m_collisionNormal.z, -contact.m_collisionNormal.y);
    }
    tangent2 = contact.m_collisionNormal.Cross(tangent1);

    // Compute the impulses in each direction and apply
    float jacobianImpulseT1 = ComputeTangentialImpulses(contact, r1, r2, tangent1);
    ApplyImpulses(contact, jacobianImpulseT1, r1, r2, tangent1);

    float jacobianImpulseT2 = ComputeTangentialImpulses(contact, r1, r2, tangent2);
    ApplyImpulses(contact, jacobianImpulseT2, r1, r2, tangent2);
}

void CollisionSystem::ApplyImpulses(CollisionData& contact, float jacobianImpulse, const Vector3& r1, const Vector3& r2, const Vector3& direction) {
    Vector3 linearImpulse = direction * jacobianImpulse;
    Vector3 angularImpulse1 = r1.Cross(direction) * jacobianImpulse;
    Vector3 angularImpulse2 = r2.Cross(direction) * jacobianImpulse;

    contact.m_bodies[0]->m_velocity += linearImpulse * contact.m_bodies[0]->m_inverseMass;
    contact.m_bodies[0]->SetAngularVelocity(
        contact.m_bodies[0]->GetAngularVelocity() + contact.m_bodies[0]->m_inverseInertiaTensorWorld * angularImpulse1
    );
    if (contact.m_bodies[1]) {
        contact.m_bodies[1]->m_velocity -= linearImpulse * contact.m_bodies[1]->m_inverseMass;
        contact.m_bodies[1]->SetAngularVelocity(
            contact.m_bodies[1]->GetAngularVelocity() - contact.m_bodies[1]->m_inverseInertiaTensorWorld * angularImpulse2
        );
    }
}