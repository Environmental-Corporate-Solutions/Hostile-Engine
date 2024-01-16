//-----------------------------------------------------------------------------
//
// File Name:	AudioEngine.cpp
// Author(s):	Kiara Santiago
// Description: Implements OpenAL Soft for audio stuffs
//
// Copyright ?2023 - 2024 DigiPen (USA) Corporation.
//
//-----------------------------------------------------------------------------
#pragma once
#include "stdafx.h"
#include "AudioEngine.h"

#include <AL/al.h>
#include <AL/alext.h>
#include <iostream>

namespace Hostile
{
    //here for temp
    ALuint buffer;
    ALenum format;
    ALuint source;

    //if instance of audio engine doesnt already exist, make one
    AudioEngine& AudioEngine::GetInstance()
    {
        static AudioEngine audio_engine;

        return audio_engine;
    }

    //where we set up our openAL stuff
    //create a context, the initial buffers, load the buffers
    //create sources, attach buffers to sources
    //set location and directions for the listeners and sources
    //set initial values for state global to openAL
    void AudioEngine::Init()
    {
        //find the default audio output device
        //it is possible to choose a different device but we cant do that rn
        m_device = alcOpenDevice(nullptr);
        if (!m_device)
        {
            std::cout << "ERROR: no audio device.\n";
        }

        //create a context for our device, we should only need 1
        //each context has its own listeners + sources
        //these cant be passed bt contexts
        m_context = alcCreateContext(m_device, nullptr);
        if (!m_context)
        {
            std::cout << "ERROR: cant make audio context for device.\n";
        }

        //make the context CURRENT
        //allows us to actually use it later
        ALCboolean current = alcMakeContextCurrent(m_context);
        if (!current)
        {
            std::cout << "ERROR: cant make audio context current.\n";
        }


        ////testing playing a sound --- load the wav
        m_testWav.LoadWav("./Assets/audio/ethanTest1.wav");


        
        alGenBuffers(1, &buffer);

        CheckNormalError(__LINE__);
        

        if (m_testWav.IsMono())
        {
            if (m_testWav.GetBitDepth() == 8)
            {
                format = AL_FORMAT_MONO8;
            }
            else if (m_testWav.GetBitDepth() == 16)
            {
                format = AL_FORMAT_MONO16;
            }
        }
        else if(m_testWav.IsStereo())
        {
            if (m_testWav.GetBitDepth() == 8)
            {
                format = AL_FORMAT_STEREO8;
            }
            else if (m_testWav.GetBitDepth() == 16)
            {
                format = AL_FORMAT_STEREO16;
            }
        }
        else
        {
            std::cout << "ERROR: wav format error\n";
        }

        alBufferData(buffer, format, m_testWav.m_sampleData.data(),
            m_testWav.m_sampleData.size(), m_testWav.GetSamplingRate());

        m_testWav.ClearData();

        CheckNormalError(__LINE__);
        

        alGenSources(1, &source);
        CheckNormalError(__LINE__);
        alSourcef(source, AL_PITCH, 1);
        CheckNormalError(__LINE__);
        alSourcef(source, AL_GAIN, 1.0f);
        CheckNormalError(__LINE__);
        alSource3f(source, AL_POSITION, 0, 0, 0);
        CheckNormalError(__LINE__);
        alSource3f(source, AL_VELOCITY, 0, 0, 0);
        CheckNormalError(__LINE__);
        alSourcei(source, AL_LOOPING, AL_FALSE);
        CheckNormalError(__LINE__);
        alSourcei(source, AL_BUFFER, buffer);
        CheckNormalError(__LINE__);

        alSourcePlay(source);
        CheckNormalError(__LINE__);
        

        


    }

    //here we make sure openAL keeps doing its thing to process
    //all our data
    void AudioEngine::Update()
    {

    }

    //properly delete/close all assets used by openAL
    //remember reverse order from init/open
    void AudioEngine::Shutdown()
    {

        alDeleteSources(1, &source);
        alDeleteBuffers(1, &buffer);

        //we done with context, stop it being current then destroy it
        alcMakeContextCurrent(NULL);
        alcDestroyContext(m_context);

        CheckContextError(m_device, __LINE__); //dunno if this is actually needed

        //close the audio output device
        ALCboolean closed = alcCloseDevice(m_device);
        if (!closed)
        {
            std::cout << "ERROR: audio device closed wrong???\n";
        }
    }

    //checks for error values from alc functions
    //returns true for error, prints message abt error
    void AudioEngine::CheckContextError(ALCdevice* _device, const std::int_fast32_t _line)
    {
        //check for context error
        ALCenum error_context = alcGetError(_device);

        //interpret if exists
        if (error_context != ALC_NO_ERROR)
        {
            std::cout << "ERROR: found Context error at line " << _line << std::endl;
            
            switch (error_context)
            {
            case ALC_INVALID_CONTEXT:
                std::cout << "ERROR is: ALC_INVALID_CONTEXT\n";
                break;
            case ALC_INVALID_DEVICE:
                std::cout << "ERROR is: ALC_INVALID_DEVICE\n";
                break;
            case ALC_INVALID_ENUM:
                std::cout << "ERROR is: ALC_INVALID_ENUM\n";
                break;
            case ALC_INVALID_VALUE:
                std::cout << "ERROR is: ALC_INVALID_VALUE\n";
                break;
            case ALC_OUT_OF_MEMORY:
                std::cout << "ERROR is: ALC_OUT_OF_MEMORY\n";
                break;
            default:
                std::cout << "UNKNOWN ERROR.\n";
            }
        }
    }

    //checks for error values from al functions
    //returns true for error, prints message abt error
    void AudioEngine::CheckNormalError(const std::int_fast32_t _line)
    {
        //check for normal error
        ALenum error = alGetError();

        //interpret if exists
        if (error != AL_NO_ERROR)
        {
            std::cout << "ERROR: found normal error at line " << _line << std::endl;

            switch (error)
            {
            case AL_INVALID_NAME:
                std::cout << "ERROR is: AL_INVALID_NAME\n";
                break;
            case AL_INVALID_ENUM:
                std::cout << "ERROR is: AL_INVALID_ENUM\n";
                break;
            case AL_INVALID_VALUE:
                std::cout << "ERROR is: AL_INVALID_VALUE\n";
                break;
            case AL_INVALID_OPERATION:
                std::cout << "ERROR is: AL_INVALID_OPERATION\n";
                break;
            case AL_OUT_OF_MEMORY:
                std::cout << "ERROR is: AL_OUT_OF_MEMORY\n";
                break;
            default:
                std::cout << "UNKNOWN ERROR.\n";
            }

        }
    }
}