#include "ConfigManager.h"
#include "Logger.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

size_t ConfigManager::LoadConfigs(const std::string& directoryPath, IXPlaneSDK& sdk) {
    m_configs.clear();
    m_fallbackConfig.clear();
    
    IFR1_LOG_INFO(sdk, "Loading configurations from: {}", directoryPath);
    
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        IFR1_LOG_ERROR(sdk, "Config directory does not exist or is not a directory.");
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
                    
                    IFR1_LOG_INFO(sdk, "  - Loaded {}{}{}", configName, (isFallback ? " (fallback)" : ""), (hasOutput ? " [has output]" : " [NO OUTPUT!]"));
                    
                    if (isFallback) {
                        m_fallbackConfig = config;
                    } else {
                        m_configs.push_back(config);
                    }
                }
            } catch (const std::exception& e) {
                IFR1_LOG_ERROR(sdk, "Error loading config {}: {}", entry.path().string(), e.what());
            }
        }
    }
    
    if (m_fallbackConfig.empty()) {
        IFR1_LOG_INFO(sdk, "No fallback configuration was found.");
    }
    
    return m_configs.size() + (m_fallbackConfig.empty() ? 0 : 1);
}

nlohmann::json ConfigManager::GetConfigForAircraft(const std::string& aircraftFilename, IXPlaneSDK& sdk) const {
    IFR1_LOG_INFO(sdk, "Matching configuration for aircraft: {}", aircraftFilename);
    
    for (const auto& config : m_configs) {
        if (config.contains("aircraft") && config["aircraft"].is_array()) {
            for (const auto& aircraft : config["aircraft"]) {
                if (aircraft.is_string()) {
                    std::string aircraftPattern = aircraft.get<std::string>();
                    if (aircraftFilename.find(aircraftPattern) != std::string::npos) {
                        IFR1_LOG_INFO(sdk, "  - Matched specific config: {}", config.value("name", "unknown"));
                        return config;
                    }
                }
            }
        }
    }
    
    if (!m_fallbackConfig.empty()) {
        IFR1_LOG_INFO(sdk, "  - No specific match, using fallback: {}", m_fallbackConfig.value("name", "unknown"));
        return m_fallbackConfig;
    }
    
    IFR1_LOG_INFO(sdk, "  - No match found and no fallback available.");
    return {};
}
