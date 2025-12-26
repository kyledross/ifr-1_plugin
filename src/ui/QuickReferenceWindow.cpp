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

#include "ui/QuickReferenceWindow.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <GL/gl.h>
#include <unordered_map>

namespace ui::quick_ref
{
    // Window state
    static XPLMWindowID g_window = nullptr;
    static bool g_monitorRegistered = false;

    // Content state
    static std::vector<std::string> g_rawLines;
    static std::vector<std::string> g_wrappedLines;
    static int g_wrapWidthCached = -1;
    static int g_scrollPx = 0;

    // UI metrics (recomputed in Draw)
    static int g_winL = 0, g_winT = 0, g_winR = 0, g_winB = 0;

    // Scrollbar dragging state
    static bool g_isDraggingScrollbar = false;
    static float g_scrollbarDragClickOffset = 0.0f;

    static std::string GetShiftInstruction(const std::string& modeName) {
        static const std::unordered_map<std::string, std::string> shiftMap = {
            {"hdg", "COM1"},
            {"baro", "COM2"},
            {"crs1", "NAV1"},
            {"crs2", "NAV2"},
            {"fms1-alt", "FMS1"},
            {"fms2-alt", "FMS2"},
            {"ap-alt", "AP"},
            {"xpdr-mode", "XPDR"}
        };

        auto it = shiftMap.find(modeName);
        if (it != shiftMap.end()) {
            std::string upperMode = modeName;
            std::ranges::transform(upperMode, upperMode.begin(), ::toupper);
            
            std::string instruction = "Press ";
            instruction += it->second;
            instruction += ", then press and hold the inner knob to enter ";
            instruction += upperMode;
            instruction += " mode.";
            return instruction;
        }
        return "";
    }

    static void BuildRawContent(const nlohmann::json& config) {
        g_rawLines.clear();
        if (config.empty()) {
            g_rawLines.emplace_back("No aircraft configuration loaded.");
            return;
        }

        std::string acfName = config.value("name", "Unknown Aircraft");
        std::string title = "Quick Reference: ";
        title += acfName;
        g_rawLines.push_back(std::move(title));
        g_rawLines.emplace_back("");

        if (!config.contains("modes") || !config["modes"].is_object()) {
            g_rawLines.emplace_back("No modes defined in configuration.");
            return;
        }

        for (auto& [modeName, modeJson] : config["modes"].items()) {
            std::string modeTitle = modeName;
            std::ranges::transform(modeTitle, modeTitle.begin(), ::toupper);
            
            std::string modeHeader = "[";
            modeHeader += modeTitle;
            modeHeader += "]";
            g_rawLines.push_back(std::move(modeHeader));

            std::string shiftInstr = GetShiftInstruction(modeName);
            if (!shiftInstr.empty()) {
                std::string line = "  ";
                line += shiftInstr;
                g_rawLines.push_back(std::move(line));
                g_rawLines.emplace_back("");
            }

            if (modeJson.is_object()) {
                for (auto& [eventName, eventJson] : modeJson.items()) {
                    if (eventJson.is_object()) {
                        bool eventHeaderAdded = false;
                        for (auto& [interactionName, interactionJson] : eventJson.items()) {
                            if (interactionJson.is_object() && interactionJson.contains("description")) {
                                if (!eventHeaderAdded) {
                                    std::string header = "  ";
                                    header += eventName;
                                    header += ":";
                                    g_rawLines.push_back(std::move(header));
                                    eventHeaderAdded = true;
                                }
                                std::string desc = interactionJson["description"].get<std::string>();
                                
                                std::string line = "    - ";
                                line += interactionName;
                                line += ": ";
                                line += desc;
                                g_rawLines.push_back(std::move(line));
                            }
                        }
                    }
                }
            }
            g_rawLines.emplace_back("");
        }
    }

    static void EnsureWrappedContent(int max_width_px) {
        if (max_width_px <= 0) return;
        if (g_wrapWidthCached == max_width_px && !g_wrappedLines.empty()) return;

        g_wrappedLines.clear();
        g_wrapWidthCached = max_width_px;

        for (const auto& rawLine : g_rawLines) {
            if (rawLine.empty()) {
                g_wrappedLines.emplace_back("");
                continue;
            }

            // Simple word wrap
            std::istringstream iss(rawLine);
            std::string word;
            std::string currentLine;
            
            // Determine indentation of the original line
            size_t firstNonSpace = rawLine.find_first_not_of(' ');
            std::string indent = (firstNonSpace == std::string::npos) ? "" : rawLine.substr(0, firstNonSpace);
            // Additional indent for wrapped lines
            std::string wrapIndent = indent;
            wrapIndent += "      ";

            while (iss >> word) {
                std::string testLine;
                if (currentLine.empty()) {
                    testLine = indent;
                    testLine += word;
                } else {
                    testLine = currentLine;
                    testLine += " ";
                    testLine += word;
                }
                
                int width = static_cast<int>(XPLMMeasureString(xplmFont_Basic, testLine.c_str(), static_cast<int>(testLine.size())));
                
                if (width > max_width_px) {
                    if (!currentLine.empty()) {
                        g_wrappedLines.push_back(currentLine);
                        currentLine = wrapIndent;
                        currentLine += word;
                    } else {
                        // Word itself is too long, just force it (could be improved)
                        g_wrappedLines.push_back(testLine);
                        currentLine.clear();
                    }
                } else {
                    currentLine = testLine;
                }
            }
            if (!currentLine.empty()) {
                g_wrappedLines.push_back(currentLine);
            }
        }
    }

