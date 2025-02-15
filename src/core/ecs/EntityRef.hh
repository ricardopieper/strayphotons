#pragma once

#include "ecs/Ecs.hh"
#include "ecs/components/Name.hh"

#include <memory>
#include <string_view>

namespace ecs {
    class EntityRef {
    private:
        struct Ref;

    public:
        EntityRef() {}
        EntityRef(const Entity &ent);
        EntityRef(const ecs::Name &name, const Entity &ent = Entity());
        EntityRef(const EntityRef &ref) : ptr(ref.ptr) {}
        EntityRef(const std::shared_ptr<Ref> &ptr) : ptr(ptr) {}

        ecs::Name Name() const;
        Entity Get(const Lock<> &lock) const;
        Entity GetLive() const;
        Entity GetStaging() const;

        void SetScope(const EntityScope &scope);

        explicit operator bool() const {
            return !!ptr;
        }

        bool operator==(const EntityRef &other) const;
        bool operator==(const Entity &other) const;
        bool operator<(const EntityRef &other) const;

    private:
        std::shared_ptr<Ref> ptr;

        friend class EntityReferenceManager;
    };
} // namespace ecs
