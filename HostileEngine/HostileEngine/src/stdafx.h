#pragma once

#pragma warning(push)
#pragma warning(disable :4001)
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include <algorithm>
#include <memory>
#include <assert.h>
#include <Windows.h>
#include <directxtk/SimpleMath.h>
#include <functional>
#include <typeindex>
#include <any>
#include <queue>
#include <thread>
#include <wrl/client.h>

#include <spdlog/spdlog.h>
#include "src/Log.h"

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;
#define UNREFERENCED_PARAMATER(P) (P)

#pragma warning(pop)