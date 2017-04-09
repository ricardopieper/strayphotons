#pragma once

#include "Common.hh"
#include "Texture.hh"
#include "RenderBuffer.hh"
#include <glm/glm.hpp>

namespace sp
{
	struct RenderTargetDesc
	{
		RenderTargetDesc() {}

		RenderTargetDesc(PixelFormat format, glm::ivec3 extent) :
			format(format), extent(extent), attachment(GL_COLOR_ATTACHMENT0)
		{
			auto glformat = GLPixelFormat::PixelFormatMapping(format).format;

			// Try to deduce attachment point.
			// The attachment can be controlled manually via the other constructor.
			if (glformat == GL_DEPTH_COMPONENT)
			{
				attachment = GL_DEPTH_ATTACHMENT;
			}
			else if (glformat == GL_DEPTH_STENCIL)
			{
				attachment = GL_DEPTH_STENCIL_ATTACHMENT;
			}
			else if (glformat == GL_STENCIL_INDEX)
			{
				attachment = GL_STENCIL_ATTACHMENT;
			}
		}

		RenderTargetDesc(PixelFormat format, glm::ivec2 extent) :
			RenderTargetDesc(format, glm::ivec3(extent.x, extent.y, 1)) {}

		RenderTargetDesc(PixelFormat format, glm::ivec2 extent, GLenum attachment) :
			format(format), extent(extent.x, extent.y, 1), attachment(attachment) {}

		RenderTargetDesc(PixelFormat format, glm::ivec2 extent, bool renderBuffer) :
			RenderTargetDesc(format, glm::ivec3(extent.x, extent.y, 1))
		{
			this->renderBuffer = renderBuffer;
		}

		RenderTargetDesc &Filter(GLenum minf, GLenum magf)
		{
			minFilter = minf;
			magFilter = magf;
			return *this;
		}

		PixelFormat format;
		glm::ivec3 extent = { 0, 0, 0 };
		uint32 levels = 1;
		bool depthCompare = false;
		bool multiSample = false;
		bool textureArray = false;
		bool renderBuffer = false;
		GLenum attachment;
		GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR, magFilter = GL_LINEAR;
		GLenum wrapS = GL_CLAMP_TO_EDGE, wrapT = GL_CLAMP_TO_EDGE, wrapR = GL_CLAMP_TO_EDGE;
		glm::vec4 borderColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		float anisotropy = 0.0;

		bool operator==(const RenderTargetDesc &other) const
		{
			return other.format == format
				   && other.extent == extent
				   && other.levels == levels
				   && other.attachment == attachment
				   && other.minFilter == minFilter
				   && other.magFilter == magFilter
				   && other.depthCompare == depthCompare
				   && other.multiSample == multiSample
				   && other.textureArray == textureArray
				   && other.renderBuffer == renderBuffer
				   && other.wrapS == wrapS
				   && other.wrapT == wrapT
				   && other.wrapR == wrapR
				   && other.borderColor == borderColor
				   && other.anisotropy == anisotropy;
		}

		bool operator!=(const RenderTargetDesc &other) const
		{
			return !(*this == other);
		}

	};

	class RenderTarget
	{
	public:
		typedef shared_ptr<RenderTarget> Ref;

		RenderTarget(RenderTargetDesc desc);
		~RenderTarget();

		int64 GetID()
		{
			return id;
		}

		Texture &GetTexture()
		{
			Assert(id >= 0, "render target destroyed");
			Assert(tex.handle, "target is a renderbuffer");
			return tex;
		}

		RenderBuffer &GetRenderBuffer()
		{
			Assert(id >= 0, "render target destroyed");
			Assert(buf.handle, "target is a texture");
			return buf;
		}

		GLuint GetHandle()
		{
			Assert(id >= 0, "render target destroyed");
			Assert(tex.handle || buf.handle, "render target must have an underlying target");
			if (tex.handle) return tex.handle;
			return buf.handle;
		}

		RenderTargetDesc GetDesc()
		{
			return desc;
		}

		bool operator==(const RenderTarget &other) const
		{
			return other.desc == desc
				   && other.tex == tex
				   && other.buf == buf;
		}

		bool operator!=(const RenderTarget &other) const
		{
			return !(*this == other);
		}

	private:
		RenderTargetDesc desc;
		int64 id;
		Texture tex;
		RenderBuffer buf;

		int unusedFrames = 0;
		friend class RenderTargetPool;
	};
}
