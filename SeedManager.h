#pragma once

/**
 * SeedManager.h — Central seed management for the Personalized mod.
 *
 * Responsible for:
 *   1. Fetching the local player UUID (or fallback device ID)
 *   2. Deriving a stable uint64_t seed from it
 *   3. Providing seeded RNG to all hook modules
 */

#include <cstdint>
#include <random>
#include <string>
#include <optional>
#include <mutex>

namespace personalized {

class SeedManager {
public:
    /// Get the singleton instance
    static SeedManager& instance();

    /// Attempt to fetch the local player UUID and derive a seed.
    /// Call this once the player has joined (or on ClientInstance init).
    /// Returns true if a seed was successfully derived.
    bool initialize();

    /// Returns true if a seed has been set
    bool isInitialized() const;

    /// Get the raw 128-bit UUID as a string (e.g. "550e8400-e29b-41d4-a716-446655440000")
    const std::string& getUUIDString() const;

    /// Get the derived 64-bit seed
    uint64_t getSeed() const;

    /// Create a new seeded MT19937_64 engine (each module should get its own
    /// copy to avoid shared-state issues across hooks)
    std::mt19937_64 createRNG() const;

    /// Force-set a seed (used by Config::fixedSeed mode or testing)
    void setSeedOverride(uint64_t seed);

private:
    SeedManager() = default;

    mutable std::mutex m_mutex;

    bool        m_initialized = false;
    std::string m_uuidString;
    uint64_t    m_seed = 0;

    // ── Internal UUID fetch routines ──

    /// Try to read UUID from the LocalPlayer via ClientInstance
    std::optional<std::string> fetchPlayerUUID();

    /// Try to read a device-unique ID (Xbox Live token hash, etc.)
    std::optional<std::string> fetchDeviceID();

    /// Hash a UUID string into a 64-bit seed using FNV-1a + mixing
    static uint64_t deriveSeedFromUUID(const std::string& uuidStr);
};

} // namespace personalized
