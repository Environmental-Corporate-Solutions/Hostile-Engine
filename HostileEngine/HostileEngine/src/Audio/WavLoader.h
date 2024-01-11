//------------------------------------------------------------------------------
//
// File Name:	WavLoader.h
// Author(s):	Kiara Santiago
//						
//
// Copyright ?2023 - 2024 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once

namespace Hostile 
{
    class WavObject
    {
    public:

        WavObject();

        void LoadWav(const std::string _name);

        unsigned GetChannelCount() const { return m_channelCount; }
        unsigned GetSamplingRate() const { return m_samplingRate; }
        unsigned GetBitDepth() const { return m_bitDepth; }
        unsigned GetFrameCount() const { return m_frameCount; }
        std::string GetName() const { return m_name; }

        bool IsMono() const { return m_channelCount == 1; }
        bool IsStereo() const { return m_channelCount == 2; }

        void ClearData() { m_sampleData.clear(); }

        std::vector<char> m_sampleData;
    private:

        std::string m_name;
        unsigned m_channelCount;
        unsigned m_samplingRate;
        unsigned m_bitDepth;
        unsigned m_frameCount; //aka number of samples in the data
    };
}