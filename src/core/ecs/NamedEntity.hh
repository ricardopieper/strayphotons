#pragma once

#include <ecs/Ecs.hh>
#include <string>

namespace ecs {
    class NamedEntity {
    public:
        NamedEntity() {}
        NamedEntity(const std::string &name, Entity ent = Entity()) : name(name), ent(ent) {}

        const std::string &Name() const {
            return name;
        }

        const Entity &Get(Lock<Read<ecs::Name>> lock);

        Entity Get(Lock<Read<ecs::Name>> lock) const;

        bool operator==(const NamedEntity &other) const {
            return !name.empty() && name == other.name;
        }

        bool operator!=(const NamedEntity &other) const {
            return !(*this == other);
        }

        bool operator==(const std::string &other) const {
            return !name.empty() && name == other;
        }

        bool operator!=(const std::string &other) const {
            return !(*this == other);
        }

        bool operator==(const char *other) const {
            return !name.empty() && name == other;
        }

        bool operator!=(const char *other) const {
            return !(*this == other);
        }

    private:
        std::string name;
        Entity ent;
    };
} // namespace ecs

static inline std::ostream &operator<<(std::ostream &out, const ecs::NamedEntity &v) {
    return out << "NamedEntity(" << v.Name() << ")";
}
