#pragma once
extern "C" {
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoClassField MonoClassField;
	typedef struct _MonoString MonoString;
	typedef struct _MonoType MonoType;
}

using GCHandle = void*;

class GarbageCollector
{
public:
	static void Init();
	static void Shutdown();

	static void CollectGarbage(bool blockUntilFinalized = true);

	static GCHandle CreateObjectReference(MonoObject* managedObject, bool weakReference, bool pinned = false, bool track = true);
	static bool IsHandleValid(GCHandle handle);
	static MonoObject* GetReferencedObject(GCHandle handle);
	static void ReleaseObjectReference(GCHandle handle);
};