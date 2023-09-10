#pragma once
namespace math {
	constexpr float PI = 3.141592f;
	constexpr float EPSILON = 1e-5f;

	static float DegreesToRadians(float degrees) {
		constexpr float coefficient = PI / 180.f;
		return degrees * coefficient;
	}
}
