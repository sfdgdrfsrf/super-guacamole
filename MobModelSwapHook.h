#pragma once

/**
 * MobModelSwapHook.h — Swaps mob rendering models client-side.
 *
 * Mechanism:
 *   - Intercepts the Actor/Mob creation pipeline to randomly assign
 *     an incorrect model definition to spawned entities.
 *   - The server still sees the correct entity — this is purely a
 *     client-side visual deception, just like the Java "Personalized" mod.
 *   - Uses the seed to deterministically decide which mob gets which
 *     wrong model, so the same player always sees the same swaps.
 */

#include <vector>
#include <string>
#include <unordered_map>

namespace personalized {

class MobModelSwapHook {
public:
    /// Install the hook
    static void install();

    /// Unhook
    static void uninstall();

    /// Add a model swap mapping at runtime
    static void addSwap(const std::string& mobId, const std::string& modelId);

    /// Query: what model should this mob render with?
    static std::string getSwappedModel(const std::string& originalMobId);

private:
    static bool s_hooked;

    /// Deterministic mapping: mobIdentifier → modelIdentifier
    static std::unordered_map<std::string, std::string> s_modelSwapMap;

    /// Build the swap map from seed + config
    static void buildSwapMap();
};

} // namespace personalized
