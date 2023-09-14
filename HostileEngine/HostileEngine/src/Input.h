#pragma once
#include <tuple>
#include "InputKeyCodes.h"
struct GLFWwindow;

class Input
{
	//for Reset func
	friend int main(int [[maybe_unused]] argc, char** [[maybe_unused]] argv);
	//for callback
	friend void KeyCallback(GLFWwindow* _pWindow, int _key, int _scancode, int _action, int _mods);
public:
	Input() = delete;

	static bool IsPressed(Key key);
	static bool IsPressed(Mouse button);
	static bool IsTriggered(Key key);
	static bool IsTriggered(Mouse button);
	static bool IsRepeating(Key key);
	//window doesn't support repeating event for mouse input
	//static bool IsRepeating(Mouse button);
	static bool IsReleased(Key key);
	static bool IsReleased(Mouse button);

	static bool IsMouseScrolled();

	static std::tuple<float, float> GetMousePosition();
	static std::tuple<float, float> GetLastMousePosition();

	static std::tuple<float, float> GetMouseOffset();
	static float GetMouseScrollOffset();
private:
	//should be called before glfwPollEvents();
	static void Reset();

	//for glfw callback
	static void SetKey(Key key, bool state);
	static void SetMouseButton(Mouse button, bool state);
	static void SetMousePosition(float x, float y);
	static void SetMouseScroll(float yOffset);
};