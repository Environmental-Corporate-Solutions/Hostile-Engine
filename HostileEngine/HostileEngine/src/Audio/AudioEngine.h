//------------------------------------------------------------------------------
//
// File Name:	AudioEngine.h
// Author(s):	Kiara Santiago
//						
//
// Copyright ?2023 - 2024 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include <AL/alc.h>
#include "WavLoader.h"

namespace Hostile
{
    class AudioEngine
    {
    public:
        AudioEngine(const AudioEngine& _audioObj) = delete;
        static AudioEngine& GetInstance();
        AudioEngine() {}

        void Init();
        void Update();
        void Shutdown();

        

    private:
        ALCdevice* m_device;
        ALCcontext* m_context;
        WavObject m_testWav;


        
        void CheckContextError(ALCdevice* _device, const std::int_fast32_t _line);
        void CheckNormalError(const std::int_fast32_t _line);


    };
}