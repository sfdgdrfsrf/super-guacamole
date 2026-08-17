/**
 * Main.cpp — Plugin entry point for Personalized (LeviLamina native mod).
 *
 * Boot order:
 *   1. Load config from JSON
 *   2. Install UUIDHook (captures player UUID on join)
 *   3. Install TextureSwapHook (remaps block textures)
 *   4. Install InventoryScrambleHook (shuffles creative inventory)
 *   5. Install MobModelSwapHook (swaps mob render models)
 *
 * The UUID hook fires first and seeds the RandomMapper.
 * Other hooks check SeedManager::isInitialized() before acting,
 * so they're inert until the player's UUID is available.
 */

#include "Config.h"
#include "SeedManager.h"
#include "Utils/Logger.h"

#include "Hooks/UUIDHook.h"
#include "Hooks/TextureSwapHook.h"
#include "Hooks/InventoryScrambleHook.h"
#include "Hooks/MobModelSwapHook.h"

#include "ll/api/Logger.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/server/ServerStartingEvent.h"
#include "ll/api/event/server/ServerStoppingEvent.h"

// ─────────────────────────────────────────────
//  LeviLamina plugin registration
// ─────────────────────────────────────────────

// LeviLamina uses a macro to declare the plugin entry point.
// The actual macro name depends on your LeviLamina version:

// Modern (v0.14+):
//   LL_REGISTER_MOD(mod, logger) { ... }
//
// Legacy (LiteLoaderBDS 2.x):
//   LL_REGISTER_PLUGIN(name, desc, version, logger) { ... }

// We use the modern style. Adjust if your fork differs.

class PersonalizedMod {
public:
    static constexpr const char* NAME    = "Personalized";
    static constexpr const char* VERSION = "0.1.0";
    static constexpr const char* DESC    = "UUID-seeded client-side world scrambling (Bedrock port of Personalized Java mod)";

    PersonalizedMod() = default;
    ~PersonalizedMod() = default;

    void onEnable();
    void onDisable();

private:
    personalized::Config m_config;
    bool m_loaded = false;
};

static PersonalizedMod g_mod;

// ─────────────────────────────────────────────
//  onEnable — called when the server starts
// ─────────────────────────────────────────────
void PersonalizedMod::onEnable() {
    PZ_LOG_INFO("╔══════════════════════════════════════════════╗");
    PZ_LOG_INFO("║  Personalized v{} — Bedrock Edition           ║", PersonalizedMod::VERSION);
    PZ_LOG_INFO("║  UUID-seeded world scrambling mod            ║");
    PZ_LOG_INFO("╚══════════════════════════════════════════════╝");

    // ── Step 1: Load configuration ──
    std::string configPath = "plugins/Personalized/config.json";
    PZ_LOG_INFO("Loading config from: {}", configPath);

    if (!personalized::Config::loadFromFile(m_config, configPath)) {
        PZ_LOG_WARN("Using default config values");
        // Save defaults so the user can edit them
        m_config.saveToFile(configPath);
    }

    if (!m_config.enabled) {
        PZ_LOG_INFO("Mod is disabled in config — nothing to install");
        return;
    }

    PZ_LOG_INFO("Config: seedSource={}, dryRun={}", m_config.seedSource, m_config.dryRun);
    PZ_LOG_INFO("Config: textureSwap={}, inventoryScramble={}, mobModelSwap={}",
                m_config.textureSwapEnabled,
                m_config.inventoryScrambleEnabled,
                m_config.mobModelSwapEnabled);

    // ── Step 2: Install UUID capture hook (MUST be first) ──
    PZ_LOG_INFO("[1/4] Installing UUIDHook...");
    personalized::UUIDHook::install();

    // ── Step 3: Install texture swap hook ──
    if (m_config.textureSwapEnabled) {
        PZ_LOG_INFO("[2/4] Installing TextureSwapHook...");
        personalized::TextureSwapHook::install();
    } else {
        PZ_LOG_INFO("[2/4] TextureSwapHook DISABLED by config");
    }

    // ── Step 4: Install inventory scramble hook ──
    if (m_config.inventoryScrambleEnabled) {
        PZ_LOG_INFO("[3/4] Installing InventoryScrambleHook...");
        personalized::InventoryScrambleHook::install();
    } else {
        PZ_LOG_INFO("[3/4] InventoryScrambleHook DISABLED by config");
    }

    // ── Step 5: Install mob model swap hook ──
    if (m_config.mobModelSwapEnabled) {
        PZ_LOG_INFO("[4/4] Installing MobModelSwapHook...");
        personalized::MobModelSwapHook::install();
    } else {
        PZ_LOG_INFO("[4/4] MobModelSwapHook DISABLED by config");
    }

    m_loaded = true;
    PZ_LOG_INFO("All hooks installed — mod active (awaiting UUID seed)");
}

// ─────────────────────────────────────────────
//  onDisable — called on server stop / plugin unload
// ─────────────────────────────────────────────
void PersonalizedMod::onDisable() {
    PZ_LOG_INFO("Shutting down Personalized...");

    personalized::MobModelSwapHook::uninstall();
    personalized::InventoryScrambleHook::uninstall();
    personalized::TextureSwapHook::uninstall();
    personalized::UUIDHook::uninstall();

    m_loaded = false;
    PZ_LOG_INFO("Personalized unloaded — all hooks removed");
}

// ─────────────────────────────────────────────
//  LeviLamina mod registration
// ─────────────────────────────────────────────
// This is the modern LeviLamina registration macro.
// It creates the NativeMod object and wires enable/disable.

/*
 * In your actual build, this typically looks like:
 *
 *   LL_MOD_EXPORT { mod.onEnable(); }
 *   LL_MOD_UNEXPORT { mod.onDisable(); }
 *
 * Or with the newer API:
 *
 *   extern "C" LL_MOD_EXPORT ll::mod::NativeMod* ll_mod_load() {
 *       return &g_mod;  // Return your mod object
 *   }
 */

// ── Placeholder registration using event listeners ──
// This approach works across LeviLamina versions:

namespace {

struct ModRegistrar {
    ModRegistrar() {
        auto& bus = ll::event::EventBus::getInstance();

        // On server start → enable mod
        bus.emplaceListener<ll::event::ServerStartingEvent>(
            [](ll::event::ServerStartingEvent&) {
                PZ_LOG_INFO("ServerStarting event — enabling Personalized");
                g_mod.onEnable();
            }
        );

        // On server stop → disable mod
        bus.emplaceListener<ll::event::ServerStoppingEvent>(
            [](ll::event::ServerStoppingEvent&) {
                PZ_LOG_INFO("ServerStopping event — disabling Personalized");
                g_mod.onDisable();
            }
        );
    }
};

// Static initialization triggers registration before main()
static ModRegistrar s_registrar;

} // anonymous namespace
