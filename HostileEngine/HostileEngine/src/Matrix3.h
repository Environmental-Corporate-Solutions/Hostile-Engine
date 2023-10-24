//------------------------------------------------------------------------------
//
// File Name:	Matrix3.h
// Author(s):	byeonggyu.park
// Description: SimpleMath doesn't provide a 3x3 matrix, 
//              so this header file includes a temporary declaration for it
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include <array>
#include "directxtk12/SimpleMath.h"
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;

namespace Hostile
{
    struct Matrix3
    {
        std::array<std::array<float, 3>, 3> m_entries;

        Matrix3(float diagonal = 1.f);
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
    static Matrix3 Extract3x3Matrix(const Quaternion& ori) {
        Matrix3 rotationMatrix_3x3;
        Matrix mat= XMMatrixRotationQuaternion(ori);
        for (int col = 0; col < 3; ++col) {//Extract3X3
            for (int row = 0; row < 3; ++row) {
                rotationMatrix_3x3[row * 3 + col] = mat.m[row][col];
            }
        }

        return rotationMatrix_3x3;
    }
}