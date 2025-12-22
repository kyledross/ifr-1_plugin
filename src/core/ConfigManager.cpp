#include "ConfigManager.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

size_t ConfigManager::LoadConfigs(const std::string& directoryPath, IXPlaneSDK& sdk) {
    m_configs.clear();
    m_fallbackConfig.clear();
    
    sdk.Log(LogLevel::Info, ("Loading configurations from: " + directoryPath).c_str());
    
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        sdk.Log(LogLevel::Error, "Config directory does not exist or is not a directory.");
        return 0;
    }

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.path().extension() == ".json") {
            try {
                std::ifstream file(entry.path());
                if (file.is_open()) {
                    nlohmann::json config;
                    file >> config;
                    if (!config.contains("name")) {
                        config["name"] = entry.path().stem().string();
                    }
                    
                    std::string configName = config["name"];
                    bool isFallback = config.value("fallback", false);
                    bool hasOutput = config.contains("output");
                    
                    sdk.Log(LogLevel::Info, ("  - Loaded " + configName + (isFallback ? " (fallback)" : "") + (hasOutput ? " [has output]" : " [NO OUTPUT!]")).c_str());
                    
                    if (isFallback) {
                        m_fallbackConfig = config;
                    } else {
                        m_configs.push_back(config);
                    }
                }
            } catch (const std::exception& e) {
                sdk.Log(LogLevel::Error, ("Error loading config " + entry.path().string() + ": " + e.what()).c_str());
            }
        }
    }
    
    if (m_fallbackConfig.empty()) {
        sdk.Log(LogLevel::Info, "No fallback configuration was found.");
    }
    
    return m_configs.size() + (m_fallbackConfig.empty() ? 0 : 1);
}

nlohmann::json ConfigManager::GetConfigForAircraft(const std::string& aircraftFilename, IXPlaneSDK& sdk) const {
    sdk.Log(LogLevel::Info, ("Matching configuration for aircraft: " + aircraftFilename).c_str());
    
    for (const auto& config : m_configs) {
        if (config.contains("aircraft") && config["aircraft"].is_array()) {
            for (const auto& aircraft : config["aircraft"]) {
                if (aircraft.is_string()) {
                    std::string aircraftPattern = aircraft.get<std::string>();
                    if (aircraftFilename.find(aircraftPattern) != std::string::npos) {
                        sdk.Log(LogLevel::Info, ("  - Matched specific config: " + config.value("name", "unknown")).c_str());
                        return config;
                    }
                }
            }
        }
    }
    
    if (!m_fallbackConfig.empty()) {
        sdk.Log(LogLevel::Info, ("  - No specific match, using fallback: " + m_fallbackConfig.value("name", "unknown")).c_str());
        return m_fallbackConfig;
    }
    
    sdk.Log(LogLevel::Info, "  - No match found and no fallback available.");
    return {};
}
