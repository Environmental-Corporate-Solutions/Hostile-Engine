#include "stdafx.h"
#include "GarbageCollector.h"
#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/profiler.h>
using ReferenceMap = std::unordered_map<GCHandle, MonoObject*>;

struct GCState
{
	ReferenceMap StrongReferences;
	ReferenceMap WeakReferences;
};

static GCState* s_GCState = nullptr;
void GarbageCollector::Init()
{
	s_GCState = new GCState();
}

void GarbageCollector::Shutdown()
{
	if (s_GCState->StrongReferences.size() > 0)
	{
		for (auto [handle, monoObject] : s_GCState->StrongReferences)
			mono_gchandle_free_v2(handle);

		s_GCState->StrongReferences.clear();
	}

	if (s_GCState->WeakReferences.size() > 0)
	{
		for (auto [handle, monoObject] : s_GCState->WeakReferences)
			mono_gchandle_free_v2(handle);

		s_GCState->WeakReferences.clear();
	}

	// Collect any leftover garbage
	mono_gc_collect(mono_gc_max_generation());
	while (mono_gc_pending_finalizers());

	delete s_GCState;
	s_GCState = nullptr;
}

void GarbageCollector::CollectGarbage(bool blockUntilFinalized)
{
	mono_gc_collect(mono_gc_max_generation());
	if (blockUntilFinalized)
	{
		while (mono_gc_pending_finalizers());
	}
}

GCHandle GarbageCollector::CreateObjectReference(MonoObject* managedObject, bool weakReference, bool pinned, bool track)
{
	GCHandle handle = weakReference ? mono_gchandle_new_weakref_v2(managedObject, pinned) : mono_gchandle_new_v2(managedObject, pinned);

	if (track)
	{
		if (weakReference)
			s_GCState->WeakReferences[handle] = managedObject;
		else
			s_GCState->StrongReferences[handle] = managedObject;
	}

	return handle;
}

bool GarbageCollector::IsHandleValid(GCHandle handle)
{
	if (handle == nullptr)
		return false;

	MonoObject* obj = mono_gchandle_get_target_v2(handle);

	if (obj == nullptr)
		return false;

	if (mono_object_get_vtable(obj) == nullptr)
		return false;

	return true;
}

MonoObject* GarbageCollector::GetReferencedObject(GCHandle handle)
{
	MonoObject* obj = mono_gchandle_get_target_v2(handle);
	if (obj == nullptr || mono_object_get_vtable(obj) == nullptr)
		return nullptr;
	return obj;
}

void GarbageCollector::ReleaseObjectReference(GCHandle handle)
{
	if (mono_gchandle_get_target_v2(handle) != nullptr)
	{
		mono_gchandle_free_v2(handle);
	}
	else
	{
		return;
	}

	if (s_GCState->StrongReferences.find(handle) != s_GCState->StrongReferences.end())
		s_GCState->StrongReferences.erase(handle);

	if (s_GCState->WeakReferences.find(handle) != s_GCState->WeakReferences.end())
		s_GCState->WeakReferences.erase(handle);
}