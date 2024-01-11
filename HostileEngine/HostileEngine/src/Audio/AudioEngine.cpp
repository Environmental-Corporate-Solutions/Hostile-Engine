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


#include <AL/alext.h>
#include <iostream>

namespace Hostile
{
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


        //we only have one source rn
        alGenSources(1, &m_source);
        alSourcef(m_source, AL_PITCH, 1);
        alSourcef(m_source, AL_GAIN, 1.0f);
        alSource3f(m_source, AL_POSITION, 0, 0, 0);
        alSource3f(m_source, AL_VELOCITY, 0, 0, 0);
        alSourcei(m_source, AL_LOOPING, AL_FALSE);

        std::vector<std::string> temp;
        LoadManyWav(temp);

        alSourcei(m_source, AL_BUFFER, m_audioObjects[0].m_buffer);
        CheckNormalError(__LINE__);

        alSourcePlay(m_source);
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

        alDeleteSources(1, &m_source);
        UnloadManyWav();

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

    //finds out what the format is for the given wav object
    ALenum AudioEngine::GetFormat(WavObject _wav)
    {
        ALenum format;

        if (_wav.IsMono())
        {
            if (_wav.GetBitDepth() == 8)
            {
                format = AL_FORMAT_MONO8;
            }
            else if (_wav.GetBitDepth() == 16)
            {
                format = AL_FORMAT_MONO16;
            }
        }
        else if (_wav.IsStereo())
        {
            if (_wav.GetBitDepth() == 8)
            {
                format = AL_FORMAT_STEREO8;
            }
            else if (_wav.GetBitDepth() == 16)
            {
                format = AL_FORMAT_STEREO16;
            }
        }
        else
        {
            std::cout << "ERROR: wav format error\n";
        }

        return format;
    }

    //loads all the wav files passed it
    void AudioEngine::LoadManyWav(std::vector<std::string> _names)
    {
        (void)_names;

        /*
        m_audioObjects.resize(_names.size());
        for (int k = 0; k < _names.size(); ++k)
        {
            WavObject temp;
            temp.LoadWav(_names[k]);
            ALenum format = GetFormat(temp);

            alGenBuffers(k+1, &m_audioObjects[k].m_buffer);
            CheckNormalError(__LINE__);
            
            alBufferData(m_audioObjects[k].m_buffer, format, temp.m_sampleData.data(),
                temp.m_sampleData.size(), temp.GetSamplingRate());
            CheckNormalError(__LINE__);
            
            temp.ClearData();
            
            m_audioObjects[k].m_name = temp.GetName();
            
        }*/

        m_audioObjects.resize(1);
        WavObject temp; 

        temp.LoadWav("./Assets/audio/ethanTest1.wav");
        ALenum format = GetFormat(temp);
        
        alGenBuffers(1, &m_audioObjects[0].m_buffer);
        CheckNormalError(__LINE__);

        alBufferData(m_audioObjects[0].m_buffer, format, temp.m_sampleData.data(),
            temp.m_sampleData.size(), temp.GetSamplingRate());
        CheckNormalError(__LINE__);

        temp.ClearData();

        m_audioObjects[0].m_name = temp.GetName();
    }

    //unloads all the currently stored wav files
    void AudioEngine::UnloadManyWav()
    {
        for (int k = 0; k < m_audioObjects.size(); ++k)
        {
            alDeleteBuffers(k+1, &m_audioObjects[k].m_buffer);
        }
    }
}