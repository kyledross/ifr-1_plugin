#include "ConfigManager.h"
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

size_t ConfigManager::LoadConfigs(const std::string& directoryPath) {
    m_configs.clear();
    m_fallbackConfig.clear();
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
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
                    if (config.value("fallback", false)) {
                        m_fallbackConfig = config;
                    } else {
                        m_configs.push_back(config);
                    }
                }
            } catch (const std::exception& e) {
                // In a real plugin, we'd use XPLMDebugString here
                std::cerr << "Error loading config " << entry.path() << ": " << e.what() << std::endl;
            }
        }
    }
    return m_configs.size() + (m_fallbackConfig.empty() ? 0 : 1);
}

nlohmann::json ConfigManager::GetConfigForAircraft(const std::string& aircraftFilename) const {
    for (const auto& config : m_configs) {
        if (config.contains("aircraft") && config["aircraft"].is_array()) {
            for (const auto& aircraft : config["aircraft"]) {
                if (aircraft.is_string() && aircraftFilename.find(aircraft.get<std::string>()) != std::string::npos) {
                    return config;
                }
            }
        }
    }
    return m_fallbackConfig;
}
