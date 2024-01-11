//------------------------------------------------------------------------------
//
// File Name:	WavLoader.cpp
// Author(s):	Kiara Santiago
// Description: Takes Wav file location and loads it into our wav object
//
// Copyright ?2023 - 2024 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "stdafx.h"
#include "WavLoader.h"
#include <fstream>
#include <iostream>

namespace Hostile
{
    //default constructor --- sets some basic values but is otherwise, does nothing
    WavObject::WavObject() : m_frameCount(0), m_bitDepth(0), m_channelCount(1), m_samplingRate(44100)
    { }

    //take the file path of the wav we wanna load, then we attempt to load it
    //yells if not valid wav file 
    void WavObject::LoadWav(const std::string _name)
    {
        //set name to filepath passed in
        m_name = _name;

        //get our stream and try to open
        std::fstream read_in(_name, std::ios_base::binary | std::ios_base::in);

        if (!read_in)
        {
            std::cout << "ERROR: failed to open file " << _name << std::endl;
        }

        //start reading in wav info
        struct
        {
            char info[4];
            unsigned size;
        } chunk;
        char tag[4];
        char fmt[16];


        //RIFF
        read_in.read(reinterpret_cast<char*>(&chunk), 8);
        if (!read_in || (strncmp(chunk.info, "RIFF", 4) != 0))
        {
            std::cout << "ERROR: couldnt read RIFF\n";
        }

        //WAVE
        read_in.read(tag, 4);
        if (!read_in || (strncmp(tag, "WAVE", 4) != 0))
        {
            std::cout << "ERROR: couldnt read WAVE\n";
        }

        //find 'fmt ' 
        read_in.read(reinterpret_cast<char*>(&chunk), 8);
        while (read_in && (strncmp(chunk.info, "fmt ", 4) != 0))
        {
            read_in.seekg(chunk.size, std::ios_base::cur);
            read_in.read(reinterpret_cast<char*>(&chunk), 8);
        }

        //couldnt find it?
        if (!read_in)
        {
            std::cout << "ERROR: couldnt find fmt\n";
        }

        //read in fmt
        read_in.read(fmt, 16);

        //read channelCount, sampleRate, bitDepth
        unsigned short channels = *reinterpret_cast<unsigned short*>(fmt + 2);
        unsigned rate = *reinterpret_cast<unsigned*>(fmt + 4);
        unsigned short bits = *reinterpret_cast<unsigned short*>(fmt + 14);

        //find data chunk
        read_in.seekg(chunk.size - 16, std::ios_base::cur);
        read_in.read(reinterpret_cast<char*>(&chunk), 8);

        while (read_in && (strncmp(chunk.info, "data", 4) != 0))
        {
            read_in.seekg(chunk.size, std::ios_base::cur);
            read_in.read(reinterpret_cast<char*>(&chunk), 8);
        }

        //couldnt find data tag?
        if (!read_in)
        {
            std::cout << "ERROR couldnt find data tag\n";
        }

        //read frameCount
        unsigned count = (8 * chunk.size) / (channels * bits);


        //transfer all that into class
        m_frameCount = count;
        m_channelCount = channels;
        m_samplingRate = rate;
        m_bitDepth = bits;

        //read in actual data
        unsigned size = chunk.size;
        m_sampleData.resize(size);
        read_in.read(m_sampleData.data(), size);

        //all done
        read_in.close();
    }
}