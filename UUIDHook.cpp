/**
 * UUIDHook.cpp — Hooks the player-join event to capture UUID and seed the mod.
 *
 * Hook strategy:
 *   - Primary:   Listen to LeviLamina's PlayerJoinEvent (event bus)
 *   - Fallback:  Hook ClientInstance::startLeaveGame or Level::addPlayer
 *                to catch the moment a local player becomes available.
 */

#include "UUIDHook.h"
#include "SeedManager.h"
#include "Config.h"
#include "Utils/Logger.h"
#include "Utils/OffsetDefs.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/server/PlayerJoinEvent.h"
#include "mc/world/actor/player/Player.h"

// For raw-function hooks (MemHook macro style)
#include "ll/api/memory/Hook.h"

namespace personalized {

bool UUIDHook::s_hooked = false;
bool UUIDHook::s_ready  = false;

// ─────────────────────────────────────────────
//  Primary: Event-bus listener
// ─────────────────────────────────────────────
void UUIDHook::install() {
    if (s_hooked) {
        PZ_LOG_WARN("UUIDHook already installed, skipping");
        return;
    }

    PZ_LOG_INFO("Installing UUIDHook (player-join event listener)...");

    auto& bus = ll::event::EventBus::getInstance();

    bus.emplaceListener<ll::event::PlayerJoinEvent>([](ll::event::PlayerJoinEvent& ev) {
        if (s_ready) return;  // Already seeded

        PZ_LOG_DEBUG("PlayerJoinEvent fired — attempting UUID capture");

        auto& player = ev.self();
        std::string playerName = player.getName();

        PZ_LOG_INFO("Player '{}' joining — initializing SeedManager", playerName);

        bool ok = SeedManager::instance().initialize();
        if (ok) {
            s_ready = true;
            PZ_LOG_INFO("SeedManager ready — UUID captured, scrambling active");
        } else {
            PZ_LOG_WARN("SeedManager init failed for player '{}' — will retry next join", playerName);
        }
    });

    s_hooked = true;
    PZ_LOG_INFO("UUIDHook installed via event bus");
}

// ─────────────────────────────────────────────
//  Fallback: Raw memory hook on Level::addPlayer
// ─────────────────────────────────────────────
// This is commented out but shows the LL_HOOK_MEMFN pattern you'd use
// if the event-bus approach doesn't fire early enough.

/*
#include "mc/world/level/Level.h"

class Level;
LL_TYPE_INSTANCE_HOOK(
    LevelAddPlayerHook,
    Level,
    off::OFFSET_Level_addPlayer,   // hook at this vtable offset
    void,
    Player& player                 // original signature
) {
    // Call original first
    origin(player);

    if (!s_ready) {
        PZ_LOG_DEBUG("Level::addPlayer caught for '{}'", player.getName());
        bool ok = SeedManager::instance().initialize();
        if (ok) s_ready = true;
    }
}
*/

void UUIDHook::uninstall() {
    // Event bus listeners are auto-cleaned on plugin unload in LeviLamina
    s_hooked = false;
    s_ready  = false;
    PZ_LOG_INFO("UUIDHook uninstalled");
}

bool UUIDHook::isReady() {
    return s_ready;
}

} // namespace personalized
