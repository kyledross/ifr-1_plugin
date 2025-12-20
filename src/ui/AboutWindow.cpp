// AboutWindow.cpp - Implementation of the plugin's About dialog

#include "ui/AboutWindow.h"

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

// Embedded resource header (generated at build time)
#define RESOURCES_EMBED_IMPL
#include "resources_embedded.h"
#include "license_embedded.h"

// OpenGL for texture creation/draw
#if defined(_WIN32)
  #include <GL/gl.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif

#if defined(_WIN32)
  #include <windows.h>
  #include <shellapi.h>
#elif defined(__APPLE__)
  #include <TargetConditionals.h>
  #include <unistd.h>
#else
  #include <unistd.h>
#endif

namespace ui::about
{

  // Window state
  static XPLMWindowID g_aboutWindow = nullptr;
  static bool g_aboutMonitorRegistered = false;

  // QR image/texture state
  static GLuint g_qrTexture = 0;
  static int g_qrImgW = 0;
  static int g_qrImgH = 0;
  static bool g_qrTextureLoaded = false;

  // Last drawn image rectangle for hit-testing (window-local coordinates)
  static int g_qrLeft = 0, g_qrTop = 0, g_qrRight = 0, g_qrBottom = 0;
  // Track last mouse position for hover effect
  static int g_mouseX = -1, g_mouseY = -1;

  // License text area rect (window-local, last computed)
  static int g_licenseL = 0, g_licenseT = 0, g_licenseR = 0, g_licenseB = 0;

  // ---- License text wrapping/scroll state ----
  static std::vector<std::string> g_licenseWrappedLines;
  static int g_licenseWrapWidthCached = -1; // in pixels
  static int g_licenseScrollPx = 0;         // vertical scroll in pixels

  static void EnsureWrappedLicense(int max_width_px) {
    if (max_width_px <= 0) return;
    if (g_licenseWrapWidthCached == max_width_px && !g_licenseWrappedLines.empty()) return;

    g_licenseWrappedLines.clear();
    g_licenseWrapWidthCached = max_width_px;

    // Split the source license into logical lines
    std::string src(g_license_text, g_license_text + g_license_size);
    std::istringstream iss(src);
    std::string paragraph;
    while (std::getline(iss, paragraph, '\n')) {
      // Preserve blank lines
      if (paragraph.empty()) {
        g_licenseWrappedLines.emplace_back("");
        continue;
      }
      // Word-wrap this paragraph
      std::istringstream wss(paragraph);
      std::string word;
      std::string line;
      while (wss >> word) {
        if (line.empty()) {
          line = word;
        } else {
          std::ostringstream oss;
          oss << line << ' ' << word;
          std::string next = oss.str();
          if (static_cast<int>(XPLMMeasureString(xplmFont_Basic, next.c_str(), static_cast<int>(next.size()))) > max_width_px) {
            g_licenseWrappedLines.emplace_back(line);
            line = word;
          } else {
            line = next;
          }
        }
        // Handle very long words
        int word_px = static_cast<int>(XPLMMeasureString(xplmFont_Basic, word.c_str(), static_cast<int>(word.size())));
        if (word_px > max_width_px) {
          std::string chunk;
          for (char c : word) {
            std::string next = chunk + c;
            if (static_cast<int>(XPLMMeasureString(xplmFont_Basic, next.c_str(), static_cast<int>(next.size()))) > max_width_px) {
              if (!chunk.empty()) {
                g_licenseWrappedLines.emplace_back(chunk);
                chunk.clear();
              }
            }
            chunk.push_back(c);
          }
          if (!chunk.empty()) {
            line = chunk; // start a new line with a remainder
          }
        }
      }
      g_licenseWrappedLines.emplace_back(line);
    }
    // Reset scroll when wrapping changes
    g_licenseScrollPx = 0;
  }

  static void LoadQRTextureIfNeeded()
  {
    if (g_qrTextureLoaded) return;
    // Use embedded RGBA bytes generated at build time (from tools/embed_png.py)
    // Refer to globals defined by the embed script; do NOT redeclare inside namespace
    constexpr int w = (int)::g_qr_w;
    constexpr int h = (int)::g_qr_h;

    glGenTextures(1, &g_qrTexture);
    glBindTexture(GL_TEXTURE_2D, g_qrTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ::g_qr_rgba);

    g_qrImgW = w;
    g_qrImgH = h;
    g_qrTextureLoaded = true;
  }

