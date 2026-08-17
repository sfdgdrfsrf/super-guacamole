#pragma once

/**
 * UUIDHook.h — Startup hook to capture the local player UUID.
 *
 * This is the FIRST hook that fires. It hooks into the player-join
 * event (or ClientInstance initialization) to seed the RandomMapper
 * before any other hook activates.
 */

#include <functional>

namespace personalized {

class UUIDHook {
public:
    /// Install the hook. Call once during plugin load.
    static void install();

    /// Unhook (called on plugin unload)
    static void uninstall();

    /// Returns true once the UUID has been captured and SeedManager is ready
    static bool isReady();

private:
    static bool s_hooked;
    static bool s_ready;
};

} // namespace personalized
