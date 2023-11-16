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
        virtual DirectX::SimpleMath::Vector2 GetDimensions() = 0;
        virtual void SetDimensions(const Vector2& _dimensions) = 0;
        virtual UINT64 GetPtr() = 0;

        virtual void BindReadBackBuffer(IReadBackBufferPtr _readback_buffer) = 0;
    };
    using IRenderTargetPtr = std::shared_ptr<IRenderTarget>;
}