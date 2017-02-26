#pragma once

#include "Common.hh"
#include "Graphics.hh"
#include "core/CVar.hh"

#include <stack>
#include <queue>

namespace sp
{
	class GPUTimer;
	extern CVar<bool> CVarProfileGPU;

	struct GPUTimeResult
	{
		string name;
		int depth = 0;
		uint64 start = 0, end = 0, elapsed = 0;
	};

	struct GPUTimeQuery
	{
		GLuint queries[2];
		int resultIndex;
	};

	class RenderPhase
	{
	public:
		const string name;
		GPUTimer *timer = nullptr;
		GPUTimeQuery query;

		// Create phase without starting the timer.
		RenderPhase(const string &phaseName) : name(phaseName) { };

		// Create phase and automatically start the timer.
		RenderPhase(const string &phaseName, GPUTimer *gpuTimer);

		// Starts the timer if not already started.
		void StartTimer(GPUTimer *gpuTimer);

		~RenderPhase();
	};

	class FrameTiming
	{
	public:
		vector<GPUTimeResult> results;
		size_t remaining = 0;
	};

	class GPUTimer
	{
	public:
		void StartFrame();
		void EndFrame();

		void Register(RenderPhase &phase);
		void Complete(RenderPhase &phase);

		void Tick();
		bool Active();

		FrameTiming lastCompleteFrame;

	private:
		std::stack<GPUTimeQuery *> stack;
		std::queue<GPUTimeQuery> pending;
		vector<GLuint> queryPool;

		FrameTiming *currentFrame = nullptr;
		std::queue<FrameTiming> pendingFrames;
	};
}