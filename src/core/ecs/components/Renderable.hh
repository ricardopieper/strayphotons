#pragma once

#include "assets/Async.hh"
#include "core/Common.hh"
#include "core/EnumTypes.hh"
#include "ecs/Components.hh"
#include "ecs/EntityRef.hh"

#include <glm/glm.hpp>

namespace sp {
    class Gltf;
}

namespace ecs {
    enum class VisibilityMask;
}

template<>
struct magic_enum::customize::enum_range<ecs::VisibilityMask> {
    static constexpr bool is_flags = true;
};

namespace ecs {
    enum class VisibilityMask {
        None = 0,
        DirectCamera = 1 << 0,
        DirectEye = 1 << 1,
        Transparent = 1 << 2,
        LightingShadow = 1 << 3,
        LightingVoxel = 1 << 4,
        Optics = 1 << 5,
        OutlineSelection = 1 << 6,
    };

    struct Renderable {
        Renderable() {}
        Renderable(const std::string &modelName, size_t meshIndex = 0);
        Renderable(const std::string &modelName, sp::AsyncPtr<sp::Gltf> model, size_t meshIndex = 0)
            : modelName(modelName), model(model), meshIndex(meshIndex) {}

        std::string modelName;
        sp::AsyncPtr<sp::Gltf> model;
        size_t meshIndex = 0;

        struct Joint {
            EntityRef entity;
            glm::mat4 inverseBindPose;
        };
        vector<Joint> joints; // list of entities corresponding to the "joints" array of the skin

        VisibilityMask visibility = VisibilityMask::DirectCamera | VisibilityMask::DirectEye |
                                    VisibilityMask::LightingShadow | VisibilityMask::LightingVoxel;
        float emissiveScale = 0;
        sp::color_alpha_t colorOverride = glm::vec4(-1);
        glm::vec2 metallicRoughnessOverride = glm::vec2(-1);

        bool IsVisible(VisibilityMask viewMask) const {
            return (visibility & viewMask) == viewMask;
        }
    };

    static StructMetadata MetadataRenderable(typeid(Renderable),
        StructField::New("model", &Renderable::modelName),
        StructField::New("mesh_index", &Renderable::meshIndex),
        StructField::New("visibility", &Renderable::visibility),
        StructField::New("emissive", &Renderable::emissiveScale),
        StructField::New("color_override", &Renderable::colorOverride),
        StructField::New("metallic_roughness_override", &Renderable::metallicRoughnessOverride));
    static Component<Renderable> ComponentRenderable("renderable", MetadataRenderable);

    template<>
    bool StructMetadata::Load<Renderable>(Renderable &dst, const picojson::value &src);
    template<>
    void Component<Renderable>::Apply(Renderable &dst, const Renderable &src, bool liveTarget);
} // namespace ecs
