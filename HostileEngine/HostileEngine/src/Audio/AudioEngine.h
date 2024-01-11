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
#include <AL/al.h>
#include "WavLoader.h"

namespace Hostile
{
    struct AlWavObj
    {
        std::string m_name;
        ALuint m_buffer;
    };

    class AudioEngine
    {
    public:
        AudioEngine(const AudioEngine& _audioObj) = delete;
        static AudioEngine& GetInstance();
        AudioEngine() {}

        void Init();
        void Update();
        void Shutdown();

        void LoadManyWav(std::vector<std::string> _names);
        void UnloadManyWav();

    private:
        ALCdevice* m_device;
        ALCcontext* m_context;
        ALuint m_source;
        std::vector<AlWavObj> m_audioObjects;

        
        void CheckContextError(ALCdevice* _device, const std::int_fast32_t _line);
        void CheckNormalError(const std::int_fast32_t _line);
        ALenum GetFormat(WavObject _wav);


    };
}