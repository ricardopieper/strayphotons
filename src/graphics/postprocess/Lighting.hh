#pragma once

#include "PostProcess.hh"

namespace sp
{
	class Tonemap : public PostProcessPass<1, 1>
	{
	public:
		void Process(const PostProcessingContext *context);

		RenderTargetDesc GetOutputDesc(uint32 id)
		{
			auto desc = GetInput(0)->GetOutput()->TargetDesc;
			desc.format = PF_SRGB8_A8;
			return desc;
		}

		string Name()
		{
			return "Tonemap";
		}
	};

	class LumiHistogram : public PostProcessPass<1, 1>
	{
	public:
		void Process(const PostProcessingContext *context);

		RenderTargetDesc GetOutputDesc(uint32 id)
		{
			return GetInput(0)->GetOutput()->TargetDesc;
		}

		string Name()
		{
			return "LumiHistogram";
		}
	};

	class VoxelLighting : public PostProcessPass<10, 1>
	{
	public:
		void Process(const PostProcessingContext *context);

		RenderTargetDesc GetOutputDesc(uint32 id)
		{
			auto desc = GetInput(0)->GetOutput()->TargetDesc;
			desc.format = PF_RGBA16F;
			return desc;
		}

		string Name()
		{
			return "VoxelLighting";
		}
	};

	class VoxelLightingDiffuse : public PostProcessPass<7, 1>
	{
	public:
		VoxelLightingDiffuse();

		void Process(const PostProcessingContext *context);

		RenderTargetDesc GetOutputDesc(uint32 id)
		{
			auto desc = GetInput(0)->GetOutput()->TargetDesc;
			desc.extent[0] /= downsample;
			desc.extent[1] /= downsample;
			desc.format = PF_RGBA16F;
			return desc;
		}

		string Name()
		{
			return "VoxelLightingDiffuse";
		}

	private:
		int downsample = 1;
	};
}