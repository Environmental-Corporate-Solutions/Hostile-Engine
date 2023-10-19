#pragma once

#pragma warning(push)
#pragma warning(disable :4001)
#define WIN32_LEAN_AND_MEAN
#include "Tracy.hpp"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#define PI 3.14159265f
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/attrdefs.h>

#include <algorithm>
#include <memory>
#include <assert.h>
#include <Windows.h>
#include <directxtk12/SimpleMath.h>
#include <functional>
#include <typeindex>
#include <any>
#include <queue>
#include <thread>
#include <wrl/client.h>
#include <filesystem>
#include <map>
#include <unordered_map>
#include <sstream>

#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imnodes.h>
#include "src/Log.h"
#include "Utility.h"
#include "Gui.h"
#include "Serializer.h"
#include "Deseralizer.h"

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;
#define UNREFERENCED_PARAMATER(P) (P)

#pragma warning(pop)