#pragma once

#include "console/CFunc.hh"
#include "ecs/Ecs.hh"
#include "ecs/components/Events.hh"

namespace sp {

    class EditorSystem {
    public:
        EditorSystem();

        void OpenEditor(std::string targetName, bool flatMode = true);
        void ToggleTray();

    private:
        ecs::EntityRef inspectorEntity = ecs::Name("editor", "inspector");
        ecs::Entity previousTarget;

        CFuncCollection funcs;
    };

} // namespace sp
