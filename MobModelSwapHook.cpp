/**
 * MobModelSwapHook.cpp — Entity model swapping on spawn.
 *
 * Hook strategies:
 *
 *   STRATEGY A: Hook ActorFactory::createActor — the central dispatch
 *     where all entities are born. After the original creates the actor,
 *     we overwrite its model reference with a random one from the pool.
 *
 *   STRATEGY B: Hook Mob::setMobModel or Actor::setModel — called
 *     during actor initialization. We intercept the model-set call
 *     and substitute a different model.
 *
 *   STRATEGY C: Listen to ActorAddEvent (LeviLamina event bus) and
 *     post-hoc swap the model on newly added actors. Less invasive
 *     but may cause a visible "flash" of the correct model.
 *
 * We implement Strategy C as the primary (safest, event-based) and
 * show Strategy A as a commented raw hook.
 */

#include "MobModelSwapHook.h"
#include "SeedManager.h"
#include "Config.h"
#include "Utils/Logger.h"
#include "Utils/OffsetDefs.h"

#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/world/ActorAddEvent.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/Mob.h"

namespace personalized {

bool MobModelSwapHook::s_hooked = false;
std::unordered_map<std::string, std::string> MobModelSwapHook::s_modelSwapMap;

// ─────────────────────────────────────────────
//  Build the swap map
// ─────────────────────────────────────────────
void MobModelSwapHook::buildSwapMap() {
    PZ_LOG_INFO("Building mob model swap map...");

    if (!SeedManager::instance().isInitialized()) {
        PZ_LOG_WARN("SeedManager not ready — cannot build mob swap map");
        return;
    }

    Config cfg;
    uint64_t seed = SeedManager::instance().getSeed();

    // Use a different seed salt so mob swaps are independent of
    // texture and inventory permutations
    std::mt19937_64 rng(seed ^ 0xB0B0FACEDEADBEEFULL);

    s_modelSwapMap.clear();

    for (const auto& mobId : cfg.mobTargetFilter) {
        // Skip player — we never swap the local player's model
        if (mobId.find("player") != std::string::npos) continue;

        // Randomly select a model from the pool
        if (cfg.mobModelPool.empty()) continue;

        // Intensity check: should this mob be swapped at all?
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) > cfg.mobSwapIntensity) {
            PZ_LOG_DEBUG("Mob '{}' skipped (intensity check)", mobId);
            continue;
        }

        // Pick a model from the pool (avoid picking the same model)
        std::string selectedModel;
        int attempts = 0;
        do {
            std::uniform_int_distribution<size_t> idxDist(0, cfg.mobModelPool.size() - 1);
            selectedModel = cfg.mobModelPool[idxDist(rng)];
            ++attempts;
        } while (selectedModel == mobId && attempts < 10);

        s_modelSwapMap[mobId] = selectedModel;
        PZ_LOG_DEBUG("Mob swap: {} → {}", mobId, selectedModel);
    }

    PZ_LOG_INFO("Mob model swap map built: {} entries", s_modelSwapMap.size());
}

// ─────────────────────────────────────────────
//  STRATEGY C: ActorAddEvent listener
// ─────────────────────────────────────────────
void MobModelSwapHook::install() {
    if (s_hooked) {
        PZ_LOG_WARN("MobModelSwapHook already installed");
        return;
    }

    PZ_LOG_INFO("Installing MobModelSwapHook (ActorAddEvent listener)...");

    auto& bus = ll::event::EventBus::getInstance();

    bus.emplaceListener<ll::event::ActorAddEvent>([](ll::event::ActorAddEvent& ev) {
        if (s_modelSwapMap.empty()) return;  // Not built yet

        auto& actor = ev.self();

        // Skip players entirely
        if (actor.isPlayer()) return;

        // Get the actor's identifier (e.g. "minecraft:zombie")
        // In LeviLamina: actor.getActorIdentifier().getCanonicalName()
        // or actor.getTypeName()
        std::string actorId;  // = actor.getTypeName();

        // Placeholder: extract the identifier
        // actorId = actor.getActorIdentifier().getCanonicalName();

        // Check if this mob type has a swap mapping
        auto it = s_modelSwapMap.find(actorId);
        if (it == s_modelSwapMap.end()) {
            // Try with just the namespace-stripped name
            // e.g. "zombie" from "minecraft:zombie"
            std::string shortName = actorId;
            auto colonPos = actorId.find(':');
            if (colonPos != std::string::npos) {
                shortName = actorId.substr(colonPos + 1);
            }
            it = s_modelSwapMap.find(shortName);
        }

        if (it == s_modelSwapMap.end()) return;  // No swap for this mob

        const std::string& newModel = it->second;

        PZ_LOG_DEBUG("Swapping model for actor '{}': now renders as '{}'",
                     actorId, newModel);

        /*
         * ── Apply the model swap ──
         *
         * The actual mechanism depends on how Bedrock stores models:
         *
         * Option 1: Overwrite the Actor's model definition pointer
         *   actor.setModelDefinition(ModelRegistry::get(newModel));
         *
         * Option 2: Change the render component's model hash
         *   auto& renderComp = actor.getRenderComponent();
         *   renderComp.modelHash = hashModelName(newModel);
         *
         * Option 3: Patch the entity's runtime identifier
         *   so the client's renderer picks up the wrong model:
         *   actor.mRuntimeIdentifier = ActorIdentifier(newModel);
         *
         * All of these are client-side only. The server's entity
         * state is unaffected.
         */

        // Placeholder: log what we'd do
        PZ_LOG_INFO("[DRY] Would swap model of '{}' to '{}'", actorId, newModel);
    });

    s_hooked = true;
    PZ_LOG_INFO("MobModelSwapHook installed via ActorAddEvent");
    PZ_LOG_WARN("NOTE: Model swap body is placeholder — fill in Actor::setModel logic");
}

// ─────────────────────────────────────────────
//  STRATEGY A: Raw hook on ActorFactory::createActor
// ─────────────────────────────────────────────
/*
LL_TYPE_INSTANCE_HOOK(
    ActorFactoryCreateHook,
    ActorFactory,
    off::OFFSET_ActorFactory_createActor,
    Actor*,
    const ActorIdentifier&    identifier,
    Level&                    level,
    const Vec3&               pos,
    const Vec2&               rotation,
    bool                      isClientSide
) {
    // Call original creation
    Actor* actor = origin(identifier, level, pos, rotation, isClientSide);

    if (!actor) return nullptr;
    if (!isClientSide) return actor;  // Server-side: don't touch
    if (actor->isPlayer()) return actor;

    // Check swap map
    std::string id = identifier.getCanonicalName();
    auto it = s_modelSwapMap.find(id);
    if (it == s_modelSwapMap.end()) return actor;

    // Swap the model
    PZ_LOG_DEBUG("ActorFactory: swapping model for '{}' → '{}'", id, it->second);
    // actor->setModel(ModelRegistry::get(it->second));

    return actor;
}
*/

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────
void MobModelSwapHook::uninstall() {
    // ActorFactoryCreateHook::unhook();
    s_hooked = false;
    s_modelSwapMap.clear();
    PZ_LOG_INFO("MobModelSwapHook uninstalled");
}

void MobModelSwapHook::addSwap(const std::string& mobId, const std::string& modelId) {
    s_modelSwapMap[mobId] = modelId;
    PZ_LOG_DEBUG("Added mob swap: {} → {}", mobId, modelId);
}

std::string MobModelSwapHook::getSwappedModel(const std::string& originalMobId) {
    auto it = s_modelSwapMap.find(originalMobId);
    if (it != s_modelSwapMap.end()) {
        return it->second;
    }
    return originalMobId;  // Identity — no swap
}

} // namespace personalized
