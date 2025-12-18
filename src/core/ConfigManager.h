#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>

class ConfigManager {
public:
    ConfigManager() = default;

    /**
     * @brief Loads all JSON configurations from the specified directory.
     * @param directoryPath Path to the directory containing .json files.
     * @return Number of configurations loaded.
     */
    size_t LoadConfigs(const std::string& directoryPath);

    /**
     * @brief Finds the configuration that matches the given aircraft filename.
     * @param aircraftFilename The filename of the .acf file.
     * @return The matching JSON configuration, or empty json if not found.
     */
    nlohmann::json GetConfigForAircraft(const std::string& aircraftFilename) const;

private:
    std::vector<nlohmann::json> m_configs;
};
