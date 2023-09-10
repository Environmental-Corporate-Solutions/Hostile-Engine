#pragma once
#include "body.h"
#include "engine/contact.h"
#include <functional>//std::function
#include <vector>

namespace physics
{
	//--------------------------------
	struct Constraint {
		virtual ~Constraint() = 0;
	};
	inline Constraint::~Constraint(){}
	
	struct Collider{
		RigidBody* rigidBody{ nullptr };
		virtual ~Collider() = 0;
	};
	inline Collider::~Collider() {}
	//--------------------------------

	struct Plane : public Constraint{
		~Plane() override final{}

		Vector3 m_normal;
		float m_offset;

		Plane(Vector3 Normal, float offset) :m_normal{ Normal }, m_offset{offset}
		{}
	};

	struct SphereCollider : public Collider
	{
		float m_radius;

		SphereCollider(RigidBody* Body, float Radius) :m_radius{ Radius } {
			rigidBody = Body;
		}
	};
}

