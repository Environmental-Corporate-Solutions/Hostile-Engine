#pragma once

#include "graphics/renderer.h"
#include "object.h"
#include "graphics/textureImage.h"
#include "engine/physicsSystem.h"
#include <memory>//unique_ptr
#include <unordered_map>
#include <vector>
#include <queue>

struct Simulator
{
    bool isRunning;
    physics::PhysicsSystem physicsSystem;
    graphics::Renderer renderer;

//public:
    Simulator();
    
    void Run();
    SphereObject* AddSphere(Vector3 pos = {0.f,1.f,0.f});    
    void HandleKeyboardInput();
};

