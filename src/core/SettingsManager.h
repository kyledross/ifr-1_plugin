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

#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "XPlaneSDK.h"

struct Setting {
    std::string name;
    std::string description;
    std::string value;
};

class SettingsManager {
public:
    explicit SettingsManager(std::filesystem::path settingsPath);
    void Load(IXPlaneSDK& sdk);
    void Save(IXPlaneSDK& sdk);
    
    bool GetBool(const std::string& name, bool defaultValue = false) const;
    void SetBool(const std::string& name, bool value);
    
    const std::vector<Setting>& GetSettings() const { return m_settings; }

private:
    std::filesystem::path m_path;
    std::vector<Setting> m_settings;
    
    void SetDefaultSettings();
};
