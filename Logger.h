#pragma once

/**
 * Logger.h — Thin wrapper around LeviLamina's logger.
 *
 * Every module should include this instead of raw ilaGetLogger() calls
 * so we can toggle verbosity, prefix module names, and compile out
 * debug traces in release builds.
 */

#include "ll/api/Logger.h"
#include <string>
#include <source_location>

// ─────────────────────────────────────────────
//  Module-level logger factory
// ─────────────────────────────────────────────
namespace personalized {

/// Returns a module-scoped logger. Each .cpp should call this once at
/// file scope with its module name.
inline ll::Logger& ModLogger() {
    static ll::Logger logger("Personalized");
    return logger;
}

/// Create a sub-logger for a specific hook module
inline ll::Logger MakeModuleLogger(const std::string& name) {
    return ll::Logger("Personalized::" + name);
}

} // namespace personalized

// ─────────────────────────────────────────────
//  Convenience macros
// ─────────────────────────────────────────────
// These expand to the global mod logger with file:line context.

#define PZ_LOG_DEBUG(...)  ::personalized::ModLogger().debug(__VA_ARGS__)
#define PZ_LOG_INFO(...)   ::personalized::ModLogger().info(__VA_ARGS__)
#define PZ_LOG_WARN(...)   ::personalized::ModLogger().warn(__VA_ARGS__)
#define PZ_LOG_ERROR(...)  ::personalized::ModLogger().error(__VA_ARGS__)
#define PZ_LOG_FATAL(...)  ::personalized::ModLogger().fatal(__VA_ARGS__)

// Trace-level: only compiled in debug builds
#ifndef NDEBUG
#define PZ_LOG_TRACE(...) ::personalized::ModLogger().debug("[TRACE] " __VA_ARGS__)
#else
#define PZ_LOG(...) ((void)0)
#endif
