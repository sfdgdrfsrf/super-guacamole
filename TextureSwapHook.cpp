/**
 * TextureSwapHook.cpp — Block/item texture scrambling implementation.
 *
 * Hook strategies (choose based on what LeviLamina exposes for your version):
 *
 *   STRATEGY A (preferred): Hook BlockPalette::getBlock so that every
 *     block lookup returns a different block's render definition.
 *
 *   STRATEGY B: Hook into the texture-pack loading pipeline
 *     (TexturePackRepository / ResourcePackStack) and remap
 *     texture file paths before they're committed to GPU.
 *
 *   STRATEGY C: Post-load patch — after all blocks are registered,
 *     iterate BlockTypeRegistry and swap their render-block pointers.
 *
 * This file implements Strategy C as the primary approach (it's the
 * safest and most portable), with Strategy A shown as a commented hook.
 */

#include "TextureSwapHook.h"
#include "SeedManager.h"
#include "Config.h"
#include "Utils/Logger.h"
#include "Utils/OffsetDefs.h"
#include "Utils/RandomMapper.h"

#include "ll/api/memory/Hook.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockLegacy.h"
#include "mc/world/level/block/BlockTypeRegistry.h"
#include "mc/world/level/block/registry/BlockPalette.h"

namespace personalized {

bool TextureSwapHook::s_hooked = false;
std::vector<size_t> TextureSwapHook::s_remapTable;
std::unordered_map<std::string, std::string> TextureSwapHook::s_nameRemap;
size_t TextureSwapHook::s_blockCount = 0;

// ─────────────────────────────────────────────
//  Build the permutation table
// ─────────────────────────────────────────────
void TextureSwapHook::buildRemapTable() {
    PZ_LOG_INFO("Building texture swap remap table...");

    if (!SeedManager::instance().isInitialized()) {
        PZ_LOG_WARN("SeedManager not ready — cannot build texture swap");
        return;
    }

    uint64_t seed = SeedManager::instance().getSeed();

    /*
     * Enumerate all registered blocks.
     *
     * In LeviLamina you can iterate BlockTypeRegistry, or use
     * BlockPalette::getBlockPalette() to get the full list.
     *
     * Placeholder: assume a known count. In production code, query
     * the registry dynamically.
     */
    constexpr size_t ESTIMATED_BLOCK_COUNT = 900;  // ~Bedrock 1.21
    s_blockCount = ESTIMATED_BLOCK_COUNT;

    // Build intensity-filtered partial scramble
    // (we don't want to swap EVERY block — just a configurable fraction)
    Config cfg;  // In practice, use the loaded config
    double intensity = cfg.textureSwapIntensity;

    s_remapTable = RandomMapper::partialScramble(seed, s_blockCount, intensity, 3);

    PZ_LOG_INFO("Texture remap table built: {} blocks, intensity {:.0f}%",
                s_blockCount, intensity * 100.0);

    // Debug: log a few sample remaps
    if (cfg.verboseLogging && s_blockCount > 10) {
        for (size_t i = 0; i < 10 && i < s_blockCount; ++i) {
            PZ_LOG_DEBUG("  block[{}] → block[{}]", i, s_remapTable[i]);
        }
    }
}

// ─────────────────────────────────────────────
//  STRATEGY C: Post-load block render pointer swap
// ─────────────────────────────────────────────
// This hook fires after BlockPalette finishes initialization.
// We iterate all blocks and swap their render-block references.

// Hook target: BlockPalette::initializeFromDisk (or equivalent)
// This is the function called once after all blocks are registered.

/*
LL_TYPE_INSTANCE_HOOK(
    BlockPaletteInitHook,
    BlockPalette,
    off::OFFSET_BlockPalette_initializeFromDisk,
    void
) {
    // Call original — let BDS finish normal initialization
    origin();

    PZ_LOG_INFO("BlockPalette init complete — applying texture swaps");

    if (!SeedManager::instance().isInitialized()) {
        PZ_LOG_WARN("SeedManager not ready at palette init time, deferring...");
        // We'll try again when UUIDHook fires
        return;
    }

    buildRemapTable();

    if (s_remapTable.empty()) return;

    // Iterate all blocks and swap render definitions
    auto& palette = *this;  // The BlockPalette instance

    for (size_t i = 0; i < s_blockCount; ++i) {
        size_t targetIdx = s_remapTable[i];
        if (targetIdx == i) continue;  // No swap needed

        // Get source and target blocks
        // Block* srcBlock  = palette.getBlock(i);    // original block
        // Block* destBlock = palette.getBlock(targetIdx); // block whose texture we want

        // if (!srcBlock || !destBlock) continue;

        // Swap render block pointer:
        // srcBlock->mRenderBlock = destBlock->mRenderBlock;
        //
        // OR if the API provides a setter:
        // srcBlock->setRenderBlock(destBlock->getRenderBlock());

        PZ_LOG_TRACE("Swapped render: block[{}] now renders as block[{}]", i, targetIdx);
    }

    PZ_LOG_INFO("Texture swap applied to {} blocks", s_blockCount);
}
*/

// ─────────────────────────────────────────────
//  STRATEGY A: Intercept BlockPalette::getBlock at call time
// ─────────────────────────────────────────────
// This is a runtime hook: every call to getBlock returns the
// "wrong" block's render definition. More invasive but always
// up-to-date.

/*
LL_TYPE_INSTANCE_HOOK(
    BlockPaletteGetBlockHook,
    BlockPalette,
    off::OFFSET_BlockPalette_getBlock,
    Block*,
    const BlockLegacy& legacy,
    int data
) {
    Block* result = origin(legacy, data);

    if (!SeedManager::instance().isInitialized() || s_remapTable.empty()) {
        return result;
    }

    // Look up the original block's index and remap
    // size_t origIdx = /* resolve legacy → numeric ID */;
    // size_t newIdx  = getRemappedIndex(origIdx);
    // return palette.getBlock(newIdx);

    return result;  // passthrough until offsets are mapped
}
*/

// ─────────────────────────────────────────────
//  STRATEGY B: Texture path remapping (resource pack level)
// ─────────────────────────────────────────────
// Instead of swapping block objects, we swap the texture FILE
// that each block points to. This is more like the Java mod's
// approach (resource pack override).

/*
LL_TYPE_INSTANCE_HOOK(
    TexturePackLoadHook,
    TexturePackRepository,
    off::OFFSET_TexturePackRepository_loadPack,
    void,
    const std::string& packPath
) {
    origin(packPath);

    // After the pack loads, intercept the texture lookup table
    // and remap "textures/blocks/stone.png" → "textures/blocks/dirt.png", etc.
    PZ_LOG_INFO("Texture pack loaded from: {}", packPath);

    // Apply name-based remap
    for (auto& [origName, newName] : s_nameRemap) {
        PZ_LOG_DEBUG("Remapping texture: {} → {}", origName, newName);
        // resourcePack->remapTexture(origName, newName);
    }
}
*/

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────
void TextureSwapHook::install() {
    if (s_hooked) {
        PZ_LOG_WARN("TextureSwapHook already installed");
        return;
    }

    PZ_LOG_INFO("Installing TextureSwapHook...");

    // The actual LL_HOOK installation happens via the static
    // hook objects defined above. In LeviLamina, hooks are
    // auto-registered when the hook class's .hook() is called.
    //
    // Example:
    //   BlockPaletteInitHook::hook();
    //   BlockPaletteGetBlockHook::hook();

    PZ_LOG_INFO("TextureSwapHook installed (strategy: post-load render swap)");
    PZ_LOG_WARN("NOTE: Hook bodies are commented out — fill in offsets and uncomment");

    s_hooked = true;
}

void TextureSwapHook::uninstall() {
    // BlockPaletteInitHook::unhook();
    s_hooked = false;
    s_remapTable.clear();
    s_nameRemap.clear();
    s_blockCount = 0;
    PZ_LOG_INFO("TextureSwapHook uninstalled");
}

void TextureSwapHook::rebuildMapping() {
    PZ_LOG_INFO("Rebuilding texture swap mapping...");
    buildRemapTable();
}

size_t TextureSwapHook::getRemappedIndex(size_t originalIndex) {
    if (originalIndex >= s_remapTable.size()) {
        return originalIndex;  // Out of range — identity
    }
    return s_remapTable[originalIndex];
}

} // namespace personalized
