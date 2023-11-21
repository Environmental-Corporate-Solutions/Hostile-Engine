//------------------------------------------------------------------------------
//
// File Name:	Deseralizer.cpp
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Deseralizer.h"
#include <fstream>
#include "Engine.h"
#include "ISystem.h"
#include "TransformSys.h"
#include "Scene.h"


namespace Hostile
{
	class Deseralizer : public IDeseralizer
	{
	public:
		void ReadFile(const char* _filepath)
		{

			std::ifstream file(_filepath);
			nlohmann::json json = nlohmann::json::parse(file);
			auto iter = json.begin();
			while (iter != json.end())
			{
				if (iter.key() == "Entity")
				{
					IEngine::Get().GetCurrentScene()->Add(ReadEntity(iter.value()));
					iter++;
				}
				else if (iter.key() == "Scene")
				{
					if (!IEngine::Get().IsSceneLoaded(iter.value()["Name"].get<std::string>()))
					{

						Scene scene = IEngine::Get().AddScene(iter.value()["Name"].get<std::string>());
						for (nlohmann::json current : iter.value()["Objects"])
						{
							scene.Add(ReadEntity(current));
						}
					}
					else
					{
						Log::Error("Scene already loaded");
					}
					iter++;
				}
			}
		}
		void AddComponent(const std::string _name, ISystemPtr _sys)
		{
			m_map[_name] = _sys;
		}

	private:
		std::unordered_map<std::string, ISystemPtr> m_map;

		flecs::entity& ReadEntity(nlohmann::json& _obj)
		{
			std::string name = _obj["Name"];
			flecs::entity entity = IEngine::Get().CreateEntity();
			entity.set<ObjectName>({ name });
			auto iter = _obj["Components"].begin();
			while (iter != _obj["Components"].end())
			{
				nlohmann::json comp = iter.value();
				std::string type = comp["Type"];
				assert(m_map.find(type) != m_map.end()); //component not register to deserialiser
				m_map[type]->Read(entity, comp, type);
				iter++;
			}
			iter = _obj["Children"].begin();
			while (iter != _obj["Children"].end())
			{
				ReadEntity(iter.value()).child_of(entity);
				iter++;
			}
			return entity;
		}


	};
	IDeseralizer& IDeseralizer::Get()
	{
		static Deseralizer des;
		return des;
	}
}