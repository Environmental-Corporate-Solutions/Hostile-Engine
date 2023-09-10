#include "shader.h"
#include <opengl/glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace graphics;

Shader::Shader(const std::vector<std::pair<std::string, GLenum>>& shaderFiles) {
    for (const auto& shaderFile : shaderFiles) {
        GLuint shader = CreateShader(GetShaderFileContents(shaderFile.first), shaderFile.second);
        m_shaders.push_back(shader);
    }

    m_program = glCreateProgram();
    for (GLuint shader : m_shaders) {
        glAttachShader(m_program, shader);
    }
    glLinkProgram(m_program);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(m_program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    for (GLuint shader : m_shaders) {
        //glDetachShader(m_program, shader);
        glDeleteShader(shader);
    }
}

Shader::Shader(Shader&& other) noexcept
    :m_program{ other.m_program }, m_shaders{std::move(other.m_shaders)}
{
    other.m_program = 0; //invaldiate old shader's program ID
}

Shader& graphics::Shader::operator=(Shader&& other) noexcept{
    if (this != &other) {
        glDeleteProgram(m_program);   // delete existing one
        m_shaders = std::move(other.m_shaders);
        m_program = other.m_program;
        other.m_program = 0;          // invalidate
    }
    return *this;
}

Shader::~Shader() {
    glDeleteProgram(m_program);
}

void Shader::Use() const {
    glUseProgram(m_program);
}

void Shader::Unbind() const {
    glUseProgram(0);
}

std::string Shader::GetShaderFileContents(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return "";
    }

    std::stringstream ss;
    ss << file.rdbuf();

    if (!ss.good()) {
        file.close();
        throw std::runtime_error("Failed to read shader: "+filename);
    }
    file.close();
    return ss.str();
}

GLuint Shader::CreateShader(const std::string& source, unsigned int shaderType) {
    GLuint shader = glCreateShader(shaderType);

    const char* shaderCode = source.c_str();
    glShaderSource(shader, 1, &shaderCode, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader::CreateShader::glGetShaderiv() failed\n" << infoLog << std::endl;
    }
    return shader;
}

void Shader::SetFloat(const std::string& name, float value) const{
    glUniform1f(glGetUniformLocation(m_program, name.c_str()), value);
}

void Shader::SetVec3(const std::string& name, glm::vec3 value) const{
    glUniform3fv(glGetUniformLocation(m_program, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetMat4(const std::string& name, glm::mat4 value) const{
    glUniformMatrix4fv(glGetUniformLocation(m_program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}