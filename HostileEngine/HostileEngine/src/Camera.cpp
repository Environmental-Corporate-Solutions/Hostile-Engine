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

static  Hostile::CameraData* _gcamdata;

/**
 * GetPosition()
 * @brief returns the position of the camera tied to player.
 * 
 * \return 
 */
Vector3 Camera::GetPosition() const
{
    return _gcamdata->m_view_info.m_position;
}


/**
 * SetPosition(float x,y,z).
 * @brief overrides and sets position of the camera for world/scene
 *        cameras not attached to a player or moveable entity.
 * 
 * \param _x
 * \param _y
 * \param _z
 */
void Camera::SetPosition(float _x, float _y, float _z)
{
    _gcamdata->m_view_info.m_position = { _x, _y, _z };
}

void Camera::SetPosition(const Vector3& _pos)
{
    _gcamdata->m_view_info.m_position = _pos;
}

Vector3 Camera::GetRight() const
{
    return _gcamdata->m_view_info.m_right;
}

Vector3 Camera::GetUp() const
{
    return _gcamdata->m_view_info.m_up;
}

Vector3 Camera::GetForward() const
{
    return _gcamdata->m_view_info.m_forward;
}

void Camera::Update() const
{
    Hostile::CameraSys::UpdateView(*_gcamdata);
}



void Camera::Pitch(float _degree)
{
    _gcamdata->m_view_info.m_forward = Vector3::TransformNormal(_gcamdata->m_view_info.m_forward, Matrix::CreateFromAxisAngle(_gcamdata->m_view_info.m_right, XMConvertToRadians(_degree)));

}

void Camera::Yaw(float _degree)
{
    _gcamdata->m_view_info.m_forward = Vector3::TransformNormal(_gcamdata->m_view_info.m_forward, Matrix::CreateFromAxisAngle(_gcamdata->m_view_info.m_up, XMConvertToRadians(_degree)));


}

void Camera::MoveForward(float _speed)
{
    Vector3 f = _gcamdata->m_view_info.m_forward * _speed;
    _gcamdata->m_view_info.m_position = Vector3::Transform(_gcamdata->m_view_info.m_position, Matrix::CreateTranslation(f));


}

void Camera::MoveRight(float _speed)
{
    _gcamdata->m_view_info.m_position = Vector3::Transform(_gcamdata->m_view_info.m_position, Matrix::CreateTranslation(_speed * _gcamdata->m_view_info.m_right));

}

void Camera::MoveUp(float _speed)
{
    _gcamdata->m_view_info.m_position = Vector3::Transform(_gcamdata->m_view_info.m_position, Matrix::CreateTranslation(_speed * _gcamdata->m_view_info.m_up));
   
}

Vector2 Camera::GetFarNear() const
{
    return { _gcamdata->m_projection_info.m_far, _gcamdata->m_projection_info.m_near };
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
    _gcamdata->m_projection_matrix = XMMatrixPerspectiveFovRH(_fovY, _aspectRatio, _near, _far);
    _gcamdata->m_projection_info.m_fovY = _fovY;
    _gcamdata->m_projection_info.m_aspectRatio = _aspectRatio;
    _gcamdata->m_projection_info.m_near = _near;
    _gcamdata->m_projection_info.m_far = _far;
}

void Camera::LookAt(Vector3 _eyePos, Vector3 _focusPos, Vector3 _globalUp)
{
    _gcamdata->m_view_info.m_position = _eyePos;
    _gcamdata->m_view_info.m_forward = _focusPos - _eyePos;
    _gcamdata->m_view_info.m_forward.Normalize();
    _gcamdata->m_view_info.m_right = _globalUp.Cross(_gcamdata->m_view_info.m_forward);
    _gcamdata->m_view_info.m_right.Normalize();
    _gcamdata->m_view_info.m_up = _gcamdata->m_view_info.m_forward.Cross(_gcamdata->m_view_info.m_right);
    _gcamdata->m_view_info.m_up.Normalize();
    
    _gcamdata->m_view_matrix= XMMatrixLookAtRH(_eyePos, _focusPos, _globalUp);
}

void Camera::LookTo(Vector3 _eyePos, Vector3 _lookDirection, Vector3 _relativeUp)
{
    _gcamdata->m_view_info.m_position = _eyePos;
    _gcamdata->m_view_info.m_forward = _lookDirection;
    _gcamdata->m_view_info.m_forward.Normalize();
    _gcamdata->m_view_info.m_right = _gcamdata->m_view_info.m_forward.Cross(_relativeUp);
    _gcamdata->m_view_info.m_right.Normalize();
    _gcamdata->m_view_info.m_up = _relativeUp;
    _gcamdata->m_view_info.m_up.Normalize();
    _gcamdata->m_view_matrix= XMMatrixLookToRH(_eyePos, _lookDirection, _relativeUp);
}

Matrix Camera::View() const
{
    return _gcamdata->m_view_matrix;
}

Matrix Camera::Projection() const
{
    return _gcamdata->m_projection_matrix;
}

Matrix Camera::ViewProjection() const
{
    return _gcamdata->m_view_matrix * _gcamdata->m_projection_matrix;
}

void Camera::SetDefaultID(_In_ int _id)
{
    m_default_camera_id = _id;
}

int Camera::GetDefaultID()
{
	return m_default_camera_id;
}

void Camera::ChangeCamera(int _camID)
{
    const flecs::entity& _camera_entity = Hostile::IEngine::Get().GetWorld()
        .entity(_camID);
   _gcamdata = _camera_entity.get_mut<Hostile::CameraData>();
    
}

void Camera::ChangeCamera(std::string _camName)
{
	const flecs::entity& _camera_entity = Hostile::IEngine::Get().GetWorld()
        .entity(_camName.c_str());
	_gcamdata = _camera_entity.get_mut<Hostile::CameraData>();
    
}

void Camera::ChangeCamera(Hostile::CameraData* _In_ _cam_data)
{
  _gcamdata = _cam_data;
}