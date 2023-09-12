#include "camera.h"
#include <opengl/glm/gtc/quaternion.hpp>

using namespace graphics;

Camera::Camera()
{
    m_position = glm::vec3(15.0f, 15.0f, 15.0f);
    m_target = glm::vec3(.0f, .0f, 0.f);
    m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    m_fov = 30.0f;

    glm::vec3 viewVector = glm::normalize(m_target - m_position); 
    m_right = glm::normalize(glm::cross(viewVector, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, viewVector));
}

glm::mat4 Camera::GetViewMatrix() const{
    return glm::lookAtRH(m_position, m_target, m_up);//OpenGL <- Right Handed
}

glm::vec3 Camera::GetViewPlaneNormal() const{
    glm::vec3 viewPlaneNormal = glm::cross(m_up, m_right);
    return glm::normalize(viewPlaneNormal);
}