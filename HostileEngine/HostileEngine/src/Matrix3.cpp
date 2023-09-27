//------------------------------------------------------------------------------
//
// File Name:	Matrix3.cpp
// Author(s):	byeonggyu.park
// Description: SimpleMath doesn't provide a 3x3 matrix, 
//              so this source file includes a temporary definition for it
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include <iostream>
#include "Matrix3.h"

namespace Hostile {

    constexpr float PI = 3.141592f;
    constexpr float EPSILON = 1e-5f;

    Matrix3::Matrix3(float diagonal) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                m_entries[j][i] = (i == j) ? diagonal : 0.0f;
            }
        }
    }

    Matrix3::Matrix3(float diagonal1, float diagonal2, float diagonal3) {
        m_entries[0][0] = diagonal1;
        m_entries[1][0] = 0.0f;
        m_entries[2][0] = 0.0f;

        m_entries[0][1] = 0.0f;
        m_entries[1][1] = diagonal2;
        m_entries[2][1] = 0.0f;

        m_entries[0][2] = 0.0f;
        m_entries[1][2] = 0.0f;
        m_entries[2][2] = diagonal3;
    }

    Matrix3::Matrix3(float e1, float e2, float e3, float e4, float e5, float e6, float e7, float e8, float e9) {
        m_entries[0][0] = e1;
        m_entries[1][0] = e2;
        m_entries[2][0] = e3;

        m_entries[0][1] = e4;
        m_entries[1][1] = e5;
        m_entries[2][1] = e6;

        m_entries[0][2] = e7;
        m_entries[1][2] = e8;
        m_entries[2][2] = e9;
    }

    Matrix3 Matrix3::Transpose() const {
        Matrix3 result;
        for (int i{}; i < 3; ++i) {
            for (int j{}; j < 3; ++j) {
                result.m_entries[i][j] = m_entries[j][i];
            }
        }
        return result;
    }

    Matrix3 Matrix3::Inverse() const
    {
        float determinant =
            m_entries[0][0] * (m_entries[1][1] * m_entries[2][2] - m_entries[1][2] * m_entries[2][1])
            - m_entries[0][1] * (m_entries[1][0] * m_entries[2][2] - m_entries[1][2] * m_entries[2][0])
            + m_entries[0][2] * (m_entries[1][0] * m_entries[2][1] - m_entries[1][1] * m_entries[2][0]);

        if (fabs(determinant) < EPSILON) {
            throw std::runtime_error("Matrix3::inverse()::singular matrix");
        }

        determinant = 1.0f / determinant;

        Matrix3 result;

        result.m_entries[0][0] = determinant * (m_entries[1][1] * m_entries[2][2] - m_entries[1][2] * m_entries[2][1]);
        result.m_entries[1][0] = determinant * (m_entries[1][2] * m_entries[2][0] - m_entries[1][0] * m_entries[2][2]);
        result.m_entries[2][0] = determinant * (m_entries[1][0] * m_entries[2][1] - m_entries[1][1] * m_entries[2][0]);
        result.m_entries[0][1] = determinant * (m_entries[0][2] * m_entries[2][1] - m_entries[0][1] * m_entries[2][2]);
        result.m_entries[1][1] = determinant * (m_entries[0][0] * m_entries[2][2] - m_entries[0][2] * m_entries[2][0]);
        result.m_entries[2][1] = determinant * (m_entries[0][1] * m_entries[2][0] - m_entries[0][0] * m_entries[2][1]);
        result.m_entries[0][2] = determinant * (m_entries[0][1] * m_entries[1][2] - m_entries[0][2] * m_entries[1][1]);
        result.m_entries[1][2] = determinant * (m_entries[0][2] * m_entries[1][0] - m_entries[0][0] * m_entries[1][2]);
        result.m_entries[2][2] = determinant * (m_entries[0][0] * m_entries[1][1] - m_entries[0][1] * m_entries[1][0]);

        return result;
    }


    Matrix3 Matrix3::operator+(const Matrix3& other) const {
        Matrix3 result;

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result.m_entries[j][i] = m_entries[j][i] + other.m_entries[j][i];
            }
        }
        return result;
    }

    Matrix3& Matrix3::operator+=(const Matrix3& other) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                m_entries[j][i] += other.m_entries[j][i];
            }
        }
        return *this;
    }

    Matrix3 Matrix3::operator-(const Matrix3& other) const {
        Matrix3 result;

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result.m_entries[j][i] = m_entries[j][i] - other.m_entries[j][i];
            }
        }

        return result;
    }

    Matrix3& Matrix3::operator-=(const Matrix3& other) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                m_entries[j][i] -= other.m_entries[j][i];
            }
        }
        return *this;
    }

    Matrix3 Matrix3::operator*(const Matrix3& other) const {
        Matrix3 result;

        //zero out
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result.m_entries[j][i] = 0.f;
            }
        }

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                for (int k = 0; k < 3; ++k) {
                    result.m_entries[j][i] += m_entries[k][i] * other.m_entries[j][k];
                }
            }
        }
        return result;
    }

    Matrix3& Matrix3::operator*=(const Matrix3& other) {
        Matrix3 result;

        //zero out
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result.m_entries[j][i] = 0.0f;
            }
        }

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                for (int k = 0; k < 3; ++k) {
                    result.m_entries[j][i] += this->m_entries[k][i] * other.m_entries[j][k];
                }
            }
        }

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                m_entries[j][i] = result.m_entries[j][i];
            }
        }

        return *this;
    }

    Vector3 Matrix3::operator*(const Vector3& vec) const {
        return DirectX::SimpleMath::Vector3{
            operator[](0) * vec.x + operator[](3) * vec.y + operator[](6) * vec.z,
            operator[](1) * vec.x + operator[](4) * vec.y + operator[](7) * vec.z,
            operator[](2) * vec.x + operator[](5) * vec.y + operator[](8) * vec.z
        };

    }

    Matrix3 Matrix3::operator*(const float value) const
    {
        Matrix3 result;

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result.m_entries[j][i] *= m_entries[j][i] * value;
            }
        }
        return result;
    }

    Matrix3& Matrix3::operator*=(const float value) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                m_entries[j][i] *= value;
            }
        }
        return *this;
    }

    Matrix3& Matrix3::operator=(const Matrix3& other)
    {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                m_entries[j][i] = other.m_entries[j][i];
            }
        }
        return *this;
    }

    bool Matrix3::operator==(const Matrix3& other) const {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (m_entries[j][i] != other.m_entries[j][i]) {
                    return false;
                }
            }
        }
        return true;
    }

    bool Matrix3::operator!=(const Matrix3& other) const {
        return !(*this == other);
    }

    float Matrix3::operator[](int idx) const {
        if (idx < 0 || idx > 8) {
            throw std::out_of_range("Index out of bounds for Matrix3");
        }
        return m_entries[idx % 3][idx / 3];
    }

    float& Matrix3::operator[](int idx) {
        if (idx < 0 || idx > 8) {
            throw std::out_of_range("Index out of bounds for Matrix3");
        }
        return m_entries[idx % 3][idx / 3];
    }

    void Matrix3::SetDiagonal(float value) {
        m_entries[0][0] = m_entries[1][1] = m_entries[2][2] = value;
    }
}