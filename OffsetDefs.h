#pragma once

/**
 * OffsetDefs.h — Bedrock Dedicated Server v1.21.x placeholder symbols
 *
 * Every offset below is a PLACEHOLDER. You must rebase them against your
 * target BDS version using IDA/Ghidra + endstone/levilamina headers.
 *
 * Naming convention:
 *   OFFSET_{Namespace}_{Class}_{Method}   — virtual / member call offset
 *   SIG_{Namespace}_{Class}_{Method}      — signature string for signature-based hooks
 *   VTABLE_{Namespace}_{Class}            — vtable base for virtual hooks
 */

#include <cstdint>

namespace off {

// ─────────────────────────────────────────────
//  Player / LocalPlayer
// ─────────────────────────────────────────────
/// Player::getOrCreateUniqueID  — returns the player's UUID as a mce::UUID
constexpr uintptr_t OFFSET_Player_getOrCreateUniqueID = 0x0ULL;  // TODO: map

/// LocalPlayer pointer offset from ClientInstance
constexpr uintptr_t OFFSET_ClientInstance_LocalPlayer = 0x0ULL;  // TODO: map

/// Player::getPlayerName  — useful for debug logging
constexpr uintptr_t OFFSET_Player_getPlayerName = 0x0ULL;  // TODO: map

// ─────────────────────────────────────────────
//  Block / Texture Registry
// ─────────────────────────────────────────────
/// BlockPalette::getBlock  — resolve a BlockLegacy → Block*
constexpr uintptr_t OFFSET_BlockPalette_getBlock = 0x0ULL;  // TODO: map

/// BlockTypeRegistry::lookupByName  — name → BlockLegacy*
constexpr uintptr_t OFFSET_BlockTypeRegistry_lookupByName = 0x0ULL;  // TODO: map

/// Block::getRenderBlock  — returns the render block definition used for visuals
constexpr uintptr_t OFFSET_Block_getRenderBlock = 0x0ULL;  // TODO: map

/// TexturePackRepository — entry point for iterating loaded texture packs
constexpr uintptr_t OFFSET_TexturePackRepository_singleton = 0x0ULL;  // TODO: map

/// BlockGraphics::replaceTexture  — hypothetical: remap a block's texture path
constexpr uintptr_t OFFSET_BlockGraphics_replaceTexture = 0x0ULL;  // TODO: map

// ─────────────────────────────────────────────
//  Item / Creative Inventory
// ─────────────────────────────────────────────
/// CreativeItemRegistry::getCreativeItems  — returns const span of all creative items
constexpr uintptr_t OFFSET_CreativeItemRegistry_getCreativeItems = 0x0ULL;  // TODO: map

/// Item::getDescriptionId  — returns the lang-key string (e.g. "item.minecraft.diamond")
constexpr uintptr_t OFFSET_Item_getDescriptionId = 0x0ULL;  // TODO: map

/// ItemRegistry::getItem  — ItemLegacyPtr → Item*
constexpr uintptr_t OFFSET_ItemRegistry_getItem = 0x0ULL;  // TODO: map

// ─────────────────────────────────────────────
//  Actor / Entity Spawning
// ─────────────────────────────────────────────
/// ActorFactory::createActor  — main entity creation dispatch
constexpr uintptr_t OFFSET_ActorFactory_createActor = 0x0ULL;  // TODO: map

/// Actor::setModel  — set the runtime client-side model (Model& or hash)
constexpr uintptr_t OFFSET_Actor_setModel = 0x0ULL;  // TODO: map

/// Actor::getActorIdentifier  — returns ActorIdentifier (namespace:id)
constexpr uintptr_t OFFSET_Actor_getActorIdentifier = 0x0ULL;  // TODO: map

/// Mob::setMobModel  — mob-specific model setter (inherits Actor)
constexpr uintptr_t OFFSET_Mob_setMobModel = 0x0ULL;  // TODO: map

// ─────────────────────────────────────────────
//  ClientInstance (global access)
// ─────────────────────────────────────────────
/// Global ClientInstance singleton getter
constexpr uintptr_t OFFSET_ClientInstance_singleton = 0x0ULL;  // TODO: map

/// ClientInstance::getLocalPlayer  — returns LocalPlayer*
constexpr uintptr_t OFFSET_ClientInstance_getLocalPlayer = 0x0ULL;  // TODO: map

// ─────────────────────────────────────────────
//  Level / GameSession
// ─────────────────────────────────────────────
/// Level::getRuntimeActorList  — returns all currently loaded actors
constexpr uintptr_t OFFSET_Level_getRuntimeActorList = 0x0ULL;  // TODO: map

/// Level::getLocalPlayer  — alternative path
constexpr uintptr_t OFFSET_Level_getLocalPlayer = 0x0ULL;  // TODO: map

// ─────────────────────────────────────────────
//  mce::UUID layout  (16 bytes, two uint64_t halves)
// ─────────────────────────────────────────────
/// mce::UUID struct: first 8 bytes (most significant)
constexpr uintptr_t OFFSET_mce_UUID_MostSignificant = 0x0ULL;  // TODO: map

/// mce::UUID struct: last 8 bytes (least significant)
constexpr uintptr_t OFFSET_mce_UUID_LeastSignificant = 0x8ULL;  // TODO: map

// ─────────────────────────────────────────────
//  Signature strings  (for PatternScan hooks)
// ─────────────────────────────────────────────
// These are x64 byte-pattern sigs. Fill in for your BDS build.
namespace sig {
    // Example: "48 89 5C 24 ?? 48 83 EC ?? 48 8B 05 ?? ?? ?? ??"
    constexpr const char* Player_getOrCreateUniqueID = "";
    constexpr const char* BlockPalette_getBlock = "";
    constexpr const char* CreativeItemRegistry_getCreativeItems = "";
    constexpr const char* ActorFactory_createActor = "";
    constexpr const char* ClientInstance_getLocalPlayer = "";
}

// ─────────────────────────────────────────────
//  VTable bases
// ─────────────────────────────────────────────
namespace vtable {
    constexpr uintptr_t Block = 0x0ULL;          // TODO: map
    constexpr uintptr_t Item = 0x0ULL;           // TODO: map
    constexpr uintptr_t Actor = 0x0ULL;          // TODO: map
    constexpr uintptr_t Mob  = 0x0ULL;          // TODO: map
    constexpr uintptr_t Player = 0x0ULL;         // TODO: map
    constexpr uintptr_t LocalPlayer = 0x0ULL;    // TODO: map
}

} // namespace off
