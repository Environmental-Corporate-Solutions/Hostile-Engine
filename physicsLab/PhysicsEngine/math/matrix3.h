#pragma once

#include <array>
#include <glm.hpp>
#include "DirectXTK/SimpleMath.h"
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;


namespace math
{
    struct Matrix3
    {
        std::array<std::array<float, 3>, 3> m_entries;

        Matrix3(float diagonal=1.f); 
        Matrix3(float diagonal1, float diagonal2, float diagonal3);
        Matrix3(float e1, float e2, float e3, float e4, float e5, float e6, float e7, float e8, float e9);

        Matrix3 Transpose() const;
        Matrix3 Inverse() const;

        Matrix3 operator+(const Matrix3& other) const;
        Matrix3& operator+=(const Matrix3& other);

        Matrix3 operator-(const Matrix3& other) const;
        Matrix3& operator-=(const Matrix3& other);

        Matrix3 operator*(const Matrix3& other) const;
        Matrix3& operator*=(const Matrix3& other);

        Vector3 operator*(const Vector3& vec) const;

        Matrix3 operator*(const float value) const;
        Matrix3& operator*=(const float value);

        Matrix3& operator=(const Matrix3& other);

        bool operator==(const Matrix3& other)const;
        bool operator!=(const Matrix3& other)const;

        float operator[](int idx) const;
        float& operator[](int idx);

        void SetDiagonal(float value);
    };
}