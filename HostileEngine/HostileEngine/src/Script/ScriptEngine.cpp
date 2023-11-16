#include "stdafx.h"
#include "ScriptEngine.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <flecs.h>

#include "Engine.h"
#include "imgui.h"
#include "ScriptClass.h"
#include "ScriptCompiler.h"
#include "ScriptGlue.h"
#include "ScriptInstance.h"
#include "ScriptSys.h"
#include "UniqueID.h"

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

		//core class
		Script::ScriptClass EntityClass;
		//actually impl of each game objects
		std::unordered_map<std::string, std::shared_ptr<Script::ScriptClass>> EntityClasses;

		std::unordered_map<UniqueID, std::string> EntityClassNameMap;
		std::unordered_map<UniqueID, std::shared_ptr<Script::ScriptInstance>> EntityInstances;
		std::unordered_map<UniqueID, std::string> EntityInstanceNames;
		//std::unordered_map<UniqueID, std::shared_ptr<Script::ScriptInstance>> UUIDInstanceMap;
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
		ScriptGlue::RegisterFunctions();
		ScriptGlue::RegisterComponents();


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

		
		LoadAppAssembly("HostileEngineApp.dll");
		LoadAssemblyClasses();

		s_Data.EntityClass = ScriptClass{ s_Data.CoreAssemblyImage, "HostileEngine","Entity" };
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
	}

	void ScriptEngine::Reload()
	{
		//will drop prev appdomain
		LoadAssembly("HostileEngine-ScriptCore.dll");

		//register functions
		ScriptGlue::RegisterFunctions();
		ScriptGlue::RegisterComponents();

		MonoAssembly* compiler = LoadMonoAssembly(s_Data.ProgramPath / "HostileEngine-Compiler.dll");
		MonoImage* compilerImage = mono_assembly_get_image(compiler);

		auto& metaData = GetReferencedAssembliesMetadata(compilerImage);
		for (auto& meta : metaData)
		{
			if (meta.Name == "mscorlib")
				continue;
			auto dllName = meta.Name + ".dll";
			LoadMonoAssembly(s_Data.MonoRuntimePath / dllName);
		}

		ScriptCompiler::Init(compiler, s_Data.ProgramPath);

		//no need to compile
		//ScriptCompiler::CompileAllCSFiles();

		LoadAppAssembly("HostileEngineApp.dll");

		//will clear tables
		LoadAssemblyClasses();
		auto copy = s_Data.EntityInstanceNames;
		s_Data.EntityInstanceNames.clear();

		s_Data.EntityClass = ScriptClass{ s_Data.CoreAssemblyImage, "HostileEngine","Entity" };

		//should handle running entities
		flecs::world& world = Hostile::IEngine::Get().GetWorld();
		world.filter<Hostile::ScriptComponent>().iter([&](flecs::iter& _it, Hostile::ScriptComponent* _script)
		{
			//todo:handle
			for(const int i:_it)
			{
				auto entity=_it.entity(i);
				auto scriptComp = entity.get_mut<Hostile::ScriptComponent>();
				std::vector<UniqueID> newUUID;
				for (auto uuid: scriptComp->UUIDs)
				{
					std::string className = copy[uuid];
					if(EntityClassExists(className))
					{
						std::shared_ptr<ScriptInstance> instance = std::make_shared<ScriptInstance>(s_Data.EntityClasses[className], entity);
						s_Data.EntityInstances[uuid] = instance;
						s_Data.EntityInstanceNames[uuid] = className;
						// Todo: Copy field values
						/*if (s_Data.EntityScriptFields.contains(entity.GetUUID()))
						{
							const ScriptFieldMap& fieldMap = s_Data->EntityScriptFields.at(entity.GetUUID());
							for (const auto& [name, fieldInstance] : fieldMap)
								instance->SetFieldValueInternal(name, fieldInstance.m_Buffer);
						}*/

						instance->InvokeOnCreate();
						newUUID.push_back(uuid);
					}
				}
				scriptComp->UUIDs = newUUID;
			}

		});

	}

	void ScriptEngine::Draw()
	{
		ImGui::Begin("Script");
		ScriptCompiler::DrawConsole();
		ImGui::End();
	}

	void ScriptEngine::LoadAssembly(const std::filesystem::path& _relFilepath)
	{
		if(s_Data.AppDomain)
		{
			mono_domain_set(mono_get_root_domain(), true);
			mono_domain_unload(s_Data.AppDomain);
			s_Data.AppDomain = nullptr;
		}

		s_Data.AppDomain = mono_domain_create_appdomain(s_AppDomainName.data(), nullptr);
		mono_domain_set(s_Data.AppDomain, true);

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

	ScriptClass& ScriptEngine::GetEntityClass()
	{
		return s_Data.EntityClass;
	}

	ScriptEngine::EntityClassesMap& ScriptEngine::GetEntityClasses()
	{
		return s_Data.EntityClasses;
	}

	std::string ScriptEngine::GetEntityScriptInstanceName(UniqueID _uuid)
	{
		auto found = s_Data.EntityInstanceNames.find(_uuid);
		if (found == s_Data.EntityInstanceNames.end())
			return {};

		return found->second;
	}

	std::shared_ptr<Script::ScriptInstance> ScriptEngine::GetEntityScriptInstance(UniqueID _uuid)
	{
		auto found = s_Data.EntityInstances.find(_uuid);
		if (found == s_Data.EntityInstances.end())
			return nullptr;

		return found->second;
	}

	MonoImage* ScriptEngine::GetCoreAssemblyImage()
	{
		return s_Data.CoreAssemblyImage;
	}

	void ScriptEngine::OnCreateEntity(const std::string& className, flecs::entity _entity)
	{
		Hostile::ScriptComponent* scriptComponent = _entity.get_mut<Hostile::ScriptComponent>();
		if (IsClassExisting(className))
		{
			UniqueID uuid;
			std::shared_ptr<ScriptInstance> instance = std::make_shared<ScriptInstance>(s_Data.EntityClasses[className], _entity);
			s_Data.EntityInstances[uuid] = instance;
			s_Data.EntityInstanceNames[uuid] = className;
			scriptComponent->UUIDs.push_back(uuid);

			// Todo: Copy field values
			/*if (s_Data.EntityScriptFields.contains(entity.GetUUID()))
			{
				const ScriptFieldMap& fieldMap = s_Data->EntityScriptFields.at(entity.GetUUID());
				for (const auto& [name, fieldInstance] : fieldMap)
					instance->SetFieldValueInternal(name, fieldInstance.m_Buffer);
			}*/

			instance->InvokeOnCreate();
		}
		else
		{
			//todo:error or skip
			Log::Error("Script Class not found : {}  (Owner name: {})", className, _entity.name());
		}
	}

	void ScriptEngine::OnUpdateEntity(flecs::entity _entity)
	{
		//ENGINE_ASSERT(s_Data->EntityInstances.contains(entityUUID), "Was Not Instantiate!!");
		const Hostile::ScriptComponent* script_component = _entity.get<Hostile::ScriptComponent>();
		for (auto uuid : script_component->UUIDs)
		{
			bool exist = s_Data.EntityInstances.find(uuid) != s_Data.EntityInstances.end();
			if (exist)
			{
				std::shared_ptr<ScriptInstance> instance = s_Data.EntityInstances[uuid];
				instance->InvokeOnUpdate();
			}
			else
			{
				//Todo:error or skip
			}
		}
	}

	bool ScriptEngine::EntityClassExists(const std::string& className)
	{
		return s_Data.EntityClasses.find(className) != s_Data.EntityClasses.end();
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

	void ScriptEngine::LoadAssemblyClasses()
	{
		if(s_Data.AppAssembly==nullptr)
			return;

		s_Data.EntityClasses.clear();

		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_Data.AppAssemblyImage, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

		MonoClass* entityClass = mono_class_from_name(s_Data.CoreAssemblyImage, "HostileEngine", "Entity");

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(s_Data.AppAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(s_Data.AppAssemblyImage, cols[MONO_TYPEDEF_NAME]);
			std::string className;

			if (strlen(nameSpace) != 0)
			{
				className += nameSpace;
				className += ".";
				className += name;
			}
			else
				className = name;

			MonoClass* monoClass = mono_class_from_name(s_Data.AppAssemblyImage, nameSpace, name);

			if (monoClass == entityClass)
				continue;

			bool is_subclass = mono_class_is_subclass_of(monoClass, entityClass, false);
			if (!is_subclass)
				continue;

			std::shared_ptr<ScriptClass> scriptClass = std::make_shared<ScriptClass>(s_Data.AppAssemblyImage, nameSpace, name);
			s_Data.EntityClasses[className] = scriptClass;

			int fieldCount = mono_class_num_fields(monoClass);
			Log::Info("C# Class {} has {} fields: ", className, fieldCount);
			void* iterator = nullptr;
			while (MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
			{
				const char* fieldName = mono_field_get_name(field);

				uint32_t flags = mono_field_get_flags(field);
				if (flags & MONO_FIELD_ATTR_PUBLIC)
				{
					MonoType* monoType = mono_field_get_type(field);
					scriptClass->m_Fields[fieldName] = ScriptField(monoType, fieldName, field);
				}
			}
		}

		
		std::stringstream ss;
		ss << "\nDetected classes:\n";
		for(auto& klass: s_Data.EntityClasses)
		{
			ss << klass.first << std::endl;
		}

		Log::Info(ss.str());
	}

	bool ScriptEngine::IsClassExisting(const std::string& _classNameWithNameSpace)
	{
		return s_Data.EntityClasses.find(_classNameWithNameSpace) != s_Data.EntityClasses.end();
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(s_Data.AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
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
