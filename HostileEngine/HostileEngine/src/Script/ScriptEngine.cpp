#include "stdafx.h"
#include "ScriptEngine.h"
#include <fstream>

namespace __ScriptEngineInner
{
	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage = nullptr;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;

		std::filesystem::path ProgramPath;
	};

	static ScriptEngineData s_Data;

	static std::string s_AppDomainName = "HostileEngine_AppDomain";

	//helper functions
	[[nodiscard]]
	bool CheckExists(const std::filesystem::path& _path)
	{
		if (!std::filesystem::exists(_path))
		{
			Log::Error("There is no file : {}", _path.string());
			return false;
		}
		return true;
	}

	[[nodiscard]]
	std::tuple<std::shared_ptr<char[]>, int> ReadFileToBytes(const std::filesystem::path& _fileName)
	{
		if (!CheckExists(_fileName)) return {};

		std::ifstream stream(_fileName, std::ios::binary | std::ios::ate);

		if (!stream)
		{
			// Failed to open the file
			Log::Error("Failed To Open : {}", _fileName.string());
			return { nullptr, 0 };
		}

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		int size = static_cast<int>(end - stream.tellg());

		if (size == 0)
		{
			// File is empty
			Log::Error("File is Empty : {}", _fileName.string());
			return { nullptr, 0 };
		}

		std::shared_ptr<char[]> buffer(new char[size]);
		stream.read(buffer.get(), size);
		stream.close();

		return { buffer, size };
	}
}

namespace Script
{
	using namespace __ScriptEngineInner;
	
	void ScriptEngine::Init(char* _programArg)
	{
		SetMonoAssembliesPath(_programArg);
		InitMono();

		LoadAssembly("HostileEngine-ScriptCore.dll");
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
	}

	void ScriptEngine::LoadAssembly(const std::filesystem::path& _relFilepath)
	{
		s_Data.AppDomain = mono_domain_create_appdomain(s_AppDomainName.data(), nullptr);
		mono_domain_set(s_Data.AppDomain, true);

		s_Data.CoreAssembly = LoadMonoAssembly(s_Data.ProgramPath / _relFilepath);
		s_Data.CoreAssemblyImage = mono_assembly_get_image(s_Data.CoreAssembly);
	}

	void ScriptEngine::LoadAppAssembly(const std::filesystem::path& _relFilepath)
	{
		s_Data.AppAssembly = LoadMonoAssembly(_relFilepath.string());
		s_Data.AppAssemblyImage = mono_assembly_get_image(s_Data.AppAssembly);
	}

	void ScriptEngine::SetMonoAssembliesPath(const std::filesystem::path& _programArg)
	{
		s_Data.ProgramPath = _programArg.parent_path();
		auto finalPath = s_Data.ProgramPath / "mono"/"lib";
		mono_set_assemblies_path(finalPath.string().c_str());
	}

	void ScriptEngine::InitMono()
	{
		//debug
		Log::Debug("Init Mono JIT Runtime");

		s_Data.RootDomain = mono_jit_init("HostileEngine_JITRuntime");
	}

	void ScriptEngine::ShutdownMono()
	{
		mono_domain_set(mono_get_root_domain(), true);

		if (s_Data.AppDomain)
		{
			mono_domain_unload(s_Data.AppDomain);
			s_Data.AppDomain = nullptr;
		}

		if (s_Data.RootDomain)
		{
			mono_jit_cleanup(s_Data.RootDomain);
			s_Data.RootDomain = nullptr;
		}
	}

	MonoAssembly* ScriptEngine::LoadMonoAssembly(const std::filesystem::path& _assemblyPath)
	{
		auto [fileData, fileSize] = ReadFileToBytes(_assemblyPath);

		//We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full(fileData.get(), fileSize, 1, &status, 0);

		if (status != MONO_IMAGE_OK)
		{
			const char* errorMessage = mono_image_strerror(status);
			Log::Error("ScriptEngine::LoadMonoAssembly - mono_image_open_from_data_full (error msg) : {}", errorMessage);
			return nullptr;
		}

		MonoAssembly* assembly = mono_assembly_load_from_full(image, _assemblyPath.string().c_str(), &status, 0);

		if (status != MONO_IMAGE_OK)
		{
			const char* errorMessage = mono_image_strerror(status);
			Log::Error("ScriptEngine::LoadMonoAssembly - mono_assembly_load_from_full (error msg) : {}", errorMessage);
			
			return nullptr;
		}
		mono_image_close(image);

		//debug
		Log::Debug("Loaded DLL: {}", _assemblyPath.filename().string());
		PrintAssemblyTypes(assembly);

		return assembly;
	}

	void ScriptEngine::PrintAssemblyTypes(MonoAssembly* _assembly)
	{
		MonoImage* image = mono_assembly_get_image(_assembly);
		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

		Log::Debug("Assembly Types:");
		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

			Log::Debug("{}.{}", nameSpace, name);
		}
	}
}
