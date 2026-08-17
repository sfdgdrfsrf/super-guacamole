/**
 * Config.cpp — JSON serialization for Config.h using nlohmann/json.
 */

#include "Config.h"
#include "Utils/Logger.h"
#include "nlohmann/json.hpp"
#include <fstream>

namespace personalized {

using json = nlohmann::json;

bool Config::loadFromFile(Config& out, const std::string& path) {
    PZ_LOG_INFO("Loading config from: {}", path);

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        PZ_LOG_WARN("Config file not found at '{}', using defaults", path);
        return false;
    }

    try {
        json j = json::parse(ifs);

        out.enabled               = j.value("enabled",               out.enabled);
        out.seedSource            = j.value("seedSource",            out.seedSource);
        out.fixedSeed             = j.value("fixedSeed",             out.fixedSeed);
        out.textureSwapEnabled    = j.value("textureSwapEnabled",    out.textureSwapEnabled);
        out.textureBlockNamespaceFilter = j.value("textureBlockNamespaceFilter", out.textureBlockNamespaceFilter);
        out.textureSwapIntensity  = j.value("textureSwapIntensity",  out.textureSwapIntensity);
        out.inventoryScrambleEnabled    = j.value("inventoryScrambleEnabled",    out.inventoryScrambleEnabled);
        out.inventoryShufflePasses      = j.value("inventoryShufflePasses",      out.inventoryShufflePasses);
        out.inventoryRescrambleIntervalTicks = j.value("inventoryRescrambleIntervalTicks", out.inventoryRescrambleIntervalTicks);
        out.mobModelSwapEnabled   = j.value("mobModelSwapEnabled",   out.mobModelSwapEnabled);
        out.mobTargetFilter       = j.value("mobTargetFilter",       out.mobTargetFilter);
        out.mobModelPool          = j.value("mobModelPool",          out.mobModelPool);
        out.mobSwapIntensity      = j.value("mobSwapIntensity",      out.mobSwapIntensity);
        out.verboseLogging        = j.value("verboseLogging",        out.verboseLogging);
        out.dryRun                = j.value("dryRun",                out.dryRun);

        PZ_LOG_INFO("Config loaded successfully");
        return true;
    } catch (const json::exception& e) {
        PZ_LOG_ERROR("Failed to parse config JSON: {}", e.what());
        return false;
    }
}

bool Config::saveToFile(const std::string& path) const {
    json j = {
        {"enabled",                          enabled},
        {"seedSource",                       seedSource},
        {"fixedSeed",                        fixedSeed},
        {"textureSwapEnabled",               textureSwapEnabled},
        {"textureBlockNamespaceFilter",      textureBlockNamespaceFilter},
        {"textureSwapIntensity",             textureSwapIntensity},
        {"inventoryScrambleEnabled",         inventoryScrambleEnabled},
        {"inventoryShufflePasses",           inventoryShufflePasses},
        {"inventoryRescrambleIntervalTicks", inventoryRescrambleIntervalTicks},
        {"mobModelSwapEnabled",              mobModelSwapEnabled},
        {"mobTargetFilter",                  mobTargetFilter},
        {"mobModelPool",                     mobModelPool},
        {"mobSwapIntensity",                 mobSwapIntensity},
        {"verboseLogging",                   verboseLogging},
        {"dryRun",                           dryRun},
    };

    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        PZ_LOG_ERROR("Cannot write config to '{}'", path);
        return false;
    }

    ofs << j.dump(4);
    PZ_LOG_INFO("Config saved to: {}", path);
    return true;
}

} // namespace personalized
