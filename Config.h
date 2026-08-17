#pragma once

/**
 * Config.h — Runtime-tunable parameters for the Personalized mod.
 *
 * These values can be loaded from a JSON config file at startup.
 * They control the intensity and scope of each scrambling module.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace personalized {

struct Config {
    // ─── Global toggle ───
    bool enabled = true;

    // ─── Seed source ───
    /// "uuid"   — use the local player's UUID
    /// "device" — use a device-unique ID (xbox/live token hash)
    /// "fixed"  — use Config::fixedSeed for testing
    std::string seedSource = "uuid";

    /// When seedSource == "fixed", this value is used directly
    uint64_t fixedSeed = 0xDEADBEEFCAFEBABEULL;

    // ─── Texture Scrambling ───
    bool textureSwapEnabled = true;

    /// Only scramble blocks whose namespace matches these prefixes
    /// Empty means "all".  Default: {"minecraft"}
    std::vector<std::string> textureBlockNamespaceFilter = {"minecraft"};

    /// Fraction of blocks to remap [0.0, 1.0]. 1.0 = full chaos.
    double textureSwapIntensity = 0.6;

    // ─── Creative Inventory Scrambling ───
    bool inventoryScrambleEnabled = true;

    /// Number of Fisher-Yates shuffle passes (more = more random-looking)
    int inventoryShufflePasses = 3;

    /// Re-scramble every N ticks (0 = once at startup only)
    int inventoryRescrambleIntervalTicks = 0;

    // ─── Mob Model Swapping ───
    bool mobModelSwapEnabled = true;

    /// Only swap models for mobs whose identifier contains these substrings
    /// Empty means "all non-player mobs"
    std::vector<std::string> mobTargetFilter = {"zombie", "skeleton", "pig", "cow", "sheep"};

    /// Pool of model identifiers to randomly assign
    std::vector<std::string> mobModelPool = {
        "minecraft:zombie",
        "minecraft:skeleton",
        "minecraft:pig",
        "minecraft:cow",
        "minecraft:sheep",
        "minecraft:chicken",
        "minecraft:creeper",
        "minecraft:spider",
    };

    /// Fraction of spawned mobs to swap [0.0, 1.0]
    double mobSwapIntensity = 0.5;

    // ─── Debug / Safety ───
    bool verboseLogging = true;
    bool dryRun = false;  ///< If true, log what *would* happen but don't modify anything

    // ─── Serialization ───
    /// Load from JSON file at the given path. Returns true on success.
    static bool loadFromFile(Config& out, const std::string& path);

    /// Save current config to JSON file
    bool saveToFile(const std::string& path) const;
};

} // namespace personalized
