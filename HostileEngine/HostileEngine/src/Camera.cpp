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
#include "Engine.h"
#include "CameraComponent.h"
Vector3 Camera::GetPosition() const
{
    return m_camera_data->m_view_info.m_position;
}

void Camera::SetPosition(float _x, float _y, float _z)
{
    m_camera_data->m_view_info.m_position = { _x, _y, _z };
}

void Camera::SetPosition(const Vector3& _pos)
{
    m_camera_data->m_view_info.m_position = _pos;
}

Vector3 Camera::GetRight() const
{
    return m_camera_data->m_view_info.m_right;
}

Vector3 Camera::GetUp() const
{
    return m_camera_data->m_view_info.m_up;
}

Vector3 Camera::GetForward() const
{
    return m_camera_data->m_view_info.m_forward;
}

void Camera::Update() const
{
    Hostile::CameraSys::UpdateView(*m_camera_data);
}



void Camera::Pitch(float _degree)
{
    m_camera_data->m_view_info.m_forward = Vector3::TransformNormal(m_camera_data->m_view_info.m_forward, Matrix::CreateFromAxisAngle(m_camera_data->m_view_info.m_right, XMConvertToRadians(_degree)));

}

void Camera::Yaw(float _degree)
{
    m_camera_data->m_view_info.m_forward = Vector3::TransformNormal(m_camera_data->m_view_info.m_forward, Matrix::CreateFromAxisAngle(m_camera_data->m_view_info.m_up, XMConvertToRadians(_degree)));


}

void Camera::MoveForward(float _speed)
{
    Vector3 f = m_camera_data->m_view_info.m_forward * _speed;
    m_camera_data->m_view_info.m_position = Vector3::Transform(m_camera_data->m_view_info.m_position, Matrix::CreateTranslation(f));


}

void Camera::MoveRight(float _speed)
{
    m_camera_data->m_view_info.m_position = Vector3::Transform(m_camera_data->m_view_info.m_position, Matrix::CreateTranslation(_speed * m_camera_data->m_view_info.m_right));

}

void Camera::MoveUp(float _speed)
{
    m_camera_data->m_view_info.m_position = Vector3::Transform(m_camera_data->m_view_info.m_position, Matrix::CreateTranslation(_speed * m_camera_data->m_view_info.m_up));
   
}

Vector2 Camera::GetFarNear() const
{
    return { m_camera_data->m_projection_info.m_far, m_camera_data->m_projection_info.m_near };
}


/**
 * \brief 
 * \param _fovY 
 * \param _aspectRatio 
 * \param _near 
 * \param _far 
 */
void Camera::SetPerspective(float _fovY, float _aspectRatio, float _near, float _far)
{
    m_camera_data->m_projection_matrix = XMMatrixPerspectiveFovRH(_fovY, _aspectRatio, _near, _far);
    m_camera_data->m_projection_info.m_fovY = _fovY;
    m_camera_data->m_projection_info.m_aspectRatio = _aspectRatio;
    m_camera_data->m_projection_info.m_near = _near;
    m_camera_data->m_projection_info.m_far = _far;
}

void Camera::LookAt(Vector3 _eyePos, Vector3 _focusPos, Vector3 _globalUp)
{
    m_camera_data->m_view_info.m_position = _eyePos;
    m_camera_data->m_view_info.m_forward = _focusPos - _eyePos;
    m_camera_data->m_view_info.m_forward.Normalize();
    m_camera_data->m_view_info.m_right = _globalUp.Cross(m_camera_data->m_view_info.m_forward);
    m_camera_data->m_view_info.m_right.Normalize();
    m_camera_data->m_view_info.m_up = m_camera_data->m_view_info.m_forward.Cross(m_camera_data->m_view_info.m_right);
    m_camera_data->m_view_info.m_up.Normalize();

    m_camera_data->m_view_matrix= XMMatrixLookAtRH(_eyePos, _focusPos, _globalUp);
}

void Camera::LookTo(Vector3 _eyePos, Vector3 _lookDirection, Vector3 _relativeUp)
{
    m_camera_data->m_view_info.m_position = _eyePos;
    m_camera_data->m_view_info.m_forward = _lookDirection;
    m_camera_data->m_view_info.m_forward.Normalize();
    m_camera_data->m_view_info.m_right = m_camera_data->m_view_info.m_forward.Cross(_relativeUp);
    m_camera_data->m_view_info.m_right.Normalize();
    m_camera_data->m_view_info.m_up = _relativeUp;
    m_camera_data->m_view_info.m_up.Normalize();
    m_camera_data->m_view_matrix= XMMatrixLookToRH(_eyePos, _lookDirection, _relativeUp);
}

Matrix Camera::View() const
{
    return m_camera_data->m_view_matrix;
}

Matrix Camera::Projection() const
{
    return m_camera_data->m_projection_matrix;
}

Matrix Camera::ViewProjection() const
{
    return m_camera_data->m_view_matrix * m_camera_data->m_projection_matrix;
}

void Camera::ChangeCamera(int _camID)
{
	const flecs::entity& _camera_entity = Hostile::IEngine::Get().GetWorld().entity(_camID);
   m_camera_data = _camera_entity.get_mut<Hostile::CameraData>();
 

    
}