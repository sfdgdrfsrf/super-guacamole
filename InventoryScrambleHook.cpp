/**
 * InventoryScrambleHook.cpp — Creative inventory scrambling.
 *
 * Hook strategies:
 *
 *   STRATEGY A: Hook CreativeItemRegistry::getCreativeItems and return
 *     a permuted span. This is the cleanest approach — the game thinks
 *     it's reading the normal list but we've rearranged the entries.
 *
 *   STRATEGY B: Hook the UI-side CreativeInventoryPanel::refreshItems
 *     or similar to re-sort the displayed items after the panel loads.
 *
 *   STRATEGY C: Tick-level hook — use a server-tick or client-tick
 *     event to periodically reshuffle the in-memory item list.
 *     Good for "inventory reshuffles every N seconds" chaos mode.
 */

#include "InventoryScrambleHook.h"
#include "SeedManager.h"
#include "Config.h"
#include "Utils/Logger.h"
#include "Utils/OffsetDefs.h"
#include "Utils/RandomMapper.h"

#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/server/ServerTickEvent.h"

// Bedrock headers (availability depends on LeviLamina version)
// #include "mc/world/item/CreativeItemRegistry.h"
// #include "mc/world/item/Item.h"
// #include "mc/world/item/ItemInstance.h"

namespace personalized {

bool InventoryScrambleHook::s_hooked = false;
std::vector<size_t> InventoryScrambleHook::s_slotPermutation;
int InventoryScrambleHook::s_tickCounter = 0;

// ─────────────────────────────────────────────
//  Apply shuffle to the creative item list
// ─────────────────────────────────────────────
void InventoryScrambleHook::applyShuffle() {
    PZ_LOG_INFO("Applying creative inventory shuffle...");

    if (!SeedManager::instance().isInitialized()) {
        PZ_LOG_WARN("SeedManager not ready — cannot shuffle inventory");
        return;
    }

    uint64_t seed = SeedManager::instance().getSeed();

    /*
     * Get the total creative item count.
     *
     * In Bedrock 1.21.x, there are roughly 900-1100 creative items.
     * The exact count can be queried from CreativeItemRegistry.
     *
     * Placeholder count — replace with runtime query:
     *   auto& items = CreativeItemRegistry::getCreativeItems();
     *   size_t count = items.size();
     */
    constexpr size_t ESTIMATED_CREATIVE_ITEMS = 1000;
    size_t itemCount = ESTIMATED_CREATIVE_ITEMS;

    Config cfg;  // Use loaded config in practice
    s_slotPermutation = RandomMapper::fisherYatesShuffle(
        seed ^ 0xC0FFEE0000000000ULL,  // XOR with a constant so inventory
                                         // permutation differs from texture
                                         // permutation despite same seed
        itemCount,
        cfg.inventoryShufflePasses
    );

    PZ_LOG_INFO("Creative inventory shuffled: {} items, {} passes",
                itemCount, cfg.inventoryShufflePasses);

    /*
     * ── Apply the permutation to the actual creative item list ──
     *
     * In practice, you'd do something like:
     *
     *   auto& items = CreativeItemRegistry::getMutableCreativeItems();
     *   std::vector<ItemInstance> original = items;  // copy
     *   for (size_t i = 0; i < items.size(); ++i) {
     *       items[i] = original[s_slotPermutation[i]];
     *   }
     *
     * OR if the registry is read-only, you'd hook the accessor function
     * and return items on-the-fly from a cached copy.
     */
}

// ─────────────────────────────────────────────
//  STRATEGY A: Hook CreativeItemRegistry::getCreativeItems
// ─────────────────────────────────────────────
// Return a permuted view of the creative items list.

/*
LL_TYPE_STATIC_HOOK(
    CreativeItemsGetHook,
    CreativeItemRegistry,
    off::OFFSET_CreativeItemRegistry_getCreativeItems,
    std::vector<ItemInstance>&
) {
    auto& items = origin();

    if (!SeedManager::instance().isInitialized()) return items;
    if (s_slotPermutation.empty()) return items;

    // Apply permutation lazily on first call, then cache
    static std::vector<ItemInstance> scrambled;
    static bool cached = false;

    if (!cached) {
        scrambled.resize(items.size());
        for (size_t i = 0; i < items.size(); ++i) {
            size_t srcIdx = s_slotPermutation[i];
            if (srcIdx < items.size()) {
                scrambled[i] = items[srcIdx];
            }
        }
        cached = true;
        PZ_LOG_DEBUG("Creative items scrambled on first getCreativeItems call");
    }

    return scrambled;
}
*/

// ─────────────────────────────────────────────
//  STRATEGY C: Tick-based periodic reshuffle
// ─────────────────────────────────────────────
static ll::event::ListenerPtr s_tickListener;

void InventoryScrambleHook::install() {
    if (s_hooked) {
        PZ_LOG_WARN("InventoryScrambleHook already installed");
        return;
    }

    PZ_LOG_INFO("Installing InventoryScrambleHook...");

    Config cfg;

    // ── Initial shuffle (fires once when SeedManager is ready) ──
    // We listen for the first tick after seed init to apply the shuffle.

    if (cfg.inventoryRescrambleIntervalTicks > 0) {
        PZ_LOG_INFO("Periodic re-shuffle enabled: every {} ticks",
                     cfg.inventoryRescrambleIntervalTicks);

        auto& bus = ll::event::EventBus::getInstance();
        s_tickListener = bus.emplaceListener<ll::event::ServerTickEvent>(
            [](ll::event::ServerTickEvent& ev) {
                if (!SeedManager::instance().isInitialized()) return;

                ++s_tickCounter;
                if (s_tickCounter >= cfg.inventoryRescrambleIntervalTicks) {
                    s_tickCounter = 0;
                    PZ_LOG_DEBUG("Re-shuffling creative inventory (tick interval reached)");
                    reshuffle();
                }
            }
        );
    }

    // ── One-time shuffle after seed is ready ──
    // Listen for a "seed ready" signal (simplified: check on first tick)
    auto& bus = ll::event::EventBus::getInstance();
    static bool oneTimeShuffleDone = false;
    static auto oneTimeListener = bus.emplaceListener<ll::event::ServerTickEvent>(
        [](ll::event::ServerTickEvent&) {
            if (oneTimeShuffleDone) return;
            if (!SeedManager::instance().isInitialized()) return;

            oneTimeShuffleDone = true;
            applyShuffle();

            // Remove this listener after first use
            ll::event::EventBus::getInstance().removeListener<ll::event::ServerTickEvent>(
                oneTimeListener
            );
        }
    );

    PZ_LOG_INFO("InventoryScrambleHook installed");
    PZ_LOG_WARN("NOTE: Hook bodies are commented out — fill in offsets and uncomment");

    s_hooked = true;
}

void InventoryScrambleHook::uninstall() {
    // CreativeItemsGetHook::unhook();
    if (s_tickListener) {
        ll::event::EventBus::getInstance().removeListener<ll::event::ServerTickEvent>(
            s_tickListener
        );
    }
    s_hooked = false;
    s_slotPermutation.clear();
    s_tickCounter = 0;
    PZ_LOG_INFO("InventoryScrambleHook uninstalled");
}

void InventoryScrambleHook::reshuffle() {
    applyShuffle();
}

size_t InventoryScrambleHook::getScrambledSlot(size_t originalSlot) {
    if (originalSlot >= s_slotPermutation.size()) {
        return originalSlot;
    }
    return s_slotPermutation[originalSlot];
}

} // namespace personalized
