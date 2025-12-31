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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "SettingsManager.h"
#include <filesystem>
#include <fstream>

class MockXPlaneSDKForSettings : public IXPlaneSDK {
public:
    MOCK_METHOD(void, Log, (LogLevel level, const char* string), (override));
    MOCK_METHOD(void, SetLogLevel, (LogLevel level), (override));
    MOCK_METHOD(LogLevel, GetLogLevel, (), (const, override));
    MOCK_METHOD(float, GetElapsedTime, (), (override));
    MOCK_METHOD(std::string, GetSystemPath, (), (override));
    MOCK_METHOD(bool, FileExists, (const std::string& path), (override));
    MOCK_METHOD(void, PlaySound, (const std::string& path), (override));
    MOCK_METHOD(void, DrawString, (const float color[4], int x, int y, const char* string), (override));
    MOCK_METHOD(void, DrawRectangle, (const float color[4], int l, int t, int r, int b), (override));
    MOCK_METHOD(void, DrawRectangleOutline, (const float color[4], int l, int t, int r, int b), (override));
    MOCK_METHOD(int, MeasureString, (const char* string), (override));
    MOCK_METHOD(int, GetFontHeight, (), (override));
    MOCK_METHOD(void, GetScreenSize, (int* outWidth, int* outHeight), (override));
    MOCK_METHOD(void*, CreateWindowEx, (const WindowCreateParams& params), (override));
    MOCK_METHOD(void, DestroyWindow, (void* windowId), (override));
    MOCK_METHOD(void, SetWindowVisible, (void* windowId, int visible), (override));
    MOCK_METHOD(void, SetWindowGeometry, (void* windowId, int left, int top, int right, int bottom), (override));
    MOCK_METHOD(void, GetWindowGeometry, (void* windowId, int* outLeft, int* outTop, int* outRight, int* outBottom), (override));
    MOCK_METHOD(void*, FindDataRef, (const char* name), (override));
    MOCK_METHOD(int, GetDataRefTypes, (void* dataRef), (override));
    MOCK_METHOD(int, GetDatai, (void* dataRef), (override));
    MOCK_METHOD(void, SetDatai, (void* dataRef, int value), (override));
    MOCK_METHOD(float, GetDataf, (void* dataRef), (override));
    MOCK_METHOD(void, SetDataf, (void* dataRef, float value), (override));
    MOCK_METHOD(int, GetDataiArray, (void* dataRef, int index), (override));
    MOCK_METHOD(void, SetDataiArray, (void* dataRef, int value, int index), (override));
    MOCK_METHOD(float, GetDatafArray, (void* dataRef, int index), (override));
    MOCK_METHOD(void, SetDatafArray, (void* dataRef, float value, int index), (override));
    MOCK_METHOD(int, GetDatab, (void* dataRef, void* outData, int offset, int maxLength), (override));
    MOCK_METHOD(void*, FindCommand, (const char* name), (override));
    MOCK_METHOD(void, CommandOnce, (void* commandRef), (override));
    MOCK_METHOD(void, CommandBegin, (void* commandRef), (override));
    MOCK_METHOD(void, CommandEnd, (void* commandRef), (override));
};

TEST(SettingsManagerTest, LoadsDefaultsWhenFileNotFound) {
    MockXPlaneSDKForSettings sdk;
    std::filesystem::path testPath = "non_existent_settings.json";
    if (std::filesystem::exists(testPath)) std::filesystem::remove(testPath);
    
    EXPECT_CALL(sdk, Log(::testing::_, ::testing::_)).WillRepeatedly(::testing::Return());

    SettingsManager manager(testPath);
    manager.Load(sdk);
    
    EXPECT_FALSE(manager.GetBool("on-screen-mode-display"));
    EXPECT_TRUE(std::filesystem::exists(testPath));
    
    std::filesystem::remove(testPath);
}

TEST(SettingsManagerTest, SavesAndLoadsSettings) {
    MockXPlaneSDKForSettings sdk;
    std::filesystem::path testPath = "test_settings.json";
    if (std::filesystem::exists(testPath)) std::filesystem::remove(testPath);

    EXPECT_CALL(sdk, Log(::testing::_, ::testing::_)).WillRepeatedly(::testing::Return());

    {
        SettingsManager manager(testPath);
        manager.SetBool("on-screen-mode-display", true);
        manager.Save(sdk);
    }
    
    {
        SettingsManager manager(testPath);
        manager.Load(sdk);
        EXPECT_TRUE(manager.GetBool("on-screen-mode-display"));
    }
    
    std::filesystem::remove(testPath);
}

TEST(SettingsManagerTest, HandlesInvalidJson) {
    MockXPlaneSDKForSettings sdk;
    std::filesystem::path testPath = "invalid_settings.json";
    
    std::ofstream file(testPath);
    file << "{ invalid json [";
    file.close();

    EXPECT_CALL(sdk, Log(LogLevel::Error, ::testing::_)).Times(::testing::AtLeast(1));
    EXPECT_CALL(sdk, Log(LogLevel::Info, ::testing::_)).WillRepeatedly(::testing::Return());

    SettingsManager manager(testPath);
    manager.Load(sdk);
    
    // Should still have defaults
    EXPECT_FALSE(manager.GetBool("on-screen-mode-display"));
    
    std::filesystem::remove(testPath);
}
