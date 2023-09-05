#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include <iostream>

void ErrorCallback(int _error, const char* _desc)
{
    std::cout << "Error: " << _error << "\n" << _desc << std::endl;
}

void KeyCallback(GLFWwindow* _pWindow, int _key, int _scancode, int _action, int _mods)
{
    if (_key == GLFW_KEY_ESCAPE && _action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(_pWindow, true);
    }
}

int main()
{
    if (!glfwInit())
        return -1;

    AllocConsole();
    FILE* pFile;
    freopen_s(&pFile, "CONIN$", "r", stdin);
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    freopen_s(&pFile, "CONOUT$", "w", stderr);

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "D3DTest", NULL, NULL);
    if (!window)
    {
        return -1;
    }

    HWND hwnd = glfwGetWin32Window(window);

    //Graphics graphics;
    //graphics.Init(hwnd);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    FreeConsole();
    glfwDestroyWindow(window);
    glfwTerminate();
}