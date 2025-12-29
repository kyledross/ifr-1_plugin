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
#include "XPlaneSDK.h"
#include <string>

class ModeDisplay {
public:
    explicit ModeDisplay(IXPlaneSDK& sdk);
    ~ModeDisplay();
    void ShowMessage(const std::string& message, float currentTime);
    void Update(float currentTime);
    void Draw();

private:
    static void DrawCallback(void* windowId, void* refcon);
    void DrawWindow();

    IXPlaneSDK& m_sdk;
    void* m_windowId = nullptr;
    std::string m_message;
    float m_startTime = -1.0f;
    float m_opacity = 0.0f;
};
