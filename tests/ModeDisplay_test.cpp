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
#include "ModeDisplay.h"
#include "XPlaneSDK.h"

namespace {

class MockXPlaneSDK : public IXPlaneSDK {
public:
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
};

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

TEST(ModeDisplayTest, AnimationSequence) {
    NiceMock<MockXPlaneSDK> sdk;
    
    IXPlaneSDK::WindowCreateParams capturedParams{};
    EXPECT_CALL(sdk, CreateWindowEx(_)).WillOnce([&](const IXPlaneSDK::WindowCreateParams& params){
        capturedParams = params;
        return reinterpret_cast<void*>(0x1234);
    });

    EXPECT_CALL(sdk, MeasureString(_)).WillRepeatedly(Return(100));
    EXPECT_CALL(sdk, GetFontHeight()).WillRepeatedly(Return(20));
    EXPECT_CALL(sdk, GetWindowGeometry(_, _, _, _, _)).WillRepeatedly([](void*, int* l, int* t, int* r, int* b){
        *l = 0; *t = 50; *r = 150; *b = 0;
    });
    EXPECT_CALL(sdk, DestroyWindow(_)).WillOnce(Return());

    ModeDisplay display(sdk);

    display.ShowMessage("TEST", 0.0f);

    // Fade in halfway (0.125s / 0.25s = 0.5 opacity)
    display.Update(0.125f);
    EXPECT_CALL(sdk, DrawRectangle(_, _, _, _, _)).Times(1);
    EXPECT_CALL(sdk, DrawRectangleOutline(_, _, _, _, _)).Times(1);
    EXPECT_CALL(sdk, DrawString(_, _, _, _)).Times(1);
    capturedParams.drawCallback(reinterpret_cast<void*>(0x1234), capturedParams.refcon);

    // Stay (1.0s)
    display.Update(1.0f);
    EXPECT_CALL(sdk, DrawRectangle(_, _, _, _, _)).Times(1);
    EXPECT_CALL(sdk, DrawRectangleOutline(_, _, _, _, _)).Times(1);
    EXPECT_CALL(sdk, DrawString(_, _, _, _)).Times(1);
    capturedParams.drawCallback(reinterpret_cast<void*>(0x1234), capturedParams.refcon);

    // Fade out halfway (2.75s is 0.5s into 1s fade out which starts at 2.25s)
    display.Update(2.75f);
    EXPECT_CALL(sdk, DrawRectangle(_, _, _, _, _)).Times(1);
    EXPECT_CALL(sdk, DrawRectangleOutline(_, _, _, _, _)).Times(1);
    EXPECT_CALL(sdk, DrawString(_, _, _, _)).Times(1);
    capturedParams.drawCallback(reinterpret_cast<void*>(0x1234), capturedParams.refcon);

    // Done
    display.Update(3.5f);
    EXPECT_CALL(sdk, DrawRectangle(_, _, _, _, _)).Times(0);
    EXPECT_CALL(sdk, DrawString(_, _, _, _)).Times(0);
    capturedParams.drawCallback(reinterpret_cast<void*>(0x1234), capturedParams.refcon);
}

TEST(ModeDisplayTest, RestartsOnNewMessage) {
    NiceMock<MockXPlaneSDK> sdk;
    ModeDisplay display(sdk);

    display.ShowMessage("FIRST", 0.0f);
    display.Update(0.3f); // Fully visible

    display.ShowMessage("SECOND", 0.6f);
    display.Update(0.7f); // 0.1s into new fade in (opacity 0.4)
}
}
