#pragma once

#include "ecs/Ecs.hh"
#include "ecs/EntityRef.hh"
#include "ecs/EventQueue.hh"

#include <glm/glm.hpp>

struct GLFWwindow;

namespace sp {
    class GlfwInputHandler {
    public:
        GlfwInputHandler(GLFWwindow &window);
        ~GlfwInputHandler();

        void Frame();

        glm::vec2 ImmediateCursor() const;

        static void KeyInputCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
        static void CharInputCallback(GLFWwindow *window, unsigned int ch);
        static void MouseMoveCallback(GLFWwindow *window, double xPos, double yPos);
        static void MouseButtonCallback(GLFWwindow *window, int button, int actions, int mods);
        static void MouseScrollCallback(GLFWwindow *window, double xOffset, double yOffset);

    private:
        GLFWwindow *window = nullptr;

        ecs::EntityRef keyboardEntity = ecs::Name("input", "keyboard");
        ecs::EntityRef mouseEntity = ecs::Name("input", "mouse");

        ecs::EventQueue glfwEventQueue;
        glm::vec2 prevMousePos;
    };
} // namespace sp
