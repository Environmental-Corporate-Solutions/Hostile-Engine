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

namespace Hostile
{
	struct CameraData;
}

using namespace DirectX;
using namespace DirectX::SimpleMath;
typedef struct CameraData CameraData;

class Camera
{
public:
		Camera() = default;
		~Camera() = default;

		Vector3 GetPosition() const;
		void SetPosition(float _x, float _y, float _z);
		void SetPosition(const Vector3& _pos);

		Vector3 GetRight() const;
		Vector3 GetUp() const;
		Vector3 GetForward() const;

		void Update() const;

		void Pitch(float _degree);
		void Yaw(float _degree);

		void MoveForward(float _speed);
		void MoveRight(float _speed);
		void MoveUp(float _speed);

		Vector2 GetFarNear() const;

		void SetPerspective(float _fovY, float _aspectRatio, float _near, float _far);

		void LookAt(Vector3 _eyePos, Vector3 _focusPos, Vector3 _globalUp);
		void LookTo(Vector3 _eyePos, Vector3 _lookDirection, Vector3 _relativeUp);

		Matrix View() const;
		Matrix Projection() const;

		Matrix ViewProjection() const;
		void SetDefaultID(int _id);
		int GetDefaultID();
		static void ChangeCamera(int _camID);
		static void ChangeCamera(std::string _camName);

private:

		Vector3 m_pos{ 0, 0, 0 };
		Vector3 m_up{ 0, 1, 0 }; // relative up
		Vector3 m_forward{ 0, 0, -1 }; // relative z
		Vector3 m_right{ 1, 0, 0 }; // relative right

		Matrix m_view{};

		float m_near = 0;
		float m_far = 0;
		float m_aspectRatio = 0;
		float m_fovY = 0;
		
		Matrix m_projection{};
		Hostile::CameraData * m_camera_data;
		float m_dirty = false;
		int m_default_camera_id;
};