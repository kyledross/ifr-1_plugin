/*
 *   Copyright 2025 Kyle D. Ross
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

#include "SettingsManager.h"
#include "Logger.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

SettingsManager::SettingsManager(std::filesystem::path settingsPath) : m_path(std::move(settingsPath)) {
    SetDefaultSettings();
}

void SettingsManager::Load(IXPlaneSDK& sdk) {
    if (!std::filesystem::exists(m_path)) {
        IFR1_LOG_INFO(sdk, "Settings file not found, using defaults: {}", m_path.string());
        Save(sdk);
        return;
    }

    try {
        std::ifstream file(m_path);
        if (!file.is_open()) {
            IFR1_LOG_ERROR(sdk, "Could not open settings file for reading: {}", m_path.string());
            return;
        }

        json j;
        file >> j;

        if (j.contains("options") && j["options"].is_array()) {
            for (const auto& item : j["options"]) {
                std::string name = item.value("option-name", "");
                if (name.empty()) continue;

                auto it = std::find_if(m_settings.begin(), m_settings.end(), [&](const Setting& s) {
                    return s.name == name;
                });

                if (it != m_settings.end()) {
                    it->description = item.value("option-description", it->description);
                    it->value = item.value("value", it->value);
                } else {
                    m_settings.push_back({
                        name,
                        item.value("option-description", ""),
                        item.value("value", "false")
                    });
                }
            }
        }
        IFR1_LOG_INFO(sdk, "Settings loaded from: {}", m_path.string());
    } catch (const std::exception& e) {
        IFR1_LOG_ERROR(sdk, "Error loading settings from {}: {}", m_path.string(), e.what());
    }
}

void SettingsManager::Save(IXPlaneSDK& sdk) {
    try {
        json j;
        j["options"] = json::array();
        for (const auto& s : m_settings) {
            j["options"].push_back({
                {"option-name", s.name},
                {"option-description", s.description},
                {"value", s.value}
            });
        }

        std::ofstream file(m_path);
        if (file.is_open()) {
            file << j.dump(4);
            IFR1_LOG_INFO(sdk, "Settings saved to: {}", m_path.string());
        } else {
            IFR1_LOG_ERROR(sdk, "Could not open settings file for writing: {}", m_path.string());
        }
    } catch (const std::exception& e) {
        IFR1_LOG_ERROR(sdk, "Error saving settings to {}: {}", m_path.string(), e.what());
    }
}

bool SettingsManager::GetBool(const std::string& name, bool defaultValue) const {
    auto it = std::find_if(m_settings.begin(), m_settings.end(), [&](const Setting& s) {
        return s.name == name;
    });

    if (it != m_settings.end()) {
        return it->value == "true";
    }
    return defaultValue;
}

void SettingsManager::SetBool(const std::string& name, bool value) {
    auto it = std::find_if(m_settings.begin(), m_settings.end(), [&](const Setting& s) {
        return s.name == name;
    });

    if (it != m_settings.end()) {
        it->value = value ? "true" : "false";
    } else {
        // Should we allow adding new ones here? Requirement says "Populate... with descriptions in the json"
        // so they should probably exist.
        m_settings.push_back({name, "", value ? "true" : "false"});
    }
}

void SettingsManager::SetDefaultSettings() {
    m_settings.clear();
    m_settings.push_back({
        "on-screen-mode-display",
        "Show mode changes on-screen",
        "false"
    });
}
