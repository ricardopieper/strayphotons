#pragma once

#include <glm/glm.hpp>
#include <PxActor.h>
#include <PxRigidDynamic.h>
#include "physx/PhysxActorDesc.hh"

namespace sp
{
	class Model;
}

namespace ecs
{
	struct Physics
	{
		Physics() {}
		Physics(
			physx::PxRigidActor *actor,
			shared_ptr<sp::Model> model,
			sp::PhysxActorDesc desc
		) : actor(actor), model(model), desc(desc) {}

		physx::PxRigidActor *actor = nullptr;
		shared_ptr<sp::Model> model;
		sp::PhysxActorDesc desc;

		glm::vec3 scale = glm::vec3(1.0);
	};
}