  static void DestroyQRTexture() {
    if (g_qrTexture != 0) {
      glDeleteTextures(1, &g_qrTexture);
      g_qrTexture = 0;
    }
    g_qrTextureLoaded = false;
    g_qrImgW = g_qrImgH = 0;
  }

  // ReSharper disable once CppDFAConstantParameter
  static void OpenURLCrossPlatform(const char* url) {
#if defined(_WIN32)
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    // macOS: use the 'open' command
    pid_t pid = fork();
    if (pid == 0) {
      execlp("open", "open", url, (char*)nullptr);
      _exit(0);
    }
#else
    // Linux: try xdg-open
    pid_t pid = fork();
    if (pid == 0) {
      execlp("xdg-open", "xdg-open", url, static_cast<char*>(nullptr));
      _exit(0);
    }
#endif
  }

  // Forward declarations for window callbacks
  static void DrawAboutWindow(XPLMWindowID inWindowID, void *inRefcon);
  static int MouseAboutWindow(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void *inRefcon);
  static int CursorAboutWindow(XPLMWindowID inWindowID, int x, int y, void *inRefcon);
  static int WheelAboutWindow(XPLMWindowID inWindowID, int x, int y, int wheel, int clicks, void *inRefcon);
  static float AboutWindowMonitorCB(float, float, int, void *);

  void Show() {
    if (g_aboutWindow) return;

    int l, t, r, b;
    XPLMGetScreenBoundsGlobal(&l, &t, &r, &b);
    int width = 480;   // slightly narrower
    int height = 460;  // slightly shorter overall
    int left = (l + r - width) / 2;
    int top = (t + b + height) / 2;   // top coordinate
    int right = left + width;
    int bottom = top - height;

    XPLMCreateWindow_t params{};
    params.structSize = sizeof(params);
    params.visible = 1;
    params.drawWindowFunc = DrawAboutWindow;
    params.handleMouseClickFunc = MouseAboutWindow;
    params.handleRightClickFunc = nullptr;
    params.handleMouseWheelFunc = WheelAboutWindow;
    params.handleCursorFunc = CursorAboutWindow;
    params.handleKeyFunc = nullptr;
    params.refcon = nullptr;
    params.left = left;
    params.top = top;
    params.right = right;
    params.bottom = bottom;
    params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
    params.layer = xplm_WindowLayerFloatingWindows;
    params.handleKeyFunc = nullptr;

    g_aboutWindow = XPLMCreateWindowEx(&params);
    XPLMSetWindowResizingLimits(g_aboutWindow, width, height, width, height);

#ifdef XPLMSetWindowCloseButtonVisible
    XPLMSetWindowCloseButtonVisible(g_aboutWindow, 1);
#endif
    if (!g_aboutMonitorRegistered) {
      XPLMRegisterFlightLoopCallback(AboutWindowMonitorCB, -1.0f, nullptr);
      g_aboutMonitorRegistered = true;
    }
  }

  void Close() {
    if (g_aboutWindow) {
      XPLMDestroyWindow(g_aboutWindow);
      g_aboutWindow = nullptr;
    }
    if (g_aboutMonitorRegistered) {
      XPLMUnregisterFlightLoopCallback(AboutWindowMonitorCB, nullptr);
      g_aboutMonitorRegistered = false;
    }
    DestroyQRTexture();
  }

