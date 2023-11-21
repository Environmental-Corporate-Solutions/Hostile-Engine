#include "stdafx.h"
#include "UniqueID.h"
#include <random>



static std::random_device s_RandomDevice;
static std::mt19937_64 eng(s_RandomDevice());
static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

UniqueID::UniqueID()
	: m_UUID(s_UniformDistribution(eng))
{
}

UniqueID::UniqueID(uint64_t uuid)
	: m_UUID(uuid)
{
}

UniqueID::UniqueID(const UniqueID& other)
	: m_UUID(other.m_UUID)
{
}
