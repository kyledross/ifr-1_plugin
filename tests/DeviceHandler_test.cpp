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
    MockXPlaneSDK() {
        ON_CALL(*this, Log(::testing::_, ::testing::_)).WillByDefault(::testing::Return());
        ON_CALL(*this, GetLogLevel()).WillByDefault(::testing::Return(LogLevel::Info));
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
    MOCK_METHOD(void, PlaySound, (const std::string& path), (override));
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
    
    EXPECT_CALL(mockHw, IsConnected())
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockHw, Connect(IFR1::VENDOR_ID, IFR1::PRODUCT_ID)).WillOnce(Return(true));
    
    // ClearLEDs will be called on connect
    EXPECT_CALL(mockHw, Write(_, 2)).WillOnce(Return(2));
    
    // Read will be called in the loop
    EXPECT_CALL(mockHw, Read(_, _, _)).WillRepeatedly(Return(0));

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

TEST(DeviceHandlerTest, Update_ResetsShiftedOnModeChange) {
    MockHardwareManager mockHw;
    MockXPlaneSDK mockSdk;
    EventProcessor eventProc(mockSdk);
    OutputProcessor outputProc(mockSdk);
    DeviceHandler handler(mockHw, eventProc, outputProc, mockSdk);
    
    nlohmann::json config = {
        {"modes", {
            {"com1", {
                {"outer-knob", {{"rotate-clockwise", {{"type", "command"}, {"value", "com1_cmd"}}}}}
            }},
            {"hdg", {
                {"outer-knob", {{"rotate-clockwise", {{"type", "command"}, {"value", "hdg_cmd"}}}}}
            }},
            {"com2", {
                {"outer-knob", {{"rotate-clockwise", {{"type", "command"}, {"value", "com2_cmd"}}}}}
            }}
        }}
    };
    
    EXPECT_CALL(mockHw, IsConnected()).WillRepeatedly(Return(true));
    
    // 1. Set mode to COM1 and long press INNER_KNOB to shift
    uint8_t reportShiftPressed[IFR1::HID_REPORT_SIZE] = {0, 0, 0x02, 0, 0, 0, 0, 0, 0}; 
    EXPECT_CALL(mockHw, Read(_, _, _))
        .WillOnce(::testing::Invoke([&](uint8_t* buf, size_t len, int t) {
            std::memcpy(buf, reportShiftPressed, IFR1::HID_REPORT_SIZE);
            return IFR1::HID_REPORT_SIZE;
        }))
        .WillOnce(Return(0));
    handler.Update(config, 0.0f);
    
    // Trigger long press (0.5s)
    EXPECT_CALL(mockHw, Read(_, _, _)).WillRepeatedly(Return(0));
    handler.Update(config, 0.6f); 
    
    // Now shifted should be true. Verify by rotating outer knob (should trigger hdg_cmd)
    uint8_t reportRotate[IFR1::HID_REPORT_SIZE] = {0, 0, 0, 0, 0, 1, 0, 0, 0}; 
    EXPECT_CALL(mockHw, Read(_, _, _))
        .WillOnce(::testing::Invoke([&](uint8_t* buf, size_t len, int t) {
            std::memcpy(buf, reportRotate, IFR1::HID_REPORT_SIZE);
            return IFR1::HID_REPORT_SIZE;
        }))
        .WillOnce(Return(0));
    
    void* hdgCmd = reinterpret_cast<void*>(0x1);
    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("hdg_cmd"))).WillOnce(Return(hdgCmd));
    EXPECT_CALL(mockSdk, CommandOnce(hdgCmd));
    handler.Update(config, 0.7f);

    // 2. Change mode to COM2
    uint8_t reportModeChange[IFR1::HID_REPORT_SIZE] = {0, 0, 0, 0, 0, 0, 0, static_cast<uint8_t>(IFR1::Mode::COM2), 0};
    EXPECT_CALL(mockHw, Read(_, _, _))
        .WillOnce(::testing::Invoke([&](uint8_t* buf, size_t len, int t) {
            std::memcpy(buf, reportModeChange, IFR1::HID_REPORT_SIZE);
            return IFR1::HID_REPORT_SIZE;
        }))
        .WillOnce(Return(0));
    handler.Update(config, 0.8f);

    // 3. Rotate outer knob again. It should trigger com2_cmd, NOT its shifted version (which would be baro)
    uint8_t reportRotate2[IFR1::HID_REPORT_SIZE] = {0, 0, 0, 0, 0, 1, 0, static_cast<uint8_t>(IFR1::Mode::COM2), 0}; 
    EXPECT_CALL(mockHw, Read(_, _, _))
        .WillOnce(::testing::Invoke([&](uint8_t* buf, size_t len, int t) {
            std::memcpy(buf, reportRotate2, IFR1::HID_REPORT_SIZE);
            return IFR1::HID_REPORT_SIZE;
        }))
        .WillOnce(Return(0));

    void* com2Cmd = reinterpret_cast<void*>(0x2);
    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("com2_cmd"))).WillOnce(Return(com2Cmd));
    EXPECT_CALL(mockSdk, CommandOnce(com2Cmd));
    
    handler.Update(config, 0.9f);
}

