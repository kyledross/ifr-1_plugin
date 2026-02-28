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
    static int g_dropdownOpenIndex = -1;
    
    static const std::vector<std::string> g_osdPositions = {
        "lower-left",
        "lower-right",
        "upper-left",
        "upper-right"
    };

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
        int height = 165;
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

        // Make window non-resizable by setting min and max to the same size
        XPLMSetWindowResizingLimits(g_window, width, height, width, height);

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
        g_dropdownOpenIndex = -1;
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
        int settingIndex = 0;
        for (const auto& s : settings) {
            bool isBoolean = (s.value == "true" || s.value == "false");
            bool isOsdPosition = (s.name == "osd-position");
            
            if (isBoolean) {
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
            } else if (isOsdPosition) {
                // Draw description label
                XPLMDrawString(col_white, x, y - 11, const_cast<char*>(s.description.c_str()), nullptr, xplmFont_Basic);
                
                // Draw dropdown box
                int dropdown_w = 120;
                int dropdown_h = 20;
                int dropdown_l = x + 185;
                int dropdown_r = dropdown_l + dropdown_w;
                int dropdown_t = y;
                int dropdown_b = y - dropdown_h;
                
                XPLMSetGraphicsState(0, 0, 0, 0, 1, 0, 0);
                
                // Background
                glColor4f(0.2f, 0.2f, 0.2f, 1.0f);
                glBegin(GL_QUADS);
                glVertex2i(dropdown_l, dropdown_t);
                glVertex2i(dropdown_r, dropdown_t);
                glVertex2i(dropdown_r, dropdown_b);
                glVertex2i(dropdown_l, dropdown_b);
                glEnd();
                
                // Border
                glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
                glBegin(GL_LINE_LOOP);
                glVertex2i(dropdown_l, dropdown_t);
                glVertex2i(dropdown_r, dropdown_t);
                glVertex2i(dropdown_r, dropdown_b);
                glVertex2i(dropdown_l, dropdown_b);
                glEnd();
                
                // Draw current value
                XPLMDrawString(col_white, dropdown_l + 5, dropdown_b + 6, const_cast<char*>(s.value.c_str()), nullptr, xplmFont_Basic);
                
                // Draw dropdown arrow
                glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
                glBegin(GL_TRIANGLES);
                int arrow_x = dropdown_r - 12;
                int arrow_y = dropdown_b + 10;
                glVertex2i(arrow_x, arrow_y + 3);
                glVertex2i(arrow_x + 6, arrow_y + 3);
                glVertex2i(arrow_x + 3, arrow_y - 3);
                glEnd();
                
                // If dropdown is open, draw options
                if (g_dropdownOpenIndex == settingIndex) {
                    int option_h = 20;
                    int options_t = dropdown_b;
                    int option_idx = 0;
                    for (const auto& option : g_osdPositions) {
                        int option_b = options_t - option_h;
                        
                        // Background
                        if (option == s.value) {
                            glColor4f(0.3f, 0.4f, 0.5f, 1.0f);
                        } else {
                            glColor4f(0.25f, 0.25f, 0.25f, 1.0f);
                        }
                        glBegin(GL_QUADS);
                        glVertex2i(dropdown_l, options_t);
                        glVertex2i(dropdown_r, options_t);
                        glVertex2i(dropdown_r, option_b);
                        glVertex2i(dropdown_l, option_b);
                        glEnd();
                        
                        // Border
                        glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
                        glBegin(GL_LINE_LOOP);
                        glVertex2i(dropdown_l, options_t);
                        glVertex2i(dropdown_r, options_t);
                        glVertex2i(dropdown_r, option_b);
                        glVertex2i(dropdown_l, option_b);
                        glEnd();
                        
                        // Text
                        XPLMDrawString(col_white, dropdown_l + 5, option_b + 6, const_cast<char*>(option.c_str()), nullptr, xplmFont_Basic);
                        
                        options_t = option_b;
                        option_idx++;
                    }
                }
            }

            y -= 25;
            settingIndex++;
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
        int settingIndex = 0;
        for (const auto& s : settings) {
            bool isBoolean = (s.value == "true" || s.value == "false");
            bool isOsdPosition = (s.name == "osd-position");
            
            if (isBoolean) {
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
            } else if (isOsdPosition) {
                int dropdown_w = 120;
                int dropdown_h = 20;
                int dropdown_l = cur_x + 185;
                int dropdown_r = dropdown_l + dropdown_w;
                int dropdown_t = cur_y;
                int dropdown_b = cur_y - dropdown_h;
                
                // Check if clicking on the dropdown box
                if (x >= dropdown_l && x <= dropdown_r && y <= dropdown_t && y >= dropdown_b) {
                    if (g_dropdownOpenIndex == settingIndex) {
                        g_dropdownOpenIndex = -1;
                    } else {
                        g_dropdownOpenIndex = settingIndex;
                    }
                    return 1;
                }
                
                // Check if clicking on dropdown options
                if (g_dropdownOpenIndex == settingIndex) {
                    int option_h = 20;
                    int options_t = dropdown_b;
                    for (const auto& option : g_osdPositions) {
                        int option_b = options_t - option_h;
                        if (x >= dropdown_l && x <= dropdown_r && y <= options_t && y >= option_b) {
                            g_settingsManager->SetString(s.name, option);
                            g_settingsManager->Save(*g_sdk);
                            g_dropdownOpenIndex = -1;
                            return 1;
                        }
                        options_t = option_b;
                    }
                }
            }

            cur_y -= 25;
            settingIndex++;
        }
        
        // Close dropdown if clicking outside
        g_dropdownOpenIndex = -1;

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
