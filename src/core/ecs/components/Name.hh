#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <string_view>

namespace sp {
    class Scene;
}

namespace ecs {
    struct Name;
    using EntityScope = Name;

    struct Name {
        std::string scene, entity;

        Name() {}
        Name(const std::string_view &scene, const std::string_view &entity);
        Name(const std::string_view &relativeName, const EntityScope &scope);
        Name(const Name &other, const EntityScope &scope);

        bool Parse(const std::string_view &relativeName, const EntityScope &scope);

        std::string String() const {
            if (scene.empty()) return entity;
            return scene + ":" + entity;
        }

        explicit operator bool() const {
            return !entity.empty();
        }

        bool operator==(const Name &) const = default;
        bool operator<(const Name &other) const {
            return scene == other.scene ? entity < other.entity : scene < other.scene;
        }
    };

    std::ostream &operator<<(std::ostream &out, const Name &v);
} // namespace ecs

namespace std {
    template<>
    struct hash<ecs::Name> {
        std::size_t operator()(const ecs::Name &n) const;
    };
} // namespace std
