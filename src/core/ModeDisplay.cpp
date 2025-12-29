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

#include "ModeDisplay.h"
#include <algorithm>

ModeDisplay::ModeDisplay(IXPlaneSDK& sdk) : m_sdk(sdk) {
    IXPlaneSDK::WindowCreateParams params{};
    params.left = 20;
    params.bottom = 20;
    params.right = 200;
    params.top = 80;
    params.visible = 0;
    params.drawCallback = ModeDisplay::DrawCallback;
    params.refcon = this;
    m_windowId = m_sdk.CreateWindowEx(params);
}

ModeDisplay::~ModeDisplay() {
    if (m_windowId) {
        m_sdk.DestroyWindow(m_windowId);
    }
}

void ModeDisplay::ShowMessage(const std::string& message, float currentTime) {
    m_message = message;
    m_startTime = currentTime;
    m_opacity = 0.0f;

    if (m_windowId) {
        int textW = m_sdk.MeasureString(m_message.c_str());
        int textH = m_sdk.GetFontHeight();

        int paddingX = 8;
        int paddingY = 4;
        int boxW = textW + paddingX * 2;
        int boxH = textH + paddingY * 2;

        int l = 20;
        int b = 20;
        m_sdk.SetWindowGeometry(m_windowId, l, b + boxH, l + boxW, b);
    }
}

void ModeDisplay::Update(float currentTime) {
    if (m_startTime < 0) {
        m_opacity = 0.0f;
    } else {
        float elapsed = currentTime - m_startTime;
        if (elapsed < 0.25f) {
            m_opacity = elapsed / 0.25f;
        } else if (elapsed < 2.25f) {
            m_opacity = 1.0f;
        } else if (elapsed < 3.25f) {
            m_opacity = 1.0f - (elapsed - 2.25f) / 1.0f;
        } else {
            m_opacity = 0.0f;
            m_startTime = -1.0f;
        }
    }

    if (m_windowId) {
        m_sdk.SetWindowVisible(m_windowId, m_opacity > 0.0f ? 1 : 0);
    }
}

void ModeDisplay::Draw() {
    // No-op, we use window callback now
}

void ModeDisplay::DrawCallback(void* /*windowId*/, void* refcon) {
    auto* self = static_cast<ModeDisplay*>(refcon);
    self->DrawWindow();
}

void ModeDisplay::DrawWindow() {
    if (m_opacity <= 0.0f || m_message.empty()) return;

    int l, t, r, b;
    m_sdk.GetWindowGeometry(m_windowId, &l, &t, &r, &b);
    int width = r - l;
    int height = t - b;

    // Box (black)
    float boxColor[4] = {0.0f, 0.0f, 0.0f, m_opacity * 0.9f};
    m_sdk.DrawRectangle(boxColor, l, t, r, b);

    // Border (white)
    float borderColor[4] = {1.0f, 1.0f, 1.0f, m_opacity};
    m_sdk.DrawRectangleOutline(borderColor, l, t, r, b);

    // Text (white)
    float textColor[4] = {1.0f, 1.0f, 1.0f, m_opacity};
    int textW = m_sdk.MeasureString(m_message.c_str());
    int textH = m_sdk.GetFontHeight();

    int textX = l + (width - textW) / 2;
    int textY = b + (height - textH) / 2 + 2;

    // Draw only once for better clarity (1:1 pixel mapping in window helps)
    m_sdk.DrawString(textColor, textX, textY, m_message.c_str());
}