  static void DrawAboutWindow(XPLMWindowID inWindowID, void * /*inRefcon*/) {
    int l, t, r, b;
    XPLMGetWindowGeometry(inWindowID, &l, &t, &r, &b);
    int word_wrap_width = r - l - 40;
    // Title
    float white[3] = {1.0f, 1.0f, 1.0f};
    int line_h;
    int char_w;
    XPLMGetFontDimensions(xplmFont_Basic, &char_w, &line_h, nullptr);

    int title_y = t - 26; // tighten top padding a bit
    XPLMDrawString(white, l + 20, title_y, const_cast<char *>("IFR-1 Flight Controller Flexible Plugin"),
                   nullptr, xplmFont_Basic);

    // Body text
    const char *line1 =
      "Copyright 2025 Kyle D. Ross";

    const char *line2 =
      "This software is not affiliated with, endorsed by, or supported by Octavi GmbH or Laminar Research.  All trademarks are the property of their respective owners, and are used herein for reference only.";

    const char *line3 =
      "Your support is very much appreciated and keeps development moving forward.  Please visit https://buymeacoffee.com/kyledross by scanning or clicking the QR code below.  Thank you!";


    int text_y = title_y - 36;
    XPLMDrawString(white, l + 20, text_y, const_cast<char *>(line1), &word_wrap_width,
                   xplmFont_Basic);

    text_y -= (line_h + 8);
    XPLMDrawString(white, l + 20, text_y, const_cast<char *>(line2), &word_wrap_width,
                   xplmFont_Basic);

    text_y -= (line_h + 64);
    XPLMDrawString(white, l + 20, text_y, const_cast<char *>(line3), &word_wrap_width,
                   xplmFont_Basic);

    // Attempt to load the QR texture lazily
    LoadQRTextureIfNeeded();
    if (g_qrTexture != 0 && g_qrImgW > 0 && g_qrImgH > 0) {
      // Compute target size: width is a quarter of the window width
      const int window_w = r - l;
      const int content_w = window_w - 40; // left/right padding matches text
      const float target_w = static_cast<float>(window_w) * 0.22f; // a bit smaller
      const float scale = target_w / static_cast<float>(g_qrImgW);
      const int draw_w = static_cast<int>(static_cast<float>(g_qrImgW) * scale);
      const int draw_h = static_cast<int>(static_cast<float>(g_qrImgH) * scale);

      // Position: horizontally centered within content area, below third line with padding
      constexpr int pad_top = 48;
      const int left_content = l + 20;
      const int img_left = left_content + (content_w - draw_w) / 2;
      const int img_top = text_y - pad_top;
      const int img_right = img_left + draw_w;
      const int img_bottom = img_top - draw_h;

      // Save rect for click hit-test
      g_qrLeft = img_left;
      g_qrTop = img_top;
      g_qrRight = img_right;
      g_qrBottom = img_bottom;

      // Set graphics state for textured 2D draw
      XPLMSetGraphicsState(
        0 /*fog*/, 1 /*tex units*/, 0 /*lighting*/, 0 /*alpha testing*/,
        1 /*alpha blending*/, 0 /*depth testing*/, 0 /*depth writing*/);

      glBindTexture(GL_TEXTURE_2D, g_qrTexture);

      // Draw as textured quad in window-local coordinates
      glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f); glVertex2i(img_left,  img_top);
      glTexCoord2f(1.0f, 0.0f); glVertex2i(img_right, img_top);
      glTexCoord2f(1.0f, 1.0f); glVertex2i(img_right, img_bottom);
      glTexCoord2f(0.0f, 1.0f); glVertex2i(img_left,  img_bottom);
      glEnd();

      // Hover highlight if the mouse is over
      const bool hover = (g_mouseX >= g_qrLeft && g_mouseX <= g_qrRight &&
        g_mouseY >= g_qrBottom && g_mouseY <= g_qrTop);
      if (hover) {
        // Disable texturing and draw a thin white border
        XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2i(img_left-1,  img_top+1);
        glVertex2i(img_right+1, img_top+1);
        glVertex2i(img_right+1, img_bottom-1);
        glVertex2i(img_left-1,  img_bottom-1);
        glEnd();
      }
    } else {
      // Texture not available; clear hit-test rect
      g_qrLeft = g_qrTop = g_qrRight = g_qrBottom = 0;
    }

    // License section box and text
    constexpr int license_pad_top = 14;
    const int box_h = 7 * (line_h + 2);
    const int box_top = b + license_pad_top + box_h; // from bottom up
    const int box_left = l + 20;
    const int box_right = r - 20;
    const int box_bottom = box_top - box_h;
    g_licenseL = box_left; g_licenseT = box_top; g_licenseR = box_right; g_licenseB = box_bottom;

    // Border
    XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(box_left,  box_top);
    glVertex2i(box_right, box_top);
    glVertex2i(box_right, box_bottom);
    glVertex2i(box_left,  box_bottom);
    glEnd();

