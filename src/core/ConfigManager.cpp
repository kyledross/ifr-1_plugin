#include "ConfigManager.h"
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

size_t ConfigManager::LoadConfigs(const std::string& directoryPath) {
    m_configs.clear();
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
                    m_configs.push_back(config);
                }
            } catch (const std::exception& e) {
                // In a real plugin, we'd use XPLMDebugString here
                std::cerr << "Error loading config " << entry.path() << ": " << e.what() << std::endl;
            }
        }
    }
    return m_configs.size();
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
    return nlohmann::json{};
}
