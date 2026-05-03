/*
 *   Copyright 2026 Kyle D. Ross
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#pragma once
#include "XPlaneSDK.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class ConfigManager {
public:
    ConfigManager() = default;

    /**
     * @brief Loads all JSON configurations from the specified directory.
     * @param directoryPath Path to the directory containing .json files.
     * @param sdk Reference to X-Plane SDK for logging.
     * @return Number of configurations loaded.
     */
    size_t LoadConfigs(const std::string& directoryPath, IXPlaneSDK& sdk);

    /**
     * @brief Finds the configuration that matches the given aircraft filename.
     * @param aircraftFilename The filename of the .acf file.
     * @param sdk Reference to X-Plane SDK for logging.
     * @return The matching JSON configuration, or empty json if not found.
     */
    [[nodiscard]] nlohmann::json GetConfigForAircraft(const std::string& aircraftFilename, IXPlaneSDK& sdk) const;

private:
    std::vector<nlohmann::json> m_configs;
    nlohmann::json m_fallbackConfig;
};
