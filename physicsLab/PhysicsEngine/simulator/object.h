#pragma once

#include "engine/body.h"
#include "engine/collider.h"
#include "graphics/shape.h"
#include "geometry.h"
#include <glm/glm.hpp>

struct RigidObject
{

    physics::RigidBody* rigidBody;
    physics::Collider* collider;
    graphics::Shape* shape;        //Graphical elements <-- to directX

    ObjectType type;

//public:
    RigidObject() : rigidBody{}, collider{}, shape{}, type{} {}

    virtual ObjectType GetObjectType() const = 0;

    graphics::Shape* GetShape() { return shape; }
    const graphics::Shape* GetShape() const { return shape; }
};

struct SphereObject : public RigidObject
{
    float radius;

public:
    SphereObject() : radius(5.0f) {}

    ObjectType GetObjectType() const override final { return ObjectType::SPHERE; }
};