    // Forward declarations
    static void DrawCB(XPLMWindowID inWindowID, void *inRefcon);
    static int MouseCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void *inRefcon);
    static int WheelCB(XPLMWindowID inWindowID, int x, int y, int wheel, int clicks, void *inRefcon);
    static int CursorCB(XPLMWindowID inWindowID, int x, int y, void *inRefcon);
    static float MonitorCB(float, float, int, void *);

    void Show(const nlohmann::json& config) {
        if (g_window) {
            // Re-build content in case config changed
            BuildRawContent(config);
            g_wrapWidthCached = -1; // Force re-wrap
            XPLMBringWindowToFront(g_window);
            return;
        }

        BuildRawContent(config);
        g_wrapWidthCached = -1;
        g_scrollPx = 0;
        g_isDraggingScrollbar = false;

        int l, t, r, b;
        XPLMGetScreenBoundsGlobal(&l, &t, &r, &b);
        int width = 600;
        int height = 600;
        int left = (l + r - width) / 2;
        int top = (t + b + height) / 2;
        
        XPLMCreateWindow_t params{};
        params.structSize = sizeof(params);
        params.visible = 1;
        params.drawWindowFunc = DrawCB;
        params.handleMouseClickFunc = MouseCB;
        params.handleMouseWheelFunc = WheelCB;
        params.handleCursorFunc = CursorCB;
        params.left = left;
        params.top = top;
        params.right = left + width;
        params.bottom = top - height;
        params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
        params.layer = xplm_WindowLayerFloatingWindows;

        g_window = XPLMCreateWindowEx(&params);
        XPLMSetWindowTitle(g_window, "Quick Reference");
        XPLMSetWindowResizingLimits(g_window, 300, 200, 1000, 1000);

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

    static void DrawCB(XPLMWindowID inWindowID, void * /*inRefcon*/) {
        int l, t, r, b;
        XPLMGetWindowGeometry(inWindowID, &l, &t, &r, &b);
        g_winL = l; g_winT = t; g_winR = r; g_winB = b;

        int char_w, line_h;
        XPLMGetFontDimensions(xplmFont_Basic, &char_w, &line_h, nullptr);

        int padding = 15;
        int scrollbar_w = 12;
        int inner_l = l + padding;
        int inner_r = r - padding - scrollbar_w - 5;
        int inner_t = t - padding;
        int inner_b = b + padding;

        EnsureWrappedContent(inner_r - inner_l);

        int lines_fit = std::max(0, (inner_t - inner_b) / line_h);
        int total_lines = static_cast<int>(g_wrappedLines.size());
        int max_scroll_px = std::max(0, (total_lines - lines_fit) * line_h);
        g_scrollPx = std::clamp(g_scrollPx, 0, max_scroll_px);

        // Draw background for scrollbar
        int sb_l = r - padding - scrollbar_w;
        int sb_r = r - padding;
        int sb_t = inner_t;
        int sb_b = inner_b;

        XPLMSetGraphicsState(0, 0, 0, 0, 1, 0, 0);
        glColor4f(0.2f, 0.2f, 0.2f, 0.5f);
        glBegin(GL_QUADS);
        glVertex2i(sb_l, sb_t);
        glVertex2i(sb_r, sb_t);
        glVertex2i(sb_r, sb_b);
        glVertex2i(sb_l, sb_b);
        glEnd();

        // Draw thumb
        if (total_lines > lines_fit) {
            float visible_ratio = static_cast<float>(lines_fit) / static_cast<float>(total_lines);
            int track_h = sb_t - sb_b;
            int thumb_h = std::max(20, static_cast<int>(static_cast<float>(track_h) * visible_ratio));
            
            float scroll_ratio = (max_scroll_px > 0) ? (static_cast<float>(g_scrollPx) / static_cast<float>(max_scroll_px)) : 0.0f;
            int thumb_t = sb_t - static_cast<int>(static_cast<float>(track_h - thumb_h) * scroll_ratio);
            int thumb_b = thumb_t - thumb_h;

            if (g_isDraggingScrollbar) glColor4f(0.7f, 0.7f, 0.7f, 0.8f);
            else glColor4f(0.5f, 0.5f, 0.5f, 0.8f);

            glBegin(GL_QUADS);
            glVertex2i(sb_l + 1, thumb_t);
            glVertex2i(sb_r - 1, thumb_t);
            glVertex2i(sb_r - 1, thumb_b);
            glVertex2i(sb_l + 1, thumb_b);
            glEnd();
        }

        // Draw text
        float white[3] = {1.0f, 1.0f, 1.0f};
        int first_line = g_scrollPx / line_h;
        int offset_y = g_scrollPx % line_h;
        
        // We need to clip text to the inner box. X-Plane doesn't provide easy clipping for floating windows
        // so we'll just draw the lines that fit.
        int y = inner_t + offset_y;
        for (int i = 0; i <= lines_fit && (first_line + i) < total_lines; ++i) {
            int draw_y = y - (i * line_h);
            // Simple clipping check
            if (draw_y - line_h >= inner_b && draw_y <= inner_t) {
                 XPLMDrawString(white, inner_l, draw_y - line_h + 3, const_cast<char*>(g_wrappedLines[first_line + i].c_str()), nullptr, xplmFont_Basic);
            }
        }
    }

    static int MouseCB(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void* /*inRefcon*/) {
        int padding = 15;
        int scrollbar_w = 12;
        int sb_l = g_winR - padding - scrollbar_w;
        int sb_r = g_winR - padding;
        int sb_t = g_winT - padding;
        int sb_b = g_winB + padding;
        int track_h = sb_t - sb_b;

        int char_w, line_h;
        XPLMGetFontDimensions(xplmFont_Basic, &char_w, &line_h, nullptr);
        int inner_t = g_winT - padding;
        int inner_b = g_winB + padding;
        int lines_fit = std::max(0, (inner_t - inner_b) / line_h);
        int total_lines = static_cast<int>(g_wrappedLines.size());
        int max_scroll_px = std::max(0, (total_lines - lines_fit) * line_h);

        if (inMouse == xplm_MouseDown) {
            if (x >= sb_l && x <= sb_r && y >= sb_b && y <= sb_t) {
                if (total_lines > lines_fit) {
                    float visible_ratio = static_cast<float>(lines_fit) / static_cast<float>(total_lines);
                    int thumb_h = std::max(20, static_cast<int>(static_cast<float>(track_h) * visible_ratio));
                    float scroll_ratio = (max_scroll_px > 0) ? (static_cast<float>(g_scrollPx) / static_cast<float>(max_scroll_px)) : 0.0f;
                    int thumb_t = sb_t - static_cast<int>(static_cast<float>(track_h - thumb_h) * scroll_ratio);
                    int thumb_b = thumb_t - thumb_h;

                    if (y >= thumb_b && y <= thumb_t) {
                        g_isDraggingScrollbar = true;
                        g_scrollbarDragClickOffset = static_cast<float>(thumb_t - y);
                    } else {
                        // Jump to
                        float new_ratio = static_cast<float>(sb_t - y - thumb_h / 2) / static_cast<float>(track_h - thumb_h);
                        g_scrollPx = static_cast<int>(new_ratio * max_scroll_px);
                        g_scrollPx = std::clamp(g_scrollPx, 0, max_scroll_px);
                        g_isDraggingScrollbar = true;
                        g_scrollbarDragClickOffset = static_cast<float>(thumb_h) / 2.0f;
                    }
                    return 1;
                }
            }
        } else if (inMouse == xplm_MouseDrag) {
            if (g_isDraggingScrollbar) {
                float visible_ratio = static_cast<float>(lines_fit) / static_cast<float>(total_lines);
                int thumb_h = std::max(20, static_cast<int>(static_cast<float>(track_h) * visible_ratio));
                float target_thumb_t = static_cast<float>(y) + g_scrollbarDragClickOffset;
                float new_ratio = (static_cast<float>(sb_t) - target_thumb_t) / static_cast<float>(track_h - thumb_h);
                g_scrollPx = static_cast<int>(new_ratio * max_scroll_px);
                g_scrollPx = std::clamp(g_scrollPx, 0, max_scroll_px);
                return 1;
            }
        } else if (inMouse == xplm_MouseUp) {
            g_isDraggingScrollbar = false;
        }
        return 0;
    }

    static int WheelCB(XPLMWindowID /*inWindowID*/, int x, int y, int /*wheel*/, int clicks, void* /*inRefcon*/) {
        if (x >= g_winL && x <= g_winR && y >= g_winB && y <= g_winT) {
            int char_w, line_h;
            XPLMGetFontDimensions(xplmFont_Basic, &char_w, &line_h, nullptr);
            g_scrollPx -= clicks * line_h * 3; // Scroll 3 lines at a time
            
            int padding = 15;
            int inner_t = g_winT - padding;
            int inner_b = g_winB + padding;
            int lines_fit = std::max(0, (inner_t - inner_b) / line_h);
            int total_lines = static_cast<int>(g_wrappedLines.size());
            int max_scroll_px = std::max(0, (total_lines - lines_fit) * line_h);
            g_scrollPx = std::clamp(g_scrollPx, 0, max_scroll_px);
            return 1;
        }
        return 0;
    }

    static int CursorCB(XPLMWindowID /*inWindowID*/, int /*x*/, int /*y*/, void* /*inRefcon*/) {
        return xplm_CursorDefault;
    }

    static float MonitorCB(float /*inElapsed*/, float /*inElapsedFlightLoop*/, int /*inCounter*/, void* /*inRefcon*/) {
        if (g_window && !XPLMGetWindowIsVisible(g_window)) {
            Close();
        }
        return -1.0f;
    }
}
