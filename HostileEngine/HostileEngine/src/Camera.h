/* Start Header -------------------------------------------------------
Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior written
consent of DigiPen Institute of Technology is prohibited.
File Name: Camera.h
Purpose: Header for the Camera class
Language: C++ MSVC
Platform: MSVC v140, Windows 11
Project: s.biks_CS300_3
Author: Sam Biks, s.biks, 0049483
Creation date: 11/21/2022
End Header --------------------------------------------------------*/
#pragma once
#include "directxtk12/SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

class Camera
{
public:
    Camera();
    ~Camera();

    Vector3 GetPosition() const;
    void SetPosition(float _x, float _y, float _z);
    void SetPosition(const Vector3& _pos);

    Vector3 GetRight() const;
    Vector3 GetUp() const;
    Vector3 GetForward() const;

    void Update();

    void Pitch(float _degree);
    void Yaw(float _degree);

    void MoveForward(float _speed);
    void MoveRight(float _speed);
    void MoveUp(float _speed);

    Vector2 GetFarNear() const;

    void SetPerspective(float _fovY, float _aspectRatio, float _near, float _far);

    void LookAt(Vector3 _eyePos, Vector3 _focusPos, Vector3 _globalUp);
    void LookTo(Vector3 _eyePos, Vector3 _lookDirection, Vector3 _relativeUp);

    Matrix View();
    Matrix Projection();

    Matrix ViewProjection();

private:

    Vector3 m_pos;
    Vector3 m_up; // relative up
    Vector3 m_forward; // relative z
    Vector3 m_right; // relative right

    Matrix m_view;

    float m_near;
    float m_far;
    float m_aspectRatio;
    float m_fovY;
    
    Matrix m_projection;

    float m_dirty;
};