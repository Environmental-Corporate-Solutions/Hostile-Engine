#include "physicsSystem.h"
#include "simulator/object.h"
#include <iterator>
#include <typeinfo>
#include <cmath>
#include <iostream>

using namespace physics;

float PhysicsSystem::m_gravity = 9.8f;

PhysicsSystem::PhysicsSystem()  {
    m_constraints.emplace_back(std::make_unique<Plane>(Vector3(0.0f, 1.0f, 0.0f), 0.0f));
}

void PhysicsSystem::Simulate(double dt)
{
    //0. reset
    collisionSystem.m_contacts.clear();

    //1. gravity
    for (auto& obj : m_objects) {
        obj->rigidBody->m_force+=(Vector3{ 0.f,-m_gravity,0.f } * obj->rigidBody->GetMass());
    }

    //2. detect collisions
    collisionSystem.DetectCollision(m_objects, m_constraints);

    //3. resolve collisions
    collisionSystem.ResolveCollision(dt);

    //4. Integrate
    for (auto& obj : m_objects){
        obj->rigidBody->Integrate(dt);
    }
}

void PhysicsSystem::AddBoxRigidBody(Vector3 pos,RigidObject* obj)
{
    if (!obj) {
        throw std::runtime_error("PhysicsWorld::AddBoxRigidBody(), empty object pointer");
    }
    RigidBody* newBody = new RigidBody;
    newBody->SetMass(5.0f);
    newBody->m_position=pos;
    newBody->m_linearAcceleration = { 0.0f, -m_gravity, 0.0f };

    Matrix3 inertiaTensor;

	float value = 0.4f * newBody->GetMass();
	inertiaTensor.SetDiagonal(value);

    newBody->SetInertiaTensor(inertiaTensor);

    obj->rigidBody=newBody;
}

void PhysicsSystem::AddBoxCollider(RigidBody* rigidBody, RigidObject* obj){
    if (!obj) {
        throw std::runtime_error("PhysicsWorld::AddBoxCollider(), empty object pointer");
    }
    Collider* newCollider{nullptr};

	newCollider = new SphereCollider(rigidBody, 1.0f);
    obj->collider=newCollider;
}

