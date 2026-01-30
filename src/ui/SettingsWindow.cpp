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

#include "ui/SettingsWindow.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include <GL/gl.h>
#include <vector>
#include <string>
#include <algorithm>

namespace ui::settings {
    static XPLMWindowID g_window = nullptr;
    static SettingsManager* g_settingsManager = nullptr;
    static IXPlaneSDK* g_sdk = nullptr;
    static bool g_monitorRegistered = false;

    static void DrawCB(XPLMWindowID inWindowID, void* inRefcon);
    static int MouseCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void* inRefcon);
    static int CursorCB(XPLMWindowID inWindowID, int x, int y, void* inRefcon);
    static float MonitorCB(float inElapsed, float inElapsedFlightLoop, int inCounter, void* inRefcon);

    void Show(SettingsManager& settingsManager, IXPlaneSDK& sdk) {
        if (g_window) {
            XPLMSetWindowIsVisible(g_window, 1);
            XPLMBringWindowToFront(g_window);
            return;
        }

        g_settingsManager = &settingsManager;
        g_sdk = &sdk;

        int l, t, r, b;
        XPLMGetScreenBoundsGlobal(&l, &t, &r, &b);
        int width = 450;
        int height = 100;
        int left = (l + r - width) / 2;
        int top = (t + b + height) / 2;

        XPLMCreateWindow_t params{};
        params.structSize = sizeof(params);
        params.visible = 1;
        params.drawWindowFunc = DrawCB;
        params.handleMouseClickFunc = MouseCB;
        params.handleCursorFunc = CursorCB;
        params.left = left;
        params.top = top;
        params.right = left + width;
        params.bottom = top - height;
        params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
        params.layer = xplm_WindowLayerFloatingWindows;

        g_window = XPLMCreateWindowEx(&params);
        XPLMSetWindowTitle(g_window, "IFR-1 Settings");

#ifdef XPLMSetWindowCloseButtonVisible
        XPLMSetWindowCloseButtonVisible(g_window, 1);
#endif

        if (!g_monitorRegistered) {
            XPLMRegisterFlightLoopCallback(MonitorCB, -1.0f, nullptr);
            g_monitorRegistered = true;
        }
    }

    void Close() {
        if (g_window) {
            XPLMDestroyWindow(g_window);
            g_window = nullptr;
        }
        if (g_monitorRegistered) {
            XPLMUnregisterFlightLoopCallback(MonitorCB, nullptr);
            g_monitorRegistered = false;
        }
    }

    bool IsOpen() {
        return g_window != nullptr;
    }

    static void DrawCB(XPLMWindowID inWindowID, void*) {
        int l, t, r, b;
        XPLMGetWindowGeometry(inWindowID, &l, &t, &r, &b);

        float col_white[] = {1.0f, 1.0f, 1.0f};
        int char_w, line_h;
        XPLMGetFontDimensions(xplmFont_Basic, &char_w, &line_h, nullptr);

        int x = l + 20;
        int y = t - 30;

        if (!g_settingsManager) return;

        const auto& settings = g_settingsManager->GetSettings();
        for (const auto& s : settings) {
            // Draw checkbox box
            int box_size = 14;
            int box_l = x;
            int box_r = x + box_size;
            int box_t = y;
            int box_b = y - box_size;

            XPLMSetGraphicsState(0, 0, 0, 0, 1, 0, 0);
            
            // Background
            glColor4f(0.2f, 0.2f, 0.2f, 1.0f);
            glBegin(GL_QUADS);
            glVertex2i(box_l, box_t);
            glVertex2i(box_r, box_t);
            glVertex2i(box_r, box_b);
            glVertex2i(box_l, box_b);
            glEnd();

            // Border
            glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2i(box_l, box_t);
            glVertex2i(box_r, box_t);
            glVertex2i(box_r, box_b);
            glVertex2i(box_l, box_b);
            glEnd();

            // Checkmark
            if (s.value == "true") {
                glColor4f(0.2f, 0.8f, 0.2f, 1.0f);
                glLineWidth(2.0f);
                glBegin(GL_LINES);
                glVertex2i(box_l + 3, box_t - 7);
                glVertex2i(box_l + 6, box_b + 3);
                glVertex2i(box_l + 6, box_b + 3);
                glVertex2i(box_r - 2, box_t - 3);
                glEnd();
                glLineWidth(1.0f);
            }

            // Draw description
            XPLMDrawString(col_white, x + box_size + 10, y - 11, const_cast<char*>(s.description.c_str()), nullptr, xplmFont_Basic);

            y -= 30;
        }
    }

    static int MouseCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void*) {
        if (inMouse != xplm_MouseDown) return 1;

        int l, t, r, b;
        XPLMGetWindowGeometry(inWindowID, &l, &t, &r, &b);

        int cur_x = l + 20;
        int cur_y = t - 30;

        if (!g_settingsManager) return 1;

        const auto& settings = g_settingsManager->GetSettings();
        for (const auto& s : settings) {
            int box_size = 14;
            int box_l = cur_x;
            int box_t = cur_y;
            int box_b = cur_y - box_size;

            int char_w, line_h;
            XPLMGetFontDimensions(xplmFont_Basic, &char_w, &line_h, nullptr);
            int text_r = cur_x + box_size + 10 + (static_cast<int>(s.description.size()) * char_w);

            if (x >= box_l && x <= text_r && y <= box_t && y >= box_b) {
                bool currentVal = (s.value == "true");
                g_settingsManager->SetBool(s.name, !currentVal);
                g_settingsManager->Save(*g_sdk);
                return 1;
            }

            cur_y -= 30;
        }

        return 1;
    }

    static int CursorCB(XPLMWindowID, int, int, void*) {
        return xplm_CursorDefault;
    }

    static float MonitorCB(float, float, int, void*) {
        if (g_window && !XPLMGetWindowIsVisible(g_window)) {
            Close();
        }
        return -1.0f;
    }
}
