/* Start Header -------------------------------------------------------
Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior written
consent of DigiPen Institute of Technology is prohibited.
File Name: Camera.cpp
Purpose: Implementation for the Camera class
Language: C++ MSVC
Platform: MSVC v140, Windows 11
Project: s.biks_CS300_3
Author: Sam Biks, s.biks, 0049483
Creation date: 11/21/2022
End Header --------------------------------------------------------*/
#include "stdafx.h"
#include "Camera.h"
#include <iostream>

Camera::Camera()
    : m_pos(Vector3{ 0, 0, 0 }), m_up(Vector3{ 0, 1, 0 }), m_forward(Vector3{ 0, 0, 1 }),
    m_right(Vector3{ 1, 0, 0 }), m_view(), m_near(0),
    m_far(0), m_aspectRatio(0), m_fovY(0),
    m_projection(), m_dirty(false)
{
}

Camera::~Camera()
{

}

Vector3 Camera::GetPosition() const
{
    return m_pos;
}

void Camera::SetPosition(float _x, float _y, float _z)
{
    m_pos = { _x, _y, _z };
}

void Camera::SetPosition(const Vector3& _pos)
{
    m_pos = _pos;
}

Vector3 Camera::GetRight() const
{
    return m_right;
}

Vector3 Camera::GetUp() const
{
    return m_up;
}

Vector3 Camera::GetForward() const
{
    return m_forward;
}

void Camera::Update()
{
    //std::cout << "F: " << m_forward.x << " " << m_forward.y << " " << m_forward.z << std::endl;
    Vector3 globalUp = { 0, 1, 0 };
    if (m_forward != Vector3{ 0, 1, 0 } && m_forward != Vector3{ 0, -1, 0 })
    {
        m_right = globalUp.Cross(m_forward);
        m_right.Normalize();

        m_up = m_forward.Cross(m_right);
        m_up.Normalize();
    }
    else
    {
        m_up = m_forward.Cross({ 1, 0, 0 });
        m_up.Normalize();

        m_right = m_up.Cross(m_forward);
        m_right.Normalize();
    }

    m_view = XMMatrixLookToRH(m_pos, m_forward, m_up);
}

void Camera::Pitch(float _degree)
{
    m_forward = Vector3::TransformNormal(m_forward, Matrix::CreateFromAxisAngle(m_right, XMConvertToRadians(_degree)));

    Update();
}

void Camera::Yaw(float _degree)
{
    m_forward = Vector3::TransformNormal(m_forward, Matrix::CreateFromAxisAngle(m_up, XMConvertToRadians(_degree)));

    Update();
}

void Camera::MoveForward(float _speed)
{
    Vector3 f = m_forward * _speed;
    m_pos = Vector3::Transform(m_pos, Matrix::CreateTranslation(f));

    Update();
}

void Camera::MoveRight(float _speed)
{
    m_pos = Vector3::Transform(m_pos, Matrix::CreateTranslation(_speed * m_right));
    Update();
}

void Camera::MoveUp(float _speed)
{
    m_pos = Vector3::Transform(m_pos, Matrix::CreateTranslation(_speed * m_up));
    Update();
}

Vector2 Camera::GetFarNear() const
{
    return { m_far, m_near };
}

void Camera::SetPerspective(float _fovY, float _aspectRatio, float _near, float _far)
{
    m_projection = XMMatrixPerspectiveFovRH(_fovY, _aspectRatio, _near, _far);
    m_fovY = _fovY;
    m_aspectRatio = _aspectRatio;
    m_near = _near;
    m_far = _far;
}

void Camera::LookAt(Vector3 _eyePos, Vector3 _focusPos, Vector3 _globalUp)
{
    m_pos = _eyePos;
    m_forward = _focusPos - _eyePos;
    m_forward.Normalize();
    m_right = _globalUp.Cross(m_forward);
    m_right.Normalize();
    m_up = m_forward.Cross(m_right);
    m_up.Normalize();

    m_view = XMMatrixLookAtRH(_eyePos, _focusPos, _globalUp);
}

void Camera::LookTo(Vector3 _eyePos, Vector3 _lookDirection, Vector3 _relativeUp)
{
    m_pos = _eyePos;
    m_forward = _lookDirection;
    m_forward.Normalize();
    m_right = m_forward.Cross(_relativeUp);
    m_right.Normalize();
    m_up = _relativeUp;
    m_up.Normalize();
    m_view = XMMatrixLookToRH(_eyePos, _lookDirection, _relativeUp);
}

Matrix Camera::View()
{
    return m_view;
}

Matrix Camera::Projection()
{
    return m_projection;
}

Matrix Camera::ViewProjection()
{
    return m_view * m_projection;
}