#include "graphics/GraphicsManager.hh"
#include "core/Logging.hh"
#include "graphics/Renderer.hh"
#include "graphics/raytrace/RaytracedRenderer.hh"
#include "graphics/basic_renderer/BasicRenderer.hh"
#include "graphics/GuiRenderer.hh"
#include "graphics/GPUTimer.hh"
#include "ecs/components/View.hh"
#include "ecs/components/Transform.hh"
#include "core/Game.hh"
#include "core/CVar.hh"

#include <cxxopts.hpp>
#include <iostream>
#include <system_error>

//#define SP_ENABLE_RAYTRACER

namespace sp
{
	CVar<glm::ivec2> CVarWindowSize("r.Size", { 1600, 900 }, "Window height");
	CVar<float> CVarWindowScale("r.Scale", 1.0f, "Scale framebuffer");
	CVar<float> CVarFieldOfView("r.FieldOfView", 60, "Camera field of view");
	CVar<int> CVarWindowFullscreen("r.Fullscreen", false, "Fullscreen window (0: window, 1: fullscreen)");

#ifdef SP_ENABLE_RAYTRACER
	static CVar<int> CVarRayTrace("r.RayTrace", false, "Run reference raytracer");
#endif

	static void glfwErrorCallback(int error, const char *message)
	{
		Errorf("GLFW returned %d: %s", error, message);
	}

	GraphicsManager::GraphicsManager(Game *game) : game(game)
	{
		if (game->options.count("basic-renderer"))
		{
			Logf("Graphics starting up (basic renderer)");
			useBasic = true;
		}
		else
		{
			Logf("Graphics starting up (full renderer)");
		}

		glfwSetErrorCallback(glfwErrorCallback);

		if (!glfwInit())
		{
			throw "glfw failed";
		}
	}

	GraphicsManager::~GraphicsManager()
	{
		if (context) ReleaseContext();
		glfwTerminate();
	}

	void GraphicsManager::CreateContext()
	{
		if (context) throw "already an active context";

		if (useBasic)
		{
			context = new BasicRenderer(game);
			context->CreateWindow(CVarWindowSize.Get());
			return;
		}

		auto renderer = new Renderer(game);
		context = renderer;
		context->CreateWindow(CVarWindowSize.Get());

#ifdef SP_ENABLE_RAYTRACER
		rayTracer = new raytrace::RaytracedRenderer(game, renderer);
#endif

		profilerGui = new ProfilerGui(context->Timer);
		game->debugGui.Attach(profilerGui);
	}

	void GraphicsManager::ReleaseContext()
	{
		if (!context) throw "no active context";

		if (profilerGui) delete profilerGui;

#ifdef SP_ENABLE_RAYTRACER
		if (rayTracer) delete rayTracer;
#endif

		delete context;
	}

	void GraphicsManager::ReloadContext()
	{
		//context->Reload();
	}

	bool GraphicsManager::HasActiveContext()
	{
		return context && !context->ShouldClose();
	}

	void GraphicsManager::BindContextInputCallbacks(InputManager &inputManager)
	{
		context->BindInputCallbacks(inputManager);
	}

	bool GraphicsManager::Frame()
	{
		if (!context) throw "no active context";
		if (!HasActiveContext()) return false;

		ecs::View primaryView;
		if (playerView.Valid())
		{
			auto newSize = CVarWindowSize.Get();
			auto newFov = glm::radians(CVarFieldOfView.Get());
			auto newScale = CVarWindowScale.Get();

			if (newSize != primaryView.extents || newFov != primaryView.fov || newScale != primaryView.scale)
			{
				auto view = playerView.Get<ecs::View>();
				view->extents = newSize;
				view->fov = newFov;
				view->scale = newScale;
			}

			primaryView = *ecs::UpdateViewCache(playerView);
		}
		else
		{
			// Default view
			primaryView.extents = CVarWindowSize.Get();
		}

		context->ResizeWindow(primaryView, CVarWindowScale.Get(), CVarWindowFullscreen.Get());

		context->Timer->StartFrame();

		{
			RenderPhase phase("Frame", context->Timer);
			context->BeginFrame();

#ifdef SP_ENABLE_RAYTRACER
			if (CVarRayTrace.Get() && rayTracer->Enable(primaryView))
			{
				rayTracer->Render();
			}
			else
			{
				rayTracer->Disable();
			}
#endif

			context->RenderPass(primaryView);
			context->EndFrame();
		}

		glfwSwapBuffers(context->GetWindow());
		context->Timer->EndFrame();

		double frameEnd = glfwGetTime();
		fpsTimer += frameEnd - lastFrameEnd;
		frameCounter++;

		if (fpsTimer > 1.0)
		{
			context->SetTitle("STRAY PHOTONS (" + std::to_string(frameCounter) + " FPS)");
			frameCounter = 0;
			fpsTimer = 0;
		}

		lastFrameEnd = frameEnd;
		glfwPollEvents();
		return true;
	}

	/**
	* This View will be used when rendering from the player's viewpoint
	*/
	void GraphicsManager::SetPlayerView(ecs::Entity entity)
	{
		ecs::ValidateView(entity);
		playerView = entity;
	}

	void GraphicsManager::RenderLoading()
	{
		if (!context) return;

		ecs::View primaryView;
		if (playerView.Valid()) primaryView = *ecs::UpdateViewCache(playerView);

		primaryView.extents = CVarWindowSize.Get();
		primaryView.blend = true;
		primaryView.clearMode = 0;

		context->RenderLoading(primaryView);
	}
}
