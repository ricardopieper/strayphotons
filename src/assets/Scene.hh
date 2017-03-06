#pragma once

#include "Common.hh"
#include <Ecs.hh>

#include <unordered_map>

namespace sp
{
	class Asset;

	class Scene : public NonCopyable
	{
	public:
		Scene(const string &name, shared_ptr<Asset> asset);
		~Scene() {}

		const string name;
		vector<ecs::Entity> entities;
		std::unordered_map<string, ecs::Entity> namedEntities;

		ecs::Entity FindEntity(const std::string name);

		vector<string> autoexecList;
	private:
		shared_ptr<Asset> asset;
	};
}
