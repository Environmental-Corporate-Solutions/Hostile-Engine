#include "stdafx.h"
#include "ScriptHandle.h"

ScriptHandle::ScriptHandle()
	:refCounter(new int(0)),uuid()
{
	++(*refCounter);
}

ScriptHandle::ScriptHandle(const ScriptHandle& other)
	:refCounter(other.refCounter), uuid(other.uuid)
{
	++(*refCounter);
}

ScriptHandle::~ScriptHandle()
{
	--(*refCounter);
	if(*refCounter<=0)
	{
		delete refCounter;
		//todo delete actual script instance
	}
}

UniqueID ScriptHandle::Get() const
{
	return uuid;
}
