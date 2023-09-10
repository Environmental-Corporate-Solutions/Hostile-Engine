#pragma once
#include "matrix3.h"
#include <glm.hpp>
namespace math {

    inline math::Matrix3 Extract3x3(const Matrix& mat) {
        math::Matrix3 rotationMatrix_3x3;
        for (int col = 0; col < 3; ++col) {
            for (int row = 0; row < 3; ++row) {
                rotationMatrix_3x3[row * 3 + col] = mat.m[row][col];
            }
        }
        return rotationMatrix_3x3;
    }
    inline glm::mat4 ConvertToGLM(const Matrix& mat) {
        glm::mat4 result=glm::mat4(1.0);
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result[row][col] = mat.m[row][col];
            }
        }
        return result;
    }
}