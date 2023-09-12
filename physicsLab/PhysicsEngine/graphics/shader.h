#pragma once

#include "glm/gtc/type_ptr.hpp"
#include <opengl/glad/glad.h>
#include <string>
#include <vector>

namespace graphics
{
    class Shader {
    public:
        Shader() :m_program{} {}
        Shader(const std::vector<std::pair<std::string, GLenum>>& shaderFiles);
        Shader(Shader&& other) noexcept;             
        Shader& operator=(Shader&& other) noexcept;  
        ~Shader();

        void Use() const;
        void Unbind() const;

		void SetFloat(const std::string& name, float value) const;
		void SetVec3(const std::string& name, glm::vec3 value) const;
		void SetMat4(const std::string& name, glm::mat4 value) const;

    private:
        std::string GetShaderFileContents(const std::string& filename);
        GLuint CreateShader(const std::string& source, unsigned int shaderType);

        GLuint m_program;
        std::vector<GLuint> m_shaders;
    };
}