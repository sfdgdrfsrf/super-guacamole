#pragma once

/**
 * TextureSwapHook.h — Scrambles block/item texture assignments.
 *
 * Mechanism:
 *   - Intercepts BlockPalette or BlockTypeRegistry during world load
 *     to remap block IDs → different render definitions.
 *   - Uses the SeedManager's seed to build a deterministic permutation
 *     of block indices so the same player always sees the same swap.
 *   - Client-side only: the server's block state is untouched.
 */

#include <vector>
#include <string>
#include <unordered_map>

namespace personalized {

class TextureSwapHook {
public:
    /// Install the hook
    static void install();

    /// Unhook
    static void uninstall();

    /// Force a re-scan and re-map (call if blocks reload)
    static void rebuildMapping();

    /// Query: what does block at original index render as?
    static size_t getRemappedIndex(size_t originalIndex);

private:
    static bool s_hooked;

    /// The permutation table: remap[i] = j means "block i renders as block j"
    static std::vector<size_t> s_remapTable;

    /// Name-based remap for string lookups
    static std::unordered_map<std::string, std::string> s_nameRemap;

    /// Total block count at time of mapping
    static size_t s_blockCount;

    /// Build the remap table from current seed
    static void buildRemapTable();
};

} // namespace personalized
