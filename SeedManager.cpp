/**
 * SeedManager.cpp — Implementation of UUID fetching and seed derivation.
 *
 * IMPORTANT: The Bedrock offset calls are PLACEHOLDERS. You must fill in
 * the correct offsets from OffsetDefs.h after mapping your target BDS version.
 */

#include "SeedManager.h"
#include "Config.h"
#include "Utils/Logger.h"
#include "Utils/OffsetDefs.h"

#include "mc/world/level/Level.h"
#include "mc/world/actor/player/LocalPlayer.h"
#include "mc/client/services/ClientInstance.h"

#include <functional>
#include <sstream>

namespace personalized {

// ─────────────────────────────────────────────
//  Singleton
// ─────────────────────────────────────────────
SeedManager& SeedManager::instance() {
    static SeedManager inst;
    return inst;
}

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────
bool SeedManager::initialize() {
    std::lock_guard lock(m_mutex);

    if (m_initialized) {
        PZ_LOG_DEBUG("SeedManager already initialized (seed=0x{:016X}), skipping", m_seed);
        return true;
    }

    const auto& cfg = Config{};  // In practice, pass the loaded config
    PZ_LOG_INFO("SeedManager initializing — source: {}", cfg.seedSource);

    std::optional<std::string> uuidOpt;

    if (cfg.seedSource == "uuid") {
        uuidOpt = fetchPlayerUUID();
        if (!uuidOpt.has_value()) {
            PZ_LOG_WARN("Player UUID not available yet, falling back to device ID");
            uuidOpt = fetchDeviceID();
        }
    } else if (cfg.seedSource == "device") {
        uuidOpt = fetchDeviceID();
    } else if (cfg.seedSource == "fixed") {
        m_seed = cfg.fixedSeed;
        m_uuidString = "FIXED_SEED";
        m_initialized = true;
        PZ_LOG_INFO("Using fixed seed: 0x{:016X}", m_seed);
        return true;
    }

    if (!uuidOpt.has_value()) {
        PZ_LOG_ERROR("Failed to obtain any UUID/device ID — mod cannot scramble!");
        return false;
    }

    m_uuidString = uuidOpt.value();
    m_seed = deriveSeedFromUUID(m_uuidString);
    m_initialized = true;

    PZ_LOG_INFO("Seed derived successfully — UUID: {}, Seed: 0x{:016X}", m_uuidString, m_seed);
    return true;
}

bool SeedManager::isInitialized() const {
    std::lock_guard lock(m_mutex);
    return m_initialized;
}

const std::string& SeedManager::getUUIDString() const {
    std::lock_guard lock(m_mutex);
    return m_uuidString;
}

uint64_t SeedManager::getSeed() const {
    std::lock_guard lock(m_mutex);
    return m_seed;
}

std::mt19937_64 SeedManager::createRNG() const {
    std::lock_guard lock(m_mutex);
    PZ_LOG_TRACE("Creating new seeded RNG from seed 0x{:016X}", m_seed);
    return std::mt19937_64(m_seed);
}

void SeedManager::setSeedOverride(uint64_t seed) {
    std::lock_guard lock(m_mutex);
    m_seed = seed;
    m_uuidString = "OVERRIDE_0x" + std::to_string(seed);
    m_initialized = true;
    PZ_LOG_INFO("Seed override applied: 0x{:016X}", seed);
}

// ─────────────────────────────────────────────
//  Internal: UUID fetch from LocalPlayer
// ─────────────────────────────────────────────
std::optional<std::string> SeedManager::fetchPlayerUUID() {
    PZ_LOG_DEBUG("Attempting to fetch LocalPlayer UUID...");

    try {
        /*
         * Strategy:
         *   1. Get ClientInstance singleton
         *   2. Call ClientInstance::getLocalPlayer()
         *   3. Call Player::getOrCreateUniqueID() → mce::UUID
         *   4. Convert to string
         *
         * In LeviLamina, you can often use the high-level API:
         *   Level::getLocalPlayer() or Global<Level>->getLocalPlayer()
         *
         * The low-level path (raw offset calls) is shown below as
         * a fallback / reference for when the typed API isn't available.
         */

        // ── High-level path (preferred when headers are complete) ──
        // auto* level = ll::service::getLevel();
        // if (!level) return std::nullopt;
        // auto* player = level->getLocalPlayer();
        // if (!player) return std::nullopt;
        // mce::UUID uuid = player->getOrCreateUniqueID();
        // return uuid.asString();

        // ── Low-level path (placeholder offsets) ──
        // This is the raw-pointer arithmetic version you'd use if
        // the LeviLamina typed headers don't expose the method yet.

        uintptr_t clientInstBase = *reinterpret_cast<uintptr_t*>(
            off::OFFSET_ClientInstance_singleton
        );

        if (clientInstBase == 0) {
            PZ_LOG_WARN("ClientInstance singleton is null — player not loaded yet?");
            return std::nullopt;
        }

        // Call getLocalPlayer vfunc
        using Fn_GetLocalPlayer = void*(*)(void*);
        auto fnGetLocalPlayer = reinterpret_cast<Fn_GetLocalPlayer>(
            clientInstBase + off::OFFSET_ClientInstance_getLocalPlayer
        );

        void* localPlayer = fnGetLocalPlayer(reinterpret_cast<void*>(clientInstBase));
        if (!localPlayer) {
            PZ_LOG_WARN("LocalPlayer pointer is null");
            return std::nullopt;
        }

        PZ_LOG_TRACE("LocalPlayer at {:p}", localPlayer);

        // Call getOrCreateUniqueID
        using Fn_GetUUID = void(*)(void*, void* /*out UUID*/);
        auto fnGetUUID = reinterpret_cast<Fn_GetUUID>(
            *reinterpret_cast<uintptr_t*>(localPlayer)  // vtable
            + off::OFFSET_Player_getOrCreateUniqueID
        );

        // mce::UUID is 16 bytes on stack
        alignas(8) uint8_t uuidBytes[16] = {};
        fnGetUUID(localPlayer, uuidBytes);

        // Format as standard UUID string
        auto hex = [](uint8_t b) -> std::string {
            char buf[3];
            std::snprintf(buf, sizeof(buf), "%02x", b);
            return buf;
        };

        std::ostringstream oss;
        for (int i = 0; i < 4; ++i)  oss << hex(uuidBytes[i]);
        oss << "-";
        for (int i = 4; i < 6; ++i)  oss << hex(uuidBytes[i]);
        oss << "-";
        for (int i = 6; i < 8; ++i)  oss << hex(uuidBytes[i]);
        oss << "-";
        for (int i = 8; i < 10; ++i) oss << hex(uuidBytes[i]);
        oss << "-";
        for (int i = 10; i < 16; ++i) oss << hex(uuidBytes[i]);

        PZ_LOG_DEBUG("Fetched player UUID: {}", oss.str());
        return oss.str();

    } catch (...) {
        PZ_LOG_ERROR("Exception during UUID fetch — possibly invalid offsets");
        return std::nullopt;
    }
}

// ─────────────────────────────────────────────
//  Internal: Device ID fallback
// ─────────────────────────────────────────────
std::optional<std::string> SeedManager::fetchDeviceID() {
    PZ_LOG_DEBUG("Attempting to fetch device-unique ID...");

    /*
     * On Bedrock, you can try:
     *   - Xbox Live token hash (if signed in)
     *   - com.mojang/minecraftpe/client.id file content
     *   - Platform-specific device serial
     *
     * For BDS (server), there's no "local player" at all.
     * Use the server's dedicated UUID or a config-fixed value.
     *
     * Placeholder: read from a known file path
     */
    const char* deviceIdPath = "com.mojang/minecraftpe/client.id";

    // In practice you'd use std::filesystem or platform APIs.
    // For now, return nullopt to signal "not implemented yet"
    PZ_LOG_WARN("Device ID fetch not yet implemented for this platform");
    return std::nullopt;
}

// ─────────────────────────────────────────────
//  Internal: UUID → seed derivation
// ─────────────────────────────────────────────
uint64_t SeedManager::deriveSeedFromUUID(const std::string& uuidStr) {
    /*
     * We use a two-pass hash to get good avalanche:
     *   1. FNV-1a over the UUID string bytes
     *   2. murmur3-style finalizer (bit mixing)
     *
     * This ensures that even similar UUIDs produce wildly different seeds.
     */

    // Pass 1: FNV-1a 64-bit
    uint64_t hash = 0xcbf29ce484222325ULL;  // FNV offset basis
    for (char c : uuidStr) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= 0x100000001b3ULL;  // FNV prime
    }

    // Pass 2: Avalanche / finalizer (murmur3 64-bit fmix)
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;

    PZ_LOG_TRACE("Derived seed from UUID '{}': 0x{:016X}", uuidStr, hash);
    return hash;
}

} // namespace personalized
