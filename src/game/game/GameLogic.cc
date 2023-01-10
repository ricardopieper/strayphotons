#include "GameLogic.hh"

#include "console/Console.hh"
#include "core/Tracing.hh"
#include "ecs/EcsImpl.hh"

namespace sp {
    GameLogic::GameLogic(bool stepMode) : RegisteredThread("GameLogic", 120.0, true), stepMode(stepMode) {
        if (stepMode) {
            funcs.Register<unsigned int>("steplogic",
                "Advance the game logic by N frames, default is 1",
                [this](unsigned int arg) {
                    this->Step(std::max(1u, arg));
                });
        }
    }

    void GameLogic::StartThread() {
        RegisteredThread::StartThread(stepMode);
    }

    void GameLogic::Frame() {
        ZoneScoped;
        {
            auto lock = ecs::StartTransaction<ecs::Write<ecs::Scripts, ecs::EventInput>>();

            for (auto &entity : lock.EntitiesWith<ecs::Scripts>()) {
                if (!entity.Has<ecs::Scripts, ecs::EventInput>(lock)) continue;
                auto &readScripts = entity.Get<const ecs::Scripts>(lock);
                for (size_t i = 0; i < readScripts.scripts.size(); i++) {
                    auto &readState = readScripts.scripts[i];
                    if (!readState.definition.events.empty() && !readState.eventQueue) {
                        auto &eventInput = entity.Get<ecs::EventInput>(lock);
                        auto &writeScripts = entity.Get<ecs::Scripts>(lock);
                        auto &writeState = writeScripts.scripts[i];
                        writeState.eventQueue = ecs::NewEventQueue();
                        for (auto &event : writeState.definition.events) {
                            eventInput.Register(lock, writeState.eventQueue, event);
                        }
                    }
                }
            }
        }
        {
            // TODO: Change this to exclude Write<EventInput>
            auto lock = ecs::StartTransaction<ecs::WriteAll>();
            for (auto &entity : lock.EntitiesWith<ecs::Scripts>()) {
                auto &scripts = entity.Get<ecs::Scripts>(lock);
                scripts.OnTick(lock, entity, interval);
            }
        }
    }
} // namespace sp
