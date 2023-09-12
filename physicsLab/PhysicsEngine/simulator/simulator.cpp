#include "simulator.h"
#include "math/mathConstants.h"
#include "graphics/textureImage.h"
#include <typeinfo>
#include <cmath>

Simulator::Simulator()
	: isRunning{ true },physicsSystem {}, renderer {"demo"}
{	
	AddSphere()->rigidBody->m_velocity = { 1.f,0.f,1.f };
				//pos									//vel
	AddSphere({21.f,1.f,30.f})->rigidBody->m_velocity = {-14.f,0.f,-20.f};
}

void Simulator::Run()
{
	double prevTime = glfwGetTime();
	double curTime, dt;

	while (!glfwWindowShouldClose(renderer.GetWindow()))
	{
		curTime = glfwGetTime();
		dt = curTime - prevTime;
		prevTime = curTime;

		HandleKeyboardInput();

		if (isRunning) {
			physicsSystem.Simulate(dt); 
		}

		renderer.Clear();

		//objects
		for (auto& object : physicsSystem.m_objects) {
			renderer.RenderObject(object.get());
		}
		glfwSwapBuffers(renderer.m_window);
		glfwPollEvents();

	}
}

SphereObject* Simulator::AddSphere(Vector3 pos) {
	std::unique_ptr<SphereObject> newObject = std::make_unique<SphereObject>();
	SphereObject* objPtr = newObject.get();

	physicsSystem.AddBoxRigidBody(pos, objPtr);
	physicsSystem.AddBoxCollider(newObject->rigidBody, objPtr);

	renderer.AddBoxShape(objPtr);
	physicsSystem.m_objects.emplace_back(newObject.release());

	return objPtr;
}

void Simulator::HandleKeyboardInput()
{
	if (glfwGetKey(renderer.GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(renderer.GetWindow(), GLFW_TRUE);
	}
}