#ifdef SP_XR_SUPPORT

    #include "XrManager.hh"

    #include "core/Logging.hh"
    #include "main/Game.hh"

    #ifdef SP_XR_SUPPORT_OPENVR
        #include "xr/openvr/OpenVrSystem.hh"
    #endif

namespace sp::xr {
    XrManager::XrManager(Game *game) : game(game) {
        funcs.Register(this, "reloadxrsystem", "Reload the state of the XR subsystem", &XrManager::LoadXrSystem);
    }

    void XrManager::LoadXrSystem() {
        std::lock_guard lock(xrLoadMutex);

        {
            // ensure old system shuts down before initializing a new one
            auto oldSystem = xrSystem;
            xrSystem.reset();
            while (oldSystem.use_count() > 1)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

    #ifdef SP_XR_SUPPORT_OPENVR
        xrSystem = std::make_shared<OpenVrSystem>();
    #else
        Abort("No XR system defined");
    #endif

        if (!xrSystem->IsHmdPresent()) {
            Logf("No VR HMD is present.");
            return;
        }

        xrSystem->Initialize(game->graphics.GetContext());
    }

    std::shared_ptr<XrSystem> XrManager::GetXrSystem() {
        std::lock_guard lock(xrLoadMutex);
        if (xrSystem && xrSystem->IsInitialized()) return xrSystem;
        return nullptr;
    }
} // namespace sp::xr

#endif
