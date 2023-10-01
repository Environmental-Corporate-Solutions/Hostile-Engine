#include "stdafx.h"
#include "ScriptEngine.h"
#include <fstream>
#include <iostream>
#include <filesystem>

#include "imgui.h"
#include "ScriptCompiler.h"

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
		std::filesystem::path MonoRuntimePath;
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
			Log::Warn("File is Empty : {}", _fileName.string());
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

		MonoAssembly* compiler = LoadMonoAssembly(s_Data.ProgramPath / "HostileEngine-Compiler.dll");
		MonoImage* compilerImage = mono_assembly_get_image(compiler);

		auto& metaData = GetReferencedAssembliesMetadata(compilerImage);
		for (auto& meta:metaData)
		{
			if(meta.Name=="mscorlib")
				continue;
			auto dllName = meta.Name + ".dll";
			LoadMonoAssembly(s_Data.MonoRuntimePath / dllName);
		}
		
		ScriptCompiler::Init(compiler, s_Data.ProgramPath);
		ScriptCompiler::CompileAllCSFiles();
		
		//LoadAppAssembly("HostileEngineApp.dll");
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
	}

	void ScriptEngine::Draw()
	{
		ImGui::Begin("Script");
		ScriptCompiler::DrawConsole();
		ImGui::End();
	}

	void ScriptEngine::LoadAssembly(const std::filesystem::path& _relFilepath)
	{
		s_Data.AppDomain = mono_domain_create_appdomain(s_AppDomainName.data(), nullptr);
		mono_domain_set(s_Data.AppDomain, true);

		//LoadMonoAssembly(s_Data.MonoRuntimePath / "System.Xml.dll"); might need in future

		s_Data.CoreAssembly = LoadMonoAssembly(s_Data.ProgramPath / _relFilepath);
		s_Data.CoreAssemblyImage = mono_assembly_get_image(s_Data.CoreAssembly);
	}

	void ScriptEngine::LoadAppAssembly(const std::filesystem::path& _relFilepath)
	{
		MonoAssembly* assembly = LoadMonoAssembly(s_Data.ProgramPath / _relFilepath);
		if(assembly==nullptr)
		{
			return;
		}
		s_Data.AppAssembly = assembly;
		s_Data.AppAssemblyImage = mono_assembly_get_image(s_Data.AppAssembly);
	}

	void ScriptEngine::SetMonoAssembliesPath(const std::filesystem::path& _programArg)
	{
		s_Data.ProgramPath = _programArg.parent_path();
		s_Data.MonoRuntimePath = s_Data.ProgramPath / "mono" / "lib" / "mono" / "4.5";
		mono_set_assemblies_path(s_Data.MonoRuntimePath.string().c_str());
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
		
		if(fileSize<=0)
		{
			Log::Warn("ScriptEngine::LoadMonoAssembly - skipped to load {} since the file size is zero", _assemblyPath.string());
			return nullptr;
		}

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
		//PrintAssemblyTypes(assembly);

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

	std::vector<AssemblyMetadata> ScriptEngine::GetReferencedAssembliesMetadata(MonoImage* image)
	{
		const MonoTableInfo* t = mono_image_get_table_info(image, MONO_TABLE_ASSEMBLYREF);
		int rows = mono_table_info_get_rows(t);

		std::vector<AssemblyMetadata> metadata;
		for (int i = 0; i < rows; i++)
		{
			uint32_t cols[MONO_ASSEMBLYREF_SIZE];
			mono_metadata_decode_row(t, i, cols, MONO_ASSEMBLYREF_SIZE);

			auto& assemblyMetadata = metadata.emplace_back();
			assemblyMetadata.Name = mono_metadata_string_heap(image, cols[MONO_ASSEMBLYREF_NAME]);
			assemblyMetadata.MajorVersion = cols[MONO_ASSEMBLYREF_MAJOR_VERSION];
			assemblyMetadata.MinorVersion = cols[MONO_ASSEMBLYREF_MINOR_VERSION];
			assemblyMetadata.BuildVersion = cols[MONO_ASSEMBLYREF_BUILD_NUMBER];
			assemblyMetadata.RevisionVersion = cols[MONO_ASSEMBLYREF_REV_NUMBER];

			//Log::Warn("{} : {}.{} - {}", assemblyMetadata.Name, assemblyMetadata.MajorVersion, assemblyMetadata.MinorVersion, assemblyMetadata.BuildVersion);
		}

		return metadata;
	}

}