TEST(DeviceHandlerTest, ClearLEDs_SendsZeroReportAndResetsState) {
    MockHardwareManager mockHw;
    MockXPlaneSDK mockSdk;
    EventProcessor eventProc(mockSdk);
    OutputProcessor outputProc(mockSdk);
    DeviceHandler handler(mockHw, eventProc, outputProc, mockSdk);
    
    EXPECT_CALL(mockHw, IsConnected()).WillRepeatedly(Return(true));
    
    // Expect a write with HID_LED_REPORT_ID and 0
    EXPECT_CALL(mockHw, Write(_, 2))
        .WillOnce(::testing::Invoke([](const uint8_t* data, size_t len) {
            EXPECT_EQ(data[0], IFR1::HID_LED_REPORT_ID);
            EXPECT_EQ(data[1], 0);
            return 2;
        }));
    
    handler.ClearLEDs();
}

TEST(DeviceHandlerTest, Update_PlaysSoundOnLongPress) {
    MockHardwareManager mockHw;
    MockXPlaneSDK mockSdk;
    
    EXPECT_CALL(mockSdk, GetSystemPath()).WillRepeatedly(Return("/xplane/"));
    
    EventProcessor eventProc(mockSdk);
    OutputProcessor outputProc(mockSdk);
    DeviceHandler handler(mockHw, eventProc, outputProc, mockSdk);
    
    nlohmann::json config = {
        {"modes", {
            {"com1", {
                {"swap", {
                    {"long-press", {{"type", "command"}, {"value", "long_cmd"}}}
                }}
            }}
        }}
    };
    
    EXPECT_CALL(mockHw, IsConnected()).WillRepeatedly(Return(true));
    
    // 1. Press button (swap is bit 0 of byte 2)
    uint8_t pressReport[IFR1::HID_REPORT_SIZE] = {0, 0, 0x01, 0, 0, 0, 0, 0, 0}; 
    EXPECT_CALL(mockHw, Read(_, _, _))
        .WillOnce(::testing::Invoke([&](uint8_t* buf, size_t len, int timeout) {
            std::memcpy(buf, pressReport, IFR1::HID_REPORT_SIZE);
            return IFR1::HID_REPORT_SIZE;
        }))
        .WillOnce(Return(0));
    
    handler.Update(config, 0.0f);
    
    // 2. Advance time for long press
    EXPECT_CALL(mockHw, Read(_, _, _)).WillRepeatedly(Return(0));
    
    void* dummyCmd = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("long_cmd"))).WillOnce(Return(dummyCmd));
    EXPECT_CALL(mockSdk, CommandOnce(dummyCmd));
    EXPECT_CALL(mockSdk, PlaySound(::testing::StrEq("/xplane/Resources/sounds/systems/click.wav")));
    
    handler.Update(config, 0.4f);
}
