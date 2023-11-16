#pragma once
#include "UniqueID.h"

class ScriptHandle
{
public:
	ScriptHandle();
	ScriptHandle(const ScriptHandle& other);
	~ScriptHandle();

	inline UniqueID Get() const;
private:
	int* refCounter;
	UniqueID uuid;
};

namespace std {

	template <>
	struct hash<ScriptHandle>
	{
		std::size_t operator()(const ScriptHandle& script_handle) const
		{
			// uuid is already a randomly generated number
			//so good for hash just use it as it is
			return hash<UniqueID>()(script_handle.Get());
		}
	};
}
