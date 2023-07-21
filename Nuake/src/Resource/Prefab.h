#pragma once
#include "src/Scene/Entities/Entity.h"
#include "src/Scene/Components/ParentComponent.h"
#include "src/Resource/Serializable.h"
#include "Engine.h"

#include <string>
#include <vector>
#include <src/Scene/Components/PrefabComponent.h>
namespace Nuake 
{
	class Prefab : ISerializable
	{
	public:
		std::string Path;
		std::vector<Entity> Entities;
		Entity Root;

		static Ref<Prefab> CreatePrefabFromEntity(Entity entity);

		static Ref<Prefab> New(const std::string& path);

		Prefab() 
		{
			Path = "";
			Entities = std::vector<Entity>();
		}
		
		void AddEntity(Entity& entity)
		{
			Entities.push_back(entity);
		}

		void SaveAs(const std::string& path)
		{
			Path = path;

			FileSystem::BeginWriteFile(path);
			FileSystem::WriteLine(Serialize().dump(4));
			FileSystem::EndWriteFile();
		}

		json Serialize()
		{
			BEGIN_SERIALIZE();
			SERIALIZE_VAL(Path);

			std::vector<json> entities = std::vector<json>();
			for (Entity e : Entities)
				entities.push_back(e.Serialize());

			SERIALIZE_VAL_LBL("Entities", entities);

			// Manual correction to remove parent if we are prefabing a child. 
			// We want to get rid of parent link in the prefab itself.
			for (auto& e : j["Entities"])
			{
				if (e["NameComponent"]["ID"] == Root.GetComponent<NameComponent>().ID)
					e["ParentComponent"]["HasParent"] = false;
			}
				
			END_SERIALIZE();
		}

		bool Deserialize(const std::string& str)
		{
			if (str == "")
				return false;

			BEGIN_DESERIALIZE();
			Path = j["Path"];
			if (j.contains("Entities"))
			{
				for (json e : j["Entities"])
				{
					Entity entity = Entity { Engine::GetCurrentScene()->m_Registry.create(), Engine::GetCurrentScene().get() };
					entity.Deserialize(e.dump());
					//entity.AddComponent<PrefabComponent>().PrefabInstance = std::make_shared(this);
					this->AddEntity(entity);
				}

				for (auto& e : Entities)
				{
					auto parentC = e.GetComponent<ParentComponent>();
					if (!parentC.HasParent)
						continue;

					auto parent = Engine::GetCurrentScene()->GetEntityByID(parentC.ParentID);
					parent.AddChild(e);
				}
			}
			return true;
		}

	private:
		void EntityWalker(Entity entity);
	};
}
