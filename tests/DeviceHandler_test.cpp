#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "DeviceHandler.h"
#include "IHardwareManager.h"
#include "XPlaneSDK.h"
#include "IFR1Protocol.h"

class MockHardwareManager : public IHardwareManager {
public:
    MOCK_METHOD(bool, Connect, (uint16_t vendorId, uint16_t productId), (override));
    MOCK_METHOD(void, Disconnect, (), (override));
    MOCK_METHOD(bool, IsConnected, (), (const, override));
    MOCK_METHOD(int, Read, (uint8_t* data, size_t length, int timeoutMs), (override));
    MOCK_METHOD(int, Write, (const uint8_t* data, size_t length), (override));
};

class MockXPlaneSDK : public IXPlaneSDK {
public:
    MOCK_METHOD(void*, FindDataRef, (const char* name), (override));
    MOCK_METHOD(int, GetDatai, (void* dataRef), (override));
    MOCK_METHOD(void, SetDatai, (void* dataRef, int value), (override));
    MOCK_METHOD(float, GetDataf, (void* dataRef), (override));
    MOCK_METHOD(void, SetDataf, (void* dataRef, float value), (override));
    MOCK_METHOD(int, GetDatab, (void* dataRef, void* outData, int offset, int maxLength), (override));
    MOCK_METHOD(void*, FindCommand, (const char* name), (override));
    MOCK_METHOD(void, CommandOnce, (void* commandRef), (override));
    MOCK_METHOD(void, CommandBegin, (void* commandRef), (override));
    MOCK_METHOD(void, CommandEnd, (void* commandRef), (override));
    MOCK_METHOD(void, DebugString, (const char* string), (override));
    MOCK_METHOD(float, GetElapsedTime, (), (override));
};

using ::testing::Return;
using ::testing::_;
using ::testing::SetArgPointee;
using ::testing::DoAll;

TEST(DeviceHandlerTest, Update_ConnectsWhenDisconnected) {
    MockHardwareManager mockHw;
    MockXPlaneSDK mockSdk;
    EventProcessor eventProc(mockSdk);
    OutputProcessor outputProc(mockSdk);
    DeviceHandler handler(mockHw, eventProc, outputProc, mockSdk);
    
    EXPECT_CALL(mockHw, IsConnected()).WillOnce(Return(false));
    EXPECT_CALL(mockHw, Connect(IFR1::VENDOR_ID, IFR1::PRODUCT_ID)).WillOnce(Return(true));
    EXPECT_CALL(mockSdk, DebugString(_));
    
    nlohmann::json config;
    handler.Update(config, 0.0f);
}

TEST(DeviceHandlerTest, Update_ProcessesKnobRotation) {
    MockHardwareManager mockHw;
    MockXPlaneSDK mockSdk;
    EventProcessor eventProc(mockSdk);
    OutputProcessor outputProc(mockSdk);
    DeviceHandler handler(mockHw, eventProc, outputProc, mockSdk);
    
    nlohmann::json config = {
        {"modes", {
            {"com1", {
                {"outer-knob", {
                    {"rotate-clockwise", {{"type", "command"}, {"value", "test_cmd"}}}
                }}
            }}
        }}
    };
    
    EXPECT_CALL(mockHw, IsConnected()).WillRepeatedly(Return(true));
    EXPECT_CALL(mockSdk, DebugString(_)).WillRepeatedly(Return());
    
    // Simulate HID report with outer knob rotation = 1
    // data[5] is outer knob, data[7] is mode
    uint8_t report[IFR1::HID_REPORT_SIZE] = {0, 0, 0, 0, 0, 1, 0, 0, 0}; 
    
    EXPECT_CALL(mockHw, Read(_, _, _))
        .WillOnce(::testing::Invoke([&](uint8_t* buf, size_t len, int timeout) {
            std::memcpy(buf, report, IFR1::HID_REPORT_SIZE);
            return IFR1::HID_REPORT_SIZE;
        }))
        .WillOnce(Return(0)); // Second call returns 0 to stop loop
    
    void* dummyCmd = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("test_cmd"))).WillOnce(Return(dummyCmd));
    EXPECT_CALL(mockSdk, CommandOnce(dummyCmd));
    
    handler.Update(config, 0.0f);
}

TEST(DeviceHandlerTest, Update_ProcessesShortPress) {
    MockHardwareManager mockHw;
    MockXPlaneSDK mockSdk;
    EventProcessor eventProc(mockSdk);
    OutputProcessor outputProc(mockSdk);
    DeviceHandler handler(mockHw, eventProc, outputProc, mockSdk);
    
    nlohmann::json config = {
        {"modes", {
            {"com1", {
                {"swap", {
                    {"short-press", {{"type", "command"}, {"value", "swap_cmd"}}}
                }}
            }}
        }}
    };
    
    EXPECT_CALL(mockHw, IsConnected()).WillRepeatedly(Return(true));
    EXPECT_CALL(mockSdk, DebugString(_)).WillRepeatedly(Return());
    
    // Frame 1: Button pressed
    uint8_t reportPressed[IFR1::HID_REPORT_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    reportPressed[2] |= (1 << (IFR1::BitPosition::SWAP - 1));
    
    EXPECT_CALL(mockHw, Read(_, _, _))
        .WillOnce(::testing::Invoke([&](uint8_t* buf, size_t len, int t) {
            std::memcpy(buf, reportPressed, IFR1::HID_REPORT_SIZE);
            return IFR1::HID_REPORT_SIZE;
        }))
        .WillOnce(Return(0));
    
    handler.Update(config, 0.0f);
    
    // Frame 2: Button released
    uint8_t reportReleased[IFR1::HID_REPORT_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_CALL(mockHw, Read(_, _, _))
        .WillOnce(::testing::Invoke([&](uint8_t* buf, size_t len, int t) {
            std::memcpy(buf, reportReleased, IFR1::HID_REPORT_SIZE);
            return IFR1::HID_REPORT_SIZE;
        }))
        .WillOnce(Return(0));
    
    void* dummyCmd = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("swap_cmd"))).WillOnce(Return(dummyCmd));
    EXPECT_CALL(mockSdk, CommandOnce(dummyCmd));
    
    handler.Update(config, 0.1f);
}
