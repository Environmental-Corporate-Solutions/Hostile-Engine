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
namespace Script
{
	struct AssemblyMetadata
	{
		std::string Name;
		uint32_t MajorVersion;
		uint32_t MinorVersion;
		uint32_t BuildVersion;
		uint32_t RevisionVersion;

		bool operator==(const AssemblyMetadata& other) const
		{
			return Name == other.Name && MajorVersion == other.MajorVersion && MinorVersion == other.MinorVersion && BuildVersion == other.BuildVersion && RevisionVersion == other.RevisionVersion;
		}

		bool operator!=(const AssemblyMetadata& other) const { return !(*this == other); }
	};

	class ScriptEngine
	{
	public:
		static void Init(char* _programArg);
		static void Shutdown();

		static void LoadAssembly(const std::filesystem::path& _relFilepath);
		static void LoadAppAssembly(const std::filesystem::path& _relFilepath);

	private:
		static void SetMonoAssembliesPath(const std::filesystem::path& _programArg);

		static void InitMono();
		static void ShutdownMono();

		/**
		 * \brief Load the script dll and get assembly of it.
		 * \param _assemblyPath the path to the script dll file 
		 * \return The assembly of the file 
		 */
		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& _assemblyPath);
		static void PrintAssemblyTypes(MonoAssembly* _assembly);
		static std::vector<AssemblyMetadata> ScriptEngine::GetReferencedAssembliesMetadata(MonoImage* image);
	};
}