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

static  Hostile::Camera* _gcamdata;

Vector3 SceneCamera::GetPosition() const
{
    return _gcamdata->m_view_info.m_position;
}

void SceneCamera::SetPosition(float _x, float _y, float _z)
{
    _gcamdata->m_view_info.m_position = { _x, _y, _z };
}

void SceneCamera::SetPosition(const Vector3& _pos)
{
    _gcamdata->m_view_info.m_position = _pos;
}

Vector3 SceneCamera::GetRight() const
{
    return _gcamdata->m_view_info.m_right;
}

Vector3 SceneCamera::GetUp() const
{
    return _gcamdata->m_view_info.m_up;
}

Vector3 SceneCamera::GetForward() const
{
    return _gcamdata->m_view_info.m_forward;
}

void SceneCamera::Update() const
{
    Hostile::CameraSys::UpdateView(*_gcamdata);
}



void SceneCamera::Pitch(float _degree)
{
    _gcamdata->m_view_info.m_forward = Vector3::TransformNormal(_gcamdata->m_view_info.m_forward, Matrix::CreateFromAxisAngle(_gcamdata->m_view_info.m_right, XMConvertToRadians(_degree)));

}

void SceneCamera::Yaw(float _degree)
{
    _gcamdata->m_view_info.m_forward = Vector3::TransformNormal(_gcamdata->m_view_info.m_forward, Matrix::CreateFromAxisAngle(_gcamdata->m_view_info.m_up, XMConvertToRadians(_degree)));


}

void SceneCamera::MoveForward(float _speed)
{
    Vector3 f = _gcamdata->m_view_info.m_forward * _speed;
    _gcamdata->m_view_info.m_position = Vector3::Transform(_gcamdata->m_view_info.m_position, Matrix::CreateTranslation(f));


}

void SceneCamera::MoveRight(float _speed)
{
    _gcamdata->m_view_info.m_position = Vector3::Transform(_gcamdata->m_view_info.m_position, Matrix::CreateTranslation(_speed * _gcamdata->m_view_info.m_right));

}

void SceneCamera::MoveUp(float _speed)
{
    _gcamdata->m_view_info.m_position = Vector3::Transform(_gcamdata->m_view_info.m_position, Matrix::CreateTranslation(_speed * _gcamdata->m_view_info.m_up));
   
}

Vector2 SceneCamera::GetFarNear() const
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
void SceneCamera::SetPerspective(float _fovY, float _aspectRatio, float _near, float _far)
{
    _gcamdata->m_projection_matrix = XMMatrixPerspectiveFovRH(_fovY, _aspectRatio, _near, _far);
    _gcamdata->m_projection_info.m_fovY = _fovY;
    _gcamdata->m_projection_info.m_aspectRatio = _aspectRatio;
    _gcamdata->m_projection_info.m_near = _near;
    _gcamdata->m_projection_info.m_far = _far;
}

void SceneCamera::LookAt(Vector3 _eyePos, Vector3 _focusPos, Vector3 _globalUp)
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

void SceneCamera::LookTo(Vector3 _eyePos, Vector3 _lookDirection, Vector3 _relativeUp)
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

Matrix SceneCamera::View() const
{
    return _gcamdata->m_view_matrix;
}

Matrix SceneCamera::Projection() const
{
    return _gcamdata->m_projection_matrix;
}

Matrix SceneCamera::ViewProjection() const
{
    return _gcamdata->m_view_matrix * _gcamdata->m_projection_matrix;
}

void SceneCamera::SetDefaultID(_In_ int _id)
{
    m_default_camera_id = _id;
}

int SceneCamera::GetDefaultID()
{
	return m_default_camera_id;
}

void SceneCamera::ChangeCamera(int _camID)
{
    const flecs::entity& _camera_entity = Hostile::IEngine::Get().GetWorld()
        .entity(_camID);
   _gcamdata = _camera_entity.get_mut<Hostile::Camera>();
    
}

void SceneCamera::ChangeCamera(std::string _camName)
{
	const flecs::entity& _camera_entity = Hostile::IEngine::Get().GetWorld()
        .entity(_camName.c_str());
	_gcamdata = _camera_entity.get_mut<Hostile::Camera>();
    
}

void SceneCamera::ChangeCamera(Hostile::Camera* _In_ _cam_data)
{
  _gcamdata = _cam_data;
}