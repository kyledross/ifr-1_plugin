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
#include "core/SettingsManager.h"
#include "core/XPlaneSDK.h"

namespace ui::settings {
    /**
     * @brief Shows the Settings window.
     * @param settingsManager Reference to the settings manager.
     * @param sdk Reference to the X-Plane SDK.
     */
    void Show(SettingsManager& settingsManager, IXPlaneSDK& sdk);

    /**
     * @brief Closes the Settings window.
     */
    void Close();

    /**
     * @brief Checks if the Settings window is currently open.
     * @return true if open, false otherwise.
     */
    bool IsOpen();
}
