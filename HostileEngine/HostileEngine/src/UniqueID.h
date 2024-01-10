#pragma once

class UniqueID
{
public:
	UniqueID();
	UniqueID(uint64_t uuid);
	UniqueID(const UniqueID& other);

	operator uint64_t () { return m_UUID; }
	operator const uint64_t() const { return m_UUID; }
private:
	uint64_t m_UUID;
};


namespace std {

	template <>
	struct hash<UniqueID>
	{
		std::size_t operator()(const UniqueID& uuid) const
		{
			// uuid is already a randomly generated number
			//so good for hash just use it as it is
			return uuid;
		}
	};
}