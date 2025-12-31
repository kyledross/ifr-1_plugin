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

// Dataref types from XPLMDataAccess.h
enum class DataRefType {
    Unknown = 0,
    Int = 1,
    Float = 2,
    Double = 4,
    FloatArray = 8,
    IntArray = 16,
    Data = 32
};

enum class LogLevel {
    Error = 0,
    Info = 1,
    Verbose = 2
};

/**
 * @brief Interface for X-Plane SDK functions to allow mocking in unit tests.
 */
class IXPlaneSDK {
public:
    virtual ~IXPlaneSDK() = default;

    // Data Access
    virtual void* FindDataRef(const char* name) = 0;
    virtual int GetDataRefTypes(void* dataRef) = 0;
    virtual int GetDatai(void* dataRef) = 0;
    virtual void SetDatai(void* dataRef, int value) = 0;
    virtual float GetDataf(void* dataRef) = 0;
    virtual void SetDataf(void* dataRef, float value) = 0;
    virtual int GetDataiArray(void* dataRef, int index) = 0;
    virtual void SetDataiArray(void* dataRef, int value, int index) = 0;
    virtual float GetDatafArray(void* dataRef, int index) = 0;
    virtual void SetDatafArray(void* dataRef, float value, int index) = 0;
    virtual int GetDatab(void* dataRef, void* outData, int offset, int maxLength) = 0;

    // Commands
    virtual void* FindCommand(const char* name) = 0;
    virtual void CommandOnce(void* commandRef) = 0;
    virtual void CommandBegin(void* commandRef) = 0;
    virtual void CommandEnd(void* commandRef) = 0;

    // Utilities
    virtual void Log(LogLevel level, const char* string) = 0;
    virtual void SetLogLevel(LogLevel level) = 0;
    virtual LogLevel GetLogLevel() const = 0;
    virtual float GetElapsedTime() = 0;
    virtual std::string GetSystemPath() = 0;
    virtual bool FileExists(const std::string& path) = 0;

    // Sound
    virtual void PlaySound(const std::string& path) = 0;

    // Drawing
    virtual void DrawString(const float color[4], int x, int y, const char* string) = 0;
    virtual void DrawRectangle(const float color[4], int l, int t, int r, int b) = 0;
    virtual void DrawRectangleOutline(const float color[4], int l, int t, int r, int b) = 0;
    virtual int MeasureString(const char* string) = 0;
    virtual int GetFontHeight() = 0;
    virtual void GetScreenSize(int* outWidth, int* outHeight) = 0;

    // Windowing
    struct WindowCreateParams {
        int left, top, right, bottom;
        int visible;
        void (*drawCallback)(void* windowId, void* refcon);
        void* refcon;
    };
    virtual void* CreateWindowEx(const WindowCreateParams& params) = 0;
    virtual void DestroyWindow(void* windowId) = 0;
    virtual void SetWindowVisible(void* windowId, int visible) = 0;
    virtual void SetWindowGeometry(void* windowId, int left, int top, int right, int bottom) = 0;
    virtual void GetWindowGeometry(void* windowId, int* outLeft, int* outTop, int* outRight, int* outBottom) = 0;
};

#include <memory>
#include <string>
std::unique_ptr<IXPlaneSDK> CreateXPlaneSDK();