    // Removed instructional caption above the text area (unnecessary)

    // Compute visible lines and wrap text into width
    const int inner_left = box_left + 6;
    const int inner_right = box_right - 6;
    const int inner_top = box_top - 8;  // tighter top padding inside box
    const int inner_bottom = box_bottom + 6;
    const int wrap_px = inner_right - inner_left;
    EnsureWrappedLicense(wrap_px);

    // Compute how many lines fit
    const int lines_fit = std::max(0, (inner_top - inner_bottom) / line_h);
    // Convert scroll in px to the starting line
    int first_line = 0;
    if (line_h > 0) first_line = std::max(0, g_licenseScrollPx / line_h);
    int y = inner_top;
    for (int i = 0; i < lines_fit && (first_line + i) < static_cast<int>(g_licenseWrappedLines.size()); ++i) {
      const std::string& ln = g_licenseWrappedLines[first_line + i];
      XPLMDrawString(white, inner_left, y, const_cast<char*>(ln.c_str()), nullptr, xplmFont_Basic);
      y -= line_h;
    }
  }

  static int MouseAboutWindow(XPLMWindowID /*inWindowID*/, int x, int y,
                              XPLMMouseStatus inMouse, void* /*inRefcon*/) {
    // Update hover tracking coordinates
    g_mouseX = x;
    g_mouseY = y;

    if (inMouse == xplm_MouseDown) {
      // If click is inside QR code, open URL
      if (x >= g_qrLeft && x <= g_qrRight && y >= g_qrBottom && y <= g_qrTop) {
        OpenURLCrossPlatform("https://buymeacoffee.com/kyledross");
        return 1;
      }
    }
    return 0;
  }

  static int CursorAboutWindow(XPLMWindowID /*inWindowID*/, int x, int y, void* /*inRefcon*/) {
    g_mouseX = x;
    g_mouseY = y;
    // Ensure the cursor is shown; pre-refactor behavior used the default cursor
    return xplm_CursorDefault;
  }

  static int WheelAboutWindow(XPLMWindowID /*inWindowID*/, int x, int y, int /*wheel*/, int clicks, void* /*inRefcon*/) {
    // Only scroll when inside the license text area
    if (x >= g_licenseL && x <= g_licenseR && y >= g_licenseB && y <= g_licenseT) {
      int char_w, line_h;
      XPLMGetFontDimensions(xplmFont_Basic, &char_w, &line_h, nullptr);

      // Recompute the same inner box metrics used by DrawAboutWindow()
      const int inner_left = g_licenseL + 6;
      const int inner_right = g_licenseR - 6;
      const int inner_top = g_licenseT - 8;
      const int inner_bottom = g_licenseB + 6;
      const int wrap_px = inner_right - inner_left;

      // Make sure wrapped content exists for the current width
      EnsureWrappedLicense(wrap_px);

      // How many lines can be shown at once?
      const int lines_fit = (line_h > 0) ? std::max(0, (inner_top - inner_bottom) / line_h) : 0;

      // Compute maximum scroll (in pixels) so the last page aligns to the bottom
      const int total_lines = static_cast<int>(g_licenseWrappedLines.size());
      const int max_first_line = std::max(0, total_lines - lines_fit);
      const int max_scroll_px = (line_h > 0) ? (max_first_line * line_h) : 0;

      // Apply wheel delta then clamp to [0, max_scroll_px]
      g_licenseScrollPx -= clicks * line_h;
      g_licenseScrollPx = std::clamp(g_licenseScrollPx, 0, max_scroll_px);

      return 1;
    }
    return 0;
  }

  static float AboutWindowMonitorCB(float /*inElapsedSinceLastCall*/, float /*inElapsedTimeSinceLastFlightLoop*/, int /*inCounter*/, void* /*inRefcon*/) {
    if (g_aboutWindow) {
      // If the window was closed externally, our pointer becomes invalid; detect the "hidden" state
      int vis = XPLMGetWindowIsVisible(g_aboutWindow);
      if (!vis) {
        // Make sure we clean up and stop monitoring
        Close();
      }
    }
    return -1.0f; // keep being called every frame
  }

}
