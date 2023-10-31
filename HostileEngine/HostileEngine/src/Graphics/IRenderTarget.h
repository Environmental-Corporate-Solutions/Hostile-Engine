#pragma once
#include <directxtk12/SimpleMath.h>
#include <memory>
#include "IReadBackBuffer.h"

namespace Hostile
{
    class IRenderTarget
    {
    public:
        virtual ~IRenderTarget() = default;

        virtual void SetView(DirectX::SimpleMath::Matrix const& _view) = 0;
        virtual void SetProjection(DirectX::SimpleMath::Matrix const& _projection) = 0;
        virtual void SetCameraPosition(DirectX::SimpleMath::Vector3 const& _cameraPosition) = 0;
        virtual DirectX::SimpleMath::Matrix GetView() const = 0;
        virtual DirectX::SimpleMath::Matrix GetProjection() const = 0;
        virtual DirectX::SimpleMath::Vector3 GetCameraPosition() const = 0;

        virtual DirectX::SimpleMath::Vector2 GetDimensions() = 0;
        virtual UINT64 GetPtr() = 0;

        virtual void BindReadBackBuffer(IReadBackBufferPtr _readback_buffer) = 0;
    };
    using IRenderTargetPtr = std::shared_ptr<IRenderTarget>;
}