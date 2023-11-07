//------------------------------------------------------------------------------
//
// File Name:	Serializer.cpp
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Serializer.h"
#include <fstream>
#include "ISystem.h"
#include "TransformSys.h"
namespace Hostile
{
	class Serializer : public ISerializer
	{
	public:
		Serializer()
		{

		}

		void WriteEntityToFile(const flecs::entity& _current)
		{
			std::ofstream outfile("Content/" + std::string(_current.get<ObjectName>()->name) + ".json");
			nlohmann::json obj = WriteEntity(_current);
			nlohmann::json final;
			final["Entity"] = obj;
			outfile << final;
		}

		nlohmann::json WriteEntity(const flecs::entity& _current)
		{
			nlohmann::json obj = nlohmann::json::object();
			obj["Name"] = _current.get<ObjectName>()->name;
			std::vector<nlohmann::json> comps;
			std::vector<nlohmann::json> children;
			_current.each([&](flecs::id id) {
				if (!id.is_pair())
				{
					std::string name = id.entity().name();
					assert(m_map.find(name) != m_map.end()); // Component is not registered to serializer
					m_map[name]->Write(_current, comps, name);

				}
				});
			_current.children([&](flecs::entity target) {
				children.push_back(WriteEntity(target));
				});
			obj["Components"] = comps;
			obj["Children"] = children;
			return obj;

		}



		void AddComponent(const std::string _name, ISystemPtr _sys)
		{
			m_map[_name] = _sys;
		}

	private:
		std::unordered_map<std::string, ISystemPtr> m_map;
	};

	ISerializer& ISerializer::Get()
	{
		static Serializer serializer;
		return serializer;
	}
}