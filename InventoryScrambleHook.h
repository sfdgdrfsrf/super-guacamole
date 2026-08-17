#pragma once

/**
 * InventoryScrambleHook.h — Scrambles the Creative Inventory item order.
 *
 * Mechanism:
 *   - Hooks into the CreativeItemRegistry or the UI inventory builder
 *     to permute the slot positions of items in the creative tab.
 *   - Uses a Fisher-Yates shuffle seeded by the player UUID.
 *   - Optionally re-shuffles on a tick interval for extra chaos.
 */

#include <vector>
#include <cstddef>

namespace personalized {

class InventoryScrambleHook {
public:
    /// Install the hook
    static void install();

    /// Unhook
    static void uninstall();

    /// Force a re-shuffle now
    static void reshuffle();

    /// Get the permuted slot for a given original slot
    static size_t getScrambledSlot(size_t originalSlot);

private:
    static bool s_hooked;

    /// Permutation table: slot[i] = j means "slot i shows item j"
    static std::vector<size_t> s_slotPermutation;

    /// Tick counter for interval-based re-shuffle
    static int s_tickCounter;

    /// Apply the shuffle to the in-memory creative item list
    static void applyShuffle();
};

} // namespace personalized
