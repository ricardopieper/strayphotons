#ifdef _WIN32
    #include <windows.h>
#endif

#include <iostream>
using namespace std;

#include "Game.hh"
#include "core/Logging.hh"

#ifdef SP_TEST_MODE
    #include "assets/AssetManager.hh"
    #include "assets/ConsoleScript.hh"
#endif

#include <csignal>
#include <cstdio>
#include <cxxopts.hpp>
#include <filesystem>
#include <memory>

using cxxopts::value;

namespace sp {
    void handleSignals(int signal) {
        if (signal == SIGINT) {
            gameExitTriggered.test_and_set();
            gameExitTriggered.notify_all();
        }
    }
} // namespace sp

// TODO: Commented until package release saves a log file
// #if defined(_WIN32) && defined(SP_PACKAGE_RELEASE)
//     #define ARGC_NAME __argc
//     #define ARGV_NAME __argv
// int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
// #else
#define ARGC_NAME argc
#define ARGV_NAME argv
int main(int argc, char **argv)
// #endif
{
#ifdef SP_TEST_MODE
    cxxopts::Options options("STRAYPHOTONS-TEST", "");
    options.positional_help("/path/to/script.txt");
#else
    cxxopts::Options options("STRAYPHOTONS", "");
#endif

    // clang-format off
    options.add_options()
        ("h,help", "Display help")
        ("m,map", "Initial scene to load", value<string>())
        ("size", "Initial window size", value<string>())
#ifdef SP_TEST_MODE
        ("script-file", "", value<string>())
#endif
#ifdef SP_XR_SUPPORT
        ("no-vr", "Disable automatic XR/VR system loading")
#endif
#ifdef SP_GRAPHICS_SUPPORT
        ("headless", "Disable window creation and graphics initialization")
#endif
#ifdef SP_GRAPHICS_SUPPORT_VK
        ("with-validation-layers", "Enable Vulkan validation layers")
#endif
        ("c,cvar", "Set cvar to initial value", value<vector<string>>());
    // clang-format on

#ifdef SP_TEST_MODE
    options.parse_positional({"script-file"});
#endif

#ifdef _WIN32
    // Increase thread scheduler resolution from default of 15ms
    timeBeginPeriod(1);
    std::shared_ptr<UINT> timePeriodReset(new UINT(1), [](UINT *period) {
        timeEndPeriod(*period);
        delete period;
    });

    signal(SIGINT, sp::handleSignals);
#else
    struct sigaction act;
    act.sa_handler = sp::handleSignals;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);
#endif

#ifdef CATCH_GLOBAL_EXCEPTIONS
    try
#endif
    {
        auto optionsResult = options.parse(ARGC_NAME, ARGV_NAME);

        if (optionsResult.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

#ifdef SP_TEST_MODE
        if (!optionsResult.count("script-file")) {
            Logf("Script file required argument.");
            return 0;
        }
#endif

        Logf("Starting in directory: %s", std::filesystem::current_path().string());

#ifdef SP_TEST_MODE
        string scriptPath = optionsResult["script-file"].as<string>();

        Logf("Loading test script: %s", scriptPath);
        auto asset = sp::Assets().Load("scripts/" + scriptPath)->Get();
        if (!asset) {
            Errorf("Test script not found: %s", scriptPath);
            return 0;
        }

        sp::ConsoleScript script(scriptPath, asset);
        sp::Game game(optionsResult, &script);
        return game.Start();
#else
        sp::Game game(optionsResult);
        return game.Start();
#endif
    }
#ifdef CATCH_GLOBAL_EXCEPTIONS
    catch (const char *err) {
        Errorf("terminating with exception: %s", err);
    } catch (const std::exception &ex) {
        Errorf("terminating with exception: %s", ex.what());
    }
#endif
    return -1;
}
