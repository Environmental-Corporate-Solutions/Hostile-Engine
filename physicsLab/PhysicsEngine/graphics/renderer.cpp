#include "glad/glad.h"
#include "renderer.h"
#include "math/matrixConversion.h"
#include <iostream>
#include <typeinfo>

using namespace graphics;


Renderer::Renderer(const char* title)
    :m_camera{}, 
    m_sceneWidth(1280), 
    m_sceneHeight(720)
{
    if (!glfwInit()) {
        throw std::runtime_error("Renderer::glfwInit() error");
    }
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    m_window = glfwCreateWindow(1280, 720, title, NULL, NULL);
    if (!m_window){
        glfwTerminate();
        throw std::runtime_error("Renderer::glfwCreateWindow() error");
    }
    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        glfwTerminate();
        throw std::runtime_error("Renderer::gladLoadGLLoader() error");
    }

	m_objectShader = Shader({
		{ "shaders/object_vert.glsl",GL_VERTEX_SHADER},
		{"shaders/object_frag.glsl",GL_FRAGMENT_SHADER }
		});

    glClearColor(0.3f, 0.1f, 0.1f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

}

Renderer::~Renderer(){
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

GLFWwindow* Renderer::GetWindow() const
{
    return m_window;
}

void Renderer::AddBoxShape(RigidObject* obj)
{
    if (!obj) {
        throw std::runtime_error("Renderer::AddGraphicalShape, invalid object type");
    }
	m_shapes[obj] = std::make_unique<Sphere>();
	obj->shape = m_shapes[obj].get();
}

void Renderer::RenderObject(RigidObject* obj)
{
    glm::mat4 view = m_camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(
        glm::radians(m_camera.m_fov),
        ((float) m_sceneWidth) / m_sceneHeight,
        NEAR_PLANE,
        FAR_PLANE
    );

    m_objectShader.Use();
    m_objectShader.SetMat4("model", math::ConvertToGLM(obj->rigidBody->m_model));
    m_objectShader.SetMat4("view", view);
    m_objectShader.SetMat4("projection", projection);
    m_objectShader.SetVec3("viewPos", m_camera.m_position);

    Shape *objectShape = m_shapes.find(obj)->second.get();
    glBindVertexArray(objectShape->m_polygonVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)objectShape->m_polygonIndices.size(), GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);

    m_objectShader.Unbind();
}

void graphics::Renderer::Clear() const {
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
}


void Renderer::SetSceneViewport(){
    glViewport(0, 0, m_sceneWidth, m_sceneHeight);
}