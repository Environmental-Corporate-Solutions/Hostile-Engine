#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <GLFW/glfw3.h>

#include "GraphicsHelpers.h"

#include "Camera.h"

#include <directxtk12/SimpleMath.h>
#include <directxtk12/Effects.h>

#include <memory>

#include "IRenderTarget.h"

#include "IReadBackBuffer.h"

#include "GraphicsTypes.h"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace Hostile
{
    //TODO: Give meshes instance counts, at beginning of frame
    // allocate based on instance count
    class IGraphics
    {
    public:
        virtual ~IGraphics() = default;

        virtual bool Init(GLFWwindow* _pWindow) = 0;

        virtual void SetLight(UINT _light, bool _active) = 0;
        virtual void SetLight(
            UINT _light, const Vector3& _position, const Vector3& _color) = 0;

        virtual void SetCamera(
            const Vector3& _position,
            const Matrix& _matrix
        ) = 0;

        virtual void Draw(DrawCall& _draw_call) = 0;

        const size_t MAX_RENDER_TARGETS = 4;

        virtual std::shared_ptr<IRenderTarget> 
            CreateRenderTarget(UINT _i = 0) = 0;
        virtual std::shared_ptr<DepthTarget> CreateDepthTarget() = 0;
        virtual IReadBackBufferPtr CreateReadBackBuffer(
            IRenderTargetPtr& _render_target) = 0;

        virtual void BeginFrame() = 0;

        virtual void EndFrame() = 0;
        virtual void RenderImGui() = 0;

        virtual void OnResize(UINT _width, UINT _height) = 0;;
        virtual void Update() = 0;

        virtual void Shutdown() = 0;

        static IGraphics& Get();
    };
}