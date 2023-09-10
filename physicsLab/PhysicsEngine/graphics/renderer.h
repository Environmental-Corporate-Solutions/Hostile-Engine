#pragma once

#include "shader.h"
#include "shape.h"
#include "simulator/object.h"
#include "textureImage.h"
#include "camera.h"
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <array>
#include <memory>//unique_ptr
#include "DirectXTK/SimpleMath.h"

using DirectX::SimpleMath::Vector3;

namespace graphics
{
    constexpr float NEAR_PLANE = 1.f;
    constexpr float FAR_PLANE = 50.0f;

    struct Renderer
    {
        using Shapes = std::unordered_map<RigidObject*, std::unique_ptr<Shape>>;

        GLFWwindow *m_window;
        int m_sceneWidth, m_sceneHeight;

        Shader m_objectShader;
        Camera m_camera;
        Shapes m_shapes;

    //public:
        Renderer(const char* title="title");
        ~Renderer();

        GLFWwindow* GetWindow() const;
        void SetSceneViewport();
        void Clear() const;

        void RenderObject(RigidObject* obj);
        void AddBoxShape(RigidObject*);
    };
}