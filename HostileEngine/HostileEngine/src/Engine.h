//------------------------------------------------------------------------------
//
// File Name:	Engine.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "ISystemPtr.h"
#include "flecs.h"
#define ADD_SYSTEM(x)                         \
    class x##Adder                            \
    {                                         \
        x##Adder()                            \
        {                                     \
            IEngine::Get().Add(new x);        \
        }                                     \
    }                                         \
    static x##Adder x##adder;                 \


namespace Hostile
{
  class IEngine
  {
  public: 
    static IEngine& Get();
    virtual void Add(ISystemPtr _pSys) = 0;
    virtual void Init() = 0;
    virtual flecs::world& GetWorld() = 0;
  private:
    
  };
}