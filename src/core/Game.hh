#pragma once

#include "physx/PhysxManager.hh"
#include "graphics/GraphicsManager.hh"
#include "audio/AudioManager.hh"
#include "Common.hh"
#include "game/GameLogic.hh"
#include "game/InputManager.hh"
#include "game/GuiManager.hh"

#include <Ecs.hh>

namespace cxxopts
{
	class Options;
}

namespace sp
{
	class Game
	{
	public:
		Game(cxxopts::Options &options);
		~Game();

		void Start();
		bool Frame();
		bool ShouldStop();

		cxxopts::Options &options;

		// Order is important.
		GuiManager gui;
		GraphicsManager graphics;
		InputManager input;
		AudioManager audio;
		ecs::EntityManager entityManager;
		GameLogic logic;
		PhysxManager physics;

	private:
		double lastFrameTime;
	};
}
