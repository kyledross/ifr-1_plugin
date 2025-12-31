#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "OutputProcessor.h"
#include "XPlaneSDK.h"
#include "IFR1Protocol.h"

class MockXPlaneSDK : public IXPlaneSDK {
public:
    MockXPlaneSDK() {
        ON_CALL(*this, Log(::testing::_, ::testing::_)).WillByDefault(::testing::Return());
        ON_CALL(*this, GetLogLevel()).WillByDefault(::testing::Return(LogLevel::Info));
        ON_CALL(*this, FileExists(::testing::_)).WillByDefault(::testing::Return(true));
    }

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

using ::testing::Return;
using ::testing::StrEq;

TEST(OutputProcessorTest, EvaluateLEDs_ReturnsOffIfNoConfig) {
    MockXPlaneSDK mockSdk;
    OutputProcessor processor(mockSdk);
    nlohmann::json config;
    
    EXPECT_EQ(processor.EvaluateLEDs(config, 0.0f), IFR1::LEDMask::OFF);
}

TEST(OutputProcessorTest, EvaluateLEDs_SetsSolidBit) {
    MockXPlaneSDK mockSdk;
    OutputProcessor processor(mockSdk);
    
    nlohmann::json config = {
        {"output", {
            {"ap", {
                {"conditions", {
                    {{"dataref", "sim/cockpit/autopilot/autopilot_mode"}, {"min", 2.0}, {"max", 2.0}, {"mode", "solid"}}
                }}
            }}
        }}
    };
    
    void* dummyDr = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit/autopilot/autopilot_mode")))
        .WillOnce(Return(dummyDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(dummyDr)).WillOnce(Return(2)); // xplmType_Float = 2
    EXPECT_CALL(mockSdk, GetDataf(dummyDr)).WillOnce(Return(2.0f));
    
    uint8_t bits = processor.EvaluateLEDs(config, 0.0f);
    EXPECT_EQ(bits, IFR1::LEDMask::AP);
}

TEST(OutputProcessorTest, EvaluateLEDs_Blinks) {
    MockXPlaneSDK mockSdk;
    OutputProcessor processor(mockSdk);
    
    nlohmann::json config = {
        {"output", {
            {"alt", {
                {"conditions", {
                    {{"dataref", "sim/cockpit2/autopilot/altitude_mode"}, {"min", 5.0}, {"max", 5.0}, {"mode", "blink"}, {"blink-rate", 1.0}}
                }}
            }}
        }}
    };
    
    void* dummyDr = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit2/autopilot/altitude_mode")))
        .Times(2)
        .WillRepeatedly(Return(dummyDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(dummyDr))
        .Times(2)
        .WillRepeatedly(Return(2)); // xplmType_Float = 2
    EXPECT_CALL(mockSdk, GetDataf(dummyDr))
        .Times(2)
        .WillRepeatedly(Return(5.0f));
    
    // 1 Hz blink -> 0.0s ON, 0.5s OFF
    EXPECT_EQ(processor.EvaluateLEDs(config, 0.0f), IFR1::LEDMask::ALT);
    EXPECT_EQ(processor.EvaluateLEDs(config, 0.5f), IFR1::LEDMask::OFF);
}

TEST(OutputProcessorTest, EvaluateLEDs_BitTest) {
    MockXPlaneSDK mockSdk;
    OutputProcessor processor(mockSdk);
    
    nlohmann::json config = {
        {"output", {
            {"hdg", {
                {"conditions", {
                    {{"dataref", "sim/cockpit/autopilot/autopilot_state"}, {"bit", 1}, {"mode", "solid"}}
                }}
            }}
        }}
    };
    
    void* dummyDr = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit/autopilot/autopilot_state")))
        .WillOnce(Return(dummyDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(dummyDr)).WillOnce(Return(1)); // xplmType_Int = 1
    // Bit 1 means (1 << 1) = 2
    EXPECT_CALL(mockSdk, GetDatai(dummyDr)).WillOnce(Return(2));
    
    uint8_t bits = processor.EvaluateLEDs(config, 0.0f);
    EXPECT_EQ(bits, IFR1::LEDMask::HDG);
}

TEST(OutputProcessorTest, EvaluateLEDs_IntDatarefWithMinMax) {
    MockXPlaneSDK mockSdk;
    OutputProcessor processor(mockSdk);
    
    nlohmann::json config = {
        {"output", {
            {"ap", {
                {"conditions", {
                    {{"dataref", "sim/cockpit/autopilot/autopilot_mode"}, {"min", 2.0}, {"max", 2.0}, {"mode", "solid"}}
                }}
            }}
        }}
    };
    
    void* dummyDr = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit/autopilot/autopilot_mode")))
        .WillOnce(Return(dummyDr));
    
    // Simulate it being an INT dataref
    EXPECT_CALL(mockSdk, GetDataRefTypes(dummyDr)).WillOnce(Return(1)); // xplmType_Int = 1
    EXPECT_CALL(mockSdk, GetDatai(dummyDr)).WillOnce(Return(2));
    
    uint8_t bits = processor.EvaluateLEDs(config, 0.0f);
    EXPECT_EQ(bits, IFR1::LEDMask::AP);
}
