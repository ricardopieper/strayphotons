#pragma once

#include "Common.hh"
#include "graphics/Graphics.hh"
#include "graphics/Texture.hh"
#include "graphics/Buffer.hh"
#include "graphics/VertexBuffer.hh"

#include <array>
#include <tiny_gltf_loader.h>

namespace sp
{
	class Asset;
	class GLModel;

	typedef std::array<uint32, 4> Hash128;

	class Model : public NonCopyable
	{
		friend GLModel;
	public:
		Model(const string &name) : name(name) { }
		Model(const string &name, shared_ptr<Asset> asset, shared_ptr<tinygltf::Scene> scene);
		virtual ~Model();

		struct Attribute
		{
			size_t byteOffset, byteStride;
			int componentType;
			size_t componentCount;
			size_t components;
			string bufferName;
		};

		struct Primitive
		{
			glm::mat4 matrix;
			int drawMode;
			Attribute indexBuffer;
			string materialName;
			Attribute attributes[3];
		};

		const string name;
		shared_ptr<GLModel> glModel;
		vector<Primitive *> primitives;

		bool HasBuffer(string name);
		vector<unsigned char> GetBuffer(string name);
		Hash128 HashBuffer(string name);

	private:
		void AddNode(string nodeName, glm::mat4 parentMatrix);

		shared_ptr<tinygltf::Scene> scene;
		shared_ptr<Asset> asset;
	};

	class GLModel : public NonCopyable
	{
	public:
		GLModel(Model *model);
		~GLModel();

		struct Primitive
		{
			Model::Primitive *parent;
			GLuint vertexBufferHandle;
			GLuint indexBufferHandle;
			Texture *baseColorTex, *roughnessTex, *metallicTex, *heightTex;
		};

		void Draw();
		void AddPrimitive(Primitive prim);
	private:
		GLuint LoadBuffer(string name);
		Texture *LoadTexture(string materialName, string type);

		Model *model;
		std::map<string, GLuint> buffers;
		std::map<string, Texture> textures;
		vector<Primitive> primitives;
	};

	struct BasicMaterial
	{
		Texture baseColorTex, roughnessTex, metallicTex, heightTex;

		BasicMaterial(
			unsigned char *baseColor = nullptr,
			unsigned char *roughness = nullptr,
			unsigned char *metallic = nullptr,
			unsigned char *bump = nullptr)
		{
			unsigned char baseColorDefault[4] = { 255, 255, 255, 255 };
			unsigned char roughnessDefault[4] = { 255, 255, 255, 255 };
			unsigned char metallicDefault[4] = { 0, 0, 0, 0 };
			unsigned char bumpDefault[4] = { 127, 127, 127, 255 };

			if (!baseColor) baseColor = baseColorDefault;
			if (!roughness) roughness = roughnessDefault;
			if (!metallic) metallic = metallicDefault;
			if (!bump) bump = bumpDefault;

			baseColorTex.Create()
			.Filter(GL_NEAREST, GL_NEAREST).Wrap(GL_REPEAT, GL_REPEAT)
			.Size(1, 1).Storage(PF_RGB8).Image2D(baseColor);

			roughnessTex.Create()
			.Filter(GL_NEAREST, GL_NEAREST).Wrap(GL_REPEAT, GL_REPEAT)
			.Size(1, 1).Storage(PF_R8).Image2D(roughness);

			metallicTex.Create()
			.Filter(GL_NEAREST, GL_NEAREST).Wrap(GL_REPEAT, GL_REPEAT)
			.Size(1, 1).Storage(PF_R8).Image2D(metallic);

			heightTex.Create()
			.Filter(GL_NEAREST, GL_NEAREST).Wrap(GL_REPEAT, GL_REPEAT)
			.Size(1, 1).Storage(PF_R8).Image2D(bump);
		}
	};
}

#ifdef ENABLE_VR
namespace vr
{
	class RenderModel_t;
	class RenderModel_TextureMap_t;
}

namespace sp
{
	class VRModel : public Model
	{
	public:
		VRModel(vr::RenderModel_t *vrModel, vr::RenderModel_TextureMap_t *vrTex);
		virtual ~VRModel();

	private:
		Texture baseColorTex, roughnessTex, metallicTex, heightTex;
		VertexBuffer vbo;
		Buffer ibo;
		Model::Primitive sourcePrim;
	};
}
#endif