#pragma once

#include <Ecs.hh>
#include "ecs/components/Controller.hh"
#include "game/InputManager.hh"

#include "physx/PhysxManager.hh"

#include <glm/glm.hpp>

namespace ecs
{
	class HumanControlSystem
	{
	public:
		HumanControlSystem(ecs::EntityManager *entities, sp::InputManager *input, sp::PhysxManager *physics);
		~HumanControlSystem();

		/**
		 * Call this once per frame
		 */
		bool Frame(double dtSinceLastFrame);

		/**
		 * Assigns a default HumanController to the given entity.
		 */
		ecs::Handle<HumanController> AssignController(ecs::Entity entity, sp::PhysxManager &px);

	private:

		/**
		 * Move an entity in the given local direction based on how much time has passed
		 * since last frame.
		 */
		void move(ecs::Entity entity, double dt, glm::vec3 normalizedDirection, bool flight = false);

		void controllerMove(ecs::Entity entity, double dt, glm::vec3 movement);

		/**
		* Pick up the object that the player is looking at and make it move at to a fixed location relative to camera
		*/
		void interact(ecs::Entity entity, double dt);

		static const float MOVE_SPEED;
		static const glm::vec2 CURSOR_SENSITIVITY;

		ecs::EntityManager *entities;
		sp::InputManager *input;
		sp::PhysxManager *physics;

	};
}