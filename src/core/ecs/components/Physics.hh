#pragma once

#include "assets/Async.hh"
#include "ecs/Components.hh"
#include "ecs/Ecs.hh"
#include "ecs/components/Transform.h"

#include <glm/glm.hpp>
#include <memory>

namespace sp {
    class Gltf;
}

namespace physx {
    class PxRigidActor;
    class PxJoint;
    class PxScene;
    class PxControllerManager;
} // namespace physx

namespace ecs {
    enum class PhysicsGroup : uint16_t {
        NoClip = 0,
        World,
        Interactive,
        Player,
        PlayerHands,
        Count,
    };

    enum PhysicsGroupMask {
        PHYSICS_GROUP_NOCLIP = 1 << (size_t)PhysicsGroup::NoClip,
        PHYSICS_GROUP_WORLD = 1 << (size_t)PhysicsGroup::World,
        PHYSICS_GROUP_INTERACTIVE = 1 << (size_t)PhysicsGroup::Interactive,
        PHYSICS_GROUP_PLAYER = 1 << (size_t)PhysicsGroup::Player,
        PHYSICS_GROUP_PLAYER_HANDS = 1 << (size_t)PhysicsGroup::PlayerHands,
    };

    enum class PhysicsJointType {
        Fixed = 0,
        Distance,
        Spherical,
        Hinge,
        Slider,
        Count,
    };

    struct PhysicsShape {
        struct Sphere {
            float radius;
            Sphere(float radius = 1.0f) : radius(radius) {}
        };

        struct Capsule {
            float radius;
            float height;
            Capsule(float height = 1.0f, float radius = 0.5f) : radius(radius), height(height) {}
        };

        struct Box {
            glm::vec3 extents;
            Box(glm::vec3 extents = glm::vec3(1)) : extents(extents) {}
        };

        struct Plane {};

        struct ConvexMesh {
            sp::AsyncPtr<sp::Gltf> model;
            size_t meshIndex = 0;

            ConvexMesh() {}
            ConvexMesh(sp::AsyncPtr<sp::Gltf> model, size_t meshIndex = 0) : model(model), meshIndex(meshIndex) {}
        };

        std::variant<std::monostate, Sphere, Capsule, Box, Plane, ConvexMesh> shape;

        PhysicsShape() : shape(std::monostate()) {}
        PhysicsShape(Sphere sphere) : shape(sphere) {}
        PhysicsShape(Capsule capsule) : shape(capsule) {}
        PhysicsShape(Box box) : shape(box) {}
        PhysicsShape(Plane plane) : shape(plane) {}
        PhysicsShape(ConvexMesh mesh) : shape(mesh) {}
        PhysicsShape(sp::AsyncPtr<sp::Gltf> model, size_t meshIndex = 0) : shape(ConvexMesh(model, meshIndex)) {}

        operator bool() const {
            return !std::holds_alternative<std::monostate>(shape);
        }
    };

    struct Physics {
        Physics() {}
        Physics(PhysicsShape shape, PhysicsGroup group = PhysicsGroup::World, bool dynamic = true, float density = 1.0f)
            : shape(shape), group(group), dynamic(dynamic), density(density) {}

        PhysicsShape shape;
        Transform shapeTransform;

        PhysicsGroup group = PhysicsGroup::World;
        bool dynamic = true;
        bool kinematic = false; // only dynamic actors can be kinematic
        bool decomposeHull = false;
        float density = 1.0f;
        float angularDamping = 0.05f;
        float linearDamping = 0.0f;

        Entity jointTarget;
        PhysicsJointType jointType = PhysicsJointType::Count;
        glm::vec2 jointRange;
        glm::vec3 jointLocalOffset, jointRemoteOffset;
        glm::quat jointLocalOrient, jointRemoteOrient;

        glm::vec3 constantForce;

        Entity constraint;
        float constraintMaxDistance = 0.0f;
        glm::vec3 constraintOffset;
        glm::quat constraintRotation;

        void SetJoint(Entity target,
            PhysicsJointType type,
            glm::vec2 range = glm::vec2(),
            glm::vec3 localOffset = glm::vec3(),
            glm::quat localOrient = glm::quat(),
            glm::vec3 remoteOffset = glm::vec3(),
            glm::quat remoteOrient = glm::quat()) {
            jointTarget = target;
            jointType = type;
            jointRange = range;
            jointLocalOffset = localOffset;
            jointRemoteOffset = remoteOffset;
            jointLocalOrient = localOrient;
            jointRemoteOrient = remoteOrient;
        }

        void RemoveJoint() {
            SetJoint(Entity(), PhysicsJointType::Count);
        }

        void SetConstraint(Entity target,
            float maxDistance = 0.0f,
            glm::vec3 offset = glm::vec3(),
            glm::quat rotation = glm::quat()) {
            constraint = target;
            constraintMaxDistance = maxDistance;
            constraintOffset = offset;
            constraintRotation = rotation;
        }

        void RemoveConstraint() {
            SetConstraint(Entity());
        }
    };

    struct PhysicsQuery {
        // Raycast query inputs
        float raycastQueryDistance = 0.0f;
        PhysicsGroupMask raycastQueryFilterGroup = PHYSICS_GROUP_WORLD;
        // Raycast outputs
        Entity raycastHitTarget;
        glm::vec3 raycastHitPosition;
        float raycastHitDistance = 0.0f;

        // Center of mass query
        Entity centerOfMassQuery;
        // The calculated center of mass of the object (relative to its Transform)
        glm::vec3 centerOfMass;
    };

    static Component<Physics> ComponentPhysics("physics");
    static Component<PhysicsQuery> ComponentPhysicsQuery("physics_query");

    template<>
    bool Component<Physics>::Load(ScenePtr scenePtr, Physics &dst, const picojson::value &src);
    template<>
    bool Component<PhysicsQuery>::Load(ScenePtr scenePtr, PhysicsQuery &dst, const picojson::value &src);
    template<>
    void Component<Physics>::ApplyComponent(Lock<ReadAll> srcLock, Entity src, Lock<AddRemove> dstLock, Entity dst);
} // namespace ecs
