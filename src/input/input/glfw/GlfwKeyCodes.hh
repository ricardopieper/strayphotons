#pragma once

#include "input/KeyCodes.hh"

#include <GLFW/glfw3.h>
#include <robin_hood.h>

namespace sp {
    static const robin_hood::unordered_flat_map<int, KeyCode> GlfwKeyMapping = {
        {GLFW_KEY_SPACE, KEY_SPACE},
        {GLFW_KEY_APOSTROPHE, KEY_APOSTROPHE},
        {GLFW_KEY_COMMA, KEY_COMMA},
        {GLFW_KEY_MINUS, KEY_MINUS},
        {GLFW_KEY_PERIOD, KEY_PERIOD},
        {GLFW_KEY_SLASH, KEY_SLASH},
        {GLFW_KEY_0, KEY_0},
        {GLFW_KEY_1, KEY_1},
        {GLFW_KEY_2, KEY_2},
        {GLFW_KEY_3, KEY_3},
        {GLFW_KEY_4, KEY_4},
        {GLFW_KEY_5, KEY_5},
        {GLFW_KEY_6, KEY_6},
        {GLFW_KEY_7, KEY_7},
        {GLFW_KEY_8, KEY_8},
        {GLFW_KEY_9, KEY_9},
        {GLFW_KEY_SEMICOLON, KEY_SEMICOLON},
        {GLFW_KEY_EQUAL, KEY_EQUALS},
        {GLFW_KEY_A, KEY_A},
        {GLFW_KEY_B, KEY_B},
        {GLFW_KEY_C, KEY_C},
        {GLFW_KEY_D, KEY_D},
        {GLFW_KEY_E, KEY_E},
        {GLFW_KEY_F, KEY_F},
        {GLFW_KEY_G, KEY_G},
        {GLFW_KEY_H, KEY_H},
        {GLFW_KEY_I, KEY_I},
        {GLFW_KEY_J, KEY_J},
        {GLFW_KEY_K, KEY_K},
        {GLFW_KEY_L, KEY_L},
        {GLFW_KEY_M, KEY_M},
        {GLFW_KEY_N, KEY_N},
        {GLFW_KEY_O, KEY_O},
        {GLFW_KEY_P, KEY_P},
        {GLFW_KEY_Q, KEY_Q},
        {GLFW_KEY_R, KEY_R},
        {GLFW_KEY_S, KEY_S},
        {GLFW_KEY_T, KEY_T},
        {GLFW_KEY_U, KEY_U},
        {GLFW_KEY_V, KEY_V},
        {GLFW_KEY_W, KEY_W},
        {GLFW_KEY_X, KEY_X},
        {GLFW_KEY_Y, KEY_Y},
        {GLFW_KEY_Z, KEY_Z},
        {GLFW_KEY_LEFT_BRACKET, KEY_LEFT_BRACKET},
        {GLFW_KEY_BACKSLASH, KEY_BACKSLASH},
        {GLFW_KEY_RIGHT_BRACKET, KEY_RIGHT_BRACKET},
        {GLFW_KEY_GRAVE_ACCENT, KEY_BACKTICK},
        {GLFW_KEY_ESCAPE, KEY_ESCAPE},
        {GLFW_KEY_ENTER, KEY_ENTER},
        {GLFW_KEY_TAB, KEY_TAB},
        {GLFW_KEY_BACKSPACE, KEY_BACKSPACE},
        {GLFW_KEY_INSERT, KEY_INSERT},
        {GLFW_KEY_DELETE, KEY_DELETE},
        {GLFW_KEY_RIGHT, KEY_RIGHT_ARROW},
        {GLFW_KEY_LEFT, KEY_LEFT_ARROW},
        {GLFW_KEY_DOWN, KEY_DOWN_ARROW},
        {GLFW_KEY_UP, KEY_UP_ARROW},
        {GLFW_KEY_PAGE_UP, KEY_PAGE_UP},
        {GLFW_KEY_PAGE_DOWN, KEY_PAGE_DOWN},
        {GLFW_KEY_HOME, KEY_HOME},
        {GLFW_KEY_END, KEY_END},
        {GLFW_KEY_CAPS_LOCK, KEY_CAPS_LOCK},
        {GLFW_KEY_SCROLL_LOCK, KEY_SCROLL_LOCK},
        {GLFW_KEY_NUM_LOCK, KEY_NUM_LOCK},
        {GLFW_KEY_PRINT_SCREEN, KEY_PRINT_SCREEN},
        {GLFW_KEY_PAUSE, KEY_PAUSE},
        {GLFW_KEY_F1, KEY_F1},
        {GLFW_KEY_F2, KEY_F2},
        {GLFW_KEY_F3, KEY_F3},
        {GLFW_KEY_F4, KEY_F4},
        {GLFW_KEY_F5, KEY_F5},
        {GLFW_KEY_F6, KEY_F6},
        {GLFW_KEY_F7, KEY_F7},
        {GLFW_KEY_F8, KEY_F8},
        {GLFW_KEY_F9, KEY_F9},
        {GLFW_KEY_F10, KEY_F10},
        {GLFW_KEY_F11, KEY_F11},
        {GLFW_KEY_F12, KEY_F12},
        {GLFW_KEY_F13, KEY_F13},
        {GLFW_KEY_F14, KEY_F14},
        {GLFW_KEY_F15, KEY_F15},
        {GLFW_KEY_F16, KEY_F16},
        {GLFW_KEY_F17, KEY_F17},
        {GLFW_KEY_F18, KEY_F18},
        {GLFW_KEY_F19, KEY_F19},
        {GLFW_KEY_F20, KEY_F20},
        {GLFW_KEY_F21, KEY_F21},
        {GLFW_KEY_F22, KEY_F22},
        {GLFW_KEY_F23, KEY_F23},
        {GLFW_KEY_F24, KEY_F24},
        {GLFW_KEY_F25, KEY_F25},
        {GLFW_KEY_KP_0, KEY_0_NUMPAD},
        {GLFW_KEY_KP_1, KEY_1_NUMPAD},
        {GLFW_KEY_KP_2, KEY_2_NUMPAD},
        {GLFW_KEY_KP_3, KEY_3_NUMPAD},
        {GLFW_KEY_KP_4, KEY_4_NUMPAD},
        {GLFW_KEY_KP_5, KEY_5_NUMPAD},
        {GLFW_KEY_KP_6, KEY_6_NUMPAD},
        {GLFW_KEY_KP_7, KEY_7_NUMPAD},
        {GLFW_KEY_KP_8, KEY_8_NUMPAD},
        {GLFW_KEY_KP_9, KEY_9_NUMPAD},
        {GLFW_KEY_KP_DECIMAL, KEY_DECIMAL_NUMPAD},
        {GLFW_KEY_KP_DIVIDE, KEY_DIVIDE_NUMPAD},
        {GLFW_KEY_KP_MULTIPLY, KEY_MULTIPLY_NUMPAD},
        {GLFW_KEY_KP_SUBTRACT, KEY_MINUS_NUMPAD},
        {GLFW_KEY_KP_ADD, KEY_PLUS_NUMPAD},
        {GLFW_KEY_KP_ENTER, KEY_ENTER_NUMPAD},
        {GLFW_KEY_KP_EQUAL, KEY_EQUALS_NUMPAD},
        {GLFW_KEY_LEFT_SHIFT, KEY_LEFT_SHIFT},
        {GLFW_KEY_LEFT_CONTROL, KEY_LEFT_CONTROL},
        {GLFW_KEY_LEFT_ALT, KEY_LEFT_ALT},
        {GLFW_KEY_LEFT_SUPER, KEY_LEFT_SUPER},
        {GLFW_KEY_RIGHT_SHIFT, KEY_RIGHT_SHIFT},
        {GLFW_KEY_RIGHT_CONTROL, KEY_RIGHT_CONTROL},
        {GLFW_KEY_RIGHT_ALT, KEY_RIGHT_ALT},
        {GLFW_KEY_RIGHT_SUPER, KEY_RIGHT_SUPER},
        {GLFW_KEY_MENU, KEY_CONTEXT_MENU},
    };
} // namespace sp
