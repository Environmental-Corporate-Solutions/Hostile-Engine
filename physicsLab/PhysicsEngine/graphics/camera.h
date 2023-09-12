#pragma once

#include "GLFW/glfw3.h"
#include "opengl/glm/glm.hpp"

namespace graphics
{
    struct Camera
    {
        glm::vec3 m_position;
        glm::vec3 m_target;
        glm::vec3 m_up;
        glm::vec3 m_right;

        glm::vec3 m_worldUp;

        float m_fov;

    //public:
        Camera();

        glm::mat4 GetViewMatrix() const;
        glm::vec3 GetViewPlaneNormal() const;
    };    
}