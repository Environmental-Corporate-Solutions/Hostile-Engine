#pragma once

namespace Hostile
{
    class IReadBackBuffer
    {
    public:
        virtual bool Map(UINT8** _pdata) = 0;
    };
    using IReadBackBufferPtr = std::shared_ptr<IReadBackBuffer>;
}