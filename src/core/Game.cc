#include "core/Game.hh"
#include "core/Logging.hh"
#include "core/Console.hh"

#include "ecs/components/Renderable.hh"
#include "ecs/components/Physics.hh"

#include "ecs/components/Transform.hh"
#include "ecs/components/Controller.hh"
#include "ecs/components/View.hh"
#include "ecs/components/Light.hh"

#include <glm/glm.hpp>

namespace sp
{
	Game::Game() : graphics(this), logic(this), physics()
	{
		// pre-register all of our component types so that errors do not arise if they
		// are queried for before an instance is ever created
		entityManager.RegisterComponentType<ECS::Renderable>();
		entityManager.RegisterComponentType<ECS::Transform>();
		entityManager.RegisterComponentType<ECS::Physics>();
		entityManager.RegisterComponentType<ecs::HumanController>();
		entityManager.RegisterComponentType<ecs::View>();
		entityManager.RegisterComponentType<ecs::Light>();
	}

	Game::~Game()
	{
	}

	void Game::Start()
	{
		try
		{
			logic.Init();
			graphics.CreateContext();
			graphics.BindContextInputCallbacks(input);
			gui.BindInput(input);
			lastFrameTime = glfwGetTime();

			while (true)
			{
				if (ShouldStop()) break;
				if (!Frame()) break;
			}
		}
		catch (char const *err)
		{
			Errorf(err);
		}
	}

	bool Game::Frame()
	{
		double frameTime = glfwGetTime();
		double dt = frameTime - lastFrameTime;

		input.Checkpoint();
		if (input.IsDown(GLFW_KEY_ESCAPE))
		{
			return false;
		}

		GConsoleManager.Update();

		if (!logic.Frame(dt)) return false;
		if (!graphics.Frame()) return false;
		physics.Frame(-dt);


		for (Entity ent : entityManager.EntitiesWith<ECS::Physics>())
		{
			auto physics = ent.Get<ECS::Physics>();
			physx::PxTransform pxT = physics->actor->getGlobalPose();
			physx::PxVec3 p = pxT.p;
			physx::PxQuat q = pxT.q;
			auto transform = ent.Get<ECS::Transform>();

			glm::vec3 glmV;
			glmV.x = p.x;
			glmV.y = p.y;
			glmV.z = p.z;

			transform->SetTransform(glmV);
			//TODO: Speak to Cory about getting transform to take in a quat
			//transform->Rotate((glm::quat) q);
		}

		lastFrameTime = frameTime;
		return true;
	}

	bool Game::ShouldStop()
	{
		return !graphics.HasActiveContext();
	}
}

