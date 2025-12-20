#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "EventProcessor.h"
#include "XPlaneSDK.h"
#include <nlohmann/json.hpp>
#include <string>

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
    MOCK_METHOD(int, GetDatab, (void* dataRef, void* outData, int offset, int maxLength), (override));
    MOCK_METHOD(void*, FindCommand, (const char* name), (override));
    MOCK_METHOD(void, CommandOnce, (void* commandRef), (override));
    MOCK_METHOD(void, CommandBegin, (void* commandRef), (override));
    MOCK_METHOD(void, CommandEnd, (void* commandRef), (override));
    MOCK_METHOD(void, Log, (LogLevel level, const char* string), (override));
    MOCK_METHOD(void, SetLogLevel, (LogLevel level), (override));
    MOCK_METHOD(LogLevel, GetLogLevel, (), (const, override));
    MOCK_METHOD(float, GetElapsedTime, (), (override));
};

using ::testing::Return;
using ::testing::NiceMock;

TEST(EventProcessorTest, ProcessEvent_CallsCommandOnce) {
    MockXPlaneSDK mockSdk;
    EventProcessor processor(mockSdk);

    nlohmann::json config = {
        {"modes", {
            {"com1", {
                {"swap", {
                    {"short-press", {
                        {"type", "command"},
                        {"value", "sim/radios/com1_standy_flip"}
                    }}
                }}
            }}
        }}
    };

    void* dummyCmd = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("sim/radios/com1_standy_flip")))
        .WillOnce(Return(dummyCmd));
    EXPECT_CALL(mockSdk, CommandOnce(dummyCmd));

    processor.ProcessEvent(config, "com1", "swap", "short-press");
}

TEST(EventProcessorTest, ProcessEvent_CallsSetDataf) {
    MockXPlaneSDK mockSdk;
    EventProcessor processor(mockSdk);

    nlohmann::json config = {
        {"modes", {
            {"com1", {
                {"swap", {
                    {"long-press", {
                        {"type", "dataref-set"},
                        {"value", "sim/cockpit2/radios/actuators/com1_standby_frequency_hz_833"},
                        {"adjustment", 121500}
                    }}
                }}
            }}
        }}
    };

    void* dummyDr = reinterpret_cast<void*>(0x5678);
    EXPECT_CALL(mockSdk, FindDataRef(::testing::StrEq("sim/cockpit2/radios/actuators/com1_standby_frequency_hz_833")))
        .WillOnce(Return(dummyDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(dummyDr)).WillOnce(Return(2)); // xplmType_Float = 2
    EXPECT_CALL(mockSdk, SetDataf(dummyDr, 121500.0f));

    processor.ProcessEvent(config, "com1", "swap", "long-press");
}

TEST(EventProcessorTest, ProcessEvent_CallsDatarefAdjustWrap) {
    MockXPlaneSDK mockSdk;
    EventProcessor processor(mockSdk);

    nlohmann::json config = {
        {"modes", {
            {"hdg", {
                {"inner-knob", {
                    {"rotate-clockwise", {
                        {"type", "dataref-adjust"},
                        {"value", "sim/cockpit/autopilot/heading_mag"},
                        {"adjustment", 1.0},
                        {"min", 0.0},
                        {"max", 359.0},
                        {"limit-type", "wrap"}
                    }}
                }}
            }}
        }}
    };

    void* dummyDr = reinterpret_cast<void*>(0x9ABC);
    EXPECT_CALL(mockSdk, FindDataRef(::testing::StrEq("sim/cockpit/autopilot/heading_mag")))
        .WillOnce(Return(dummyDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(dummyDr)).WillOnce(Return(2)); // xplmType_Float = 2
    EXPECT_CALL(mockSdk, GetDataf(dummyDr)).WillOnce(Return(359.0f));
    EXPECT_CALL(mockSdk, SetDataf(dummyDr, 0.0f));

    processor.ProcessEvent(config, "hdg", "inner-knob", "rotate-clockwise");
}

TEST(EventProcessorTest, ProcessEvent_CallsDatarefAdjustStop) {
    MockXPlaneSDK mockSdk;
    EventProcessor processor(mockSdk);

    nlohmann::json config = {
        {"modes", {
            {"ap", {
                {"outer-knob", {
                    {"rotate-counterclockwise", {
                        {"type", "dataref-adjust"},
                        {"value", "sim/cockpit/autopilot/altitude"},
                        {"adjustment", -100.0},
                        {"min", 0.0},
                        {"max", 40000.0},
                        {"limit-type", "stop"}
                    }}
                }}
            }}
        }}
    };

    void* dummyDr = reinterpret_cast<void*>(0xDEF0);
    EXPECT_CALL(mockSdk, FindDataRef(::testing::StrEq("sim/cockpit/autopilot/altitude")))
        .WillOnce(Return(dummyDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(dummyDr)).WillOnce(Return(2)); // xplmType_Float = 2
    EXPECT_CALL(mockSdk, GetDataf(dummyDr)).WillOnce(Return(50.0f));
    EXPECT_CALL(mockSdk, SetDataf(dummyDr, 0.0f));

    processor.ProcessEvent(config, "ap", "outer-knob", "rotate-counterclockwise");
}

TEST(EventProcessorTest, ProcessEvent_AdjustsIntDataref) {
    MockXPlaneSDK mockSdk;
    EventProcessor processor(mockSdk);
    
    nlohmann::json config = {
        {"modes", {
            {"xpdr", {
                {"inner-knob", {
                    {"rotate-clockwise", {
                        {"type", "dataref-adjust"},
                        {"value", "sim/transponder/transponder_code"},
                        {"adjustment", 1.0},
                        {"min", 0.0},
                        {"max", 7777.0}
                    }}
                }}
            }}
        }}
    };
    
    void* dummyDr = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindDataRef(::testing::StrEq("sim/transponder/transponder_code")))
        .WillOnce(Return(dummyDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(dummyDr)).WillOnce(Return(1)); // xplmType_Int = 1
    EXPECT_CALL(mockSdk, GetDatai(dummyDr)).WillOnce(Return(1200));
    EXPECT_CALL(mockSdk, SetDatai(dummyDr, 1201));
    
    processor.ProcessEvent(config, "xpdr", "inner-knob", "rotate-clockwise");
}

TEST(EventProcessorTest, ProcessEvent_LogsAtVerboseLevel) {
    NiceMock<MockXPlaneSDK> mockSdk;
    EventProcessor processor(mockSdk);

    nlohmann::json config = {
        {"modes", {
            {"com1", {
                {"swap", {
                    {"short-press", {
                        {"type", "command"},
                        {"value", "sim/radios/com1_standy_flip"}
                    }}
                }}
            }}
        }}
    };

    void* dummyCmd = reinterpret_cast<void*>(0x1234);
    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("sim/radios/com1_standy_flip")))
        .WillOnce(Return(dummyCmd));
    EXPECT_CALL(mockSdk, CommandOnce(dummyCmd));
    
    // GetLogLevel should be called to check if verbose logging is enabled for conditions
    EXPECT_CALL(mockSdk, GetLogLevel()).WillRepeatedly(Return(LogLevel::Info));

    // Log should be called with Verbose level for the event and action
    EXPECT_CALL(mockSdk, Log(LogLevel::Verbose, ::testing::_)).WillRepeatedly(Return());
    EXPECT_CALL(mockSdk, Log(LogLevel::Verbose, ::testing::HasSubstr("Event - mode: com1"))).Times(1);
    EXPECT_CALL(mockSdk, Log(LogLevel::Verbose, ::testing::HasSubstr("Executing command"))).Times(1);

    processor.ProcessEvent(config, "com1", "swap", "short-press");
}

TEST(EventProcessorTest, ProcessEvent_ExecutesMultipleActionsWhenRequested) {
    MockXPlaneSDK mockSdk;
    EventProcessor processor(mockSdk);

    nlohmann::json config = {
        {"modes", {
            {"com1", {
                {"swap", {
                    {"short-press", {
                        {
                            {"condition", {{"dataref", "sim/test/dr1"}, {"min", 1}, {"max", 1}, {"evaluate_next_condition", true}}},
                            {"type", "command"},
                            {"value", "sim/test/cmd1"}
                        },
                        {
                            {"type", "command"},
                            {"value", "sim/test/cmd2"}
                        }
                    }}
                }}
            }}
        }}
    };

    void* dr1 = reinterpret_cast<void*>(0x1);
    void* cmd1 = reinterpret_cast<void*>(0x10);
    void* cmd2 = reinterpret_cast<void*>(0x20);

    EXPECT_CALL(mockSdk, FindDataRef(::testing::StrEq("sim/test/dr1"))).WillRepeatedly(::testing::Return(dr1));
    EXPECT_CALL(mockSdk, GetDataRefTypes(dr1)).WillRepeatedly(::testing::Return(1)); // Int
    EXPECT_CALL(mockSdk, GetDatai(dr1)).WillRepeatedly(::testing::Return(1));

    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("sim/test/cmd1"))).WillOnce(::testing::Return(cmd1));
    EXPECT_CALL(mockSdk, CommandOnce(cmd1));

    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("sim/test/cmd2"))).WillOnce(::testing::Return(cmd2));
    EXPECT_CALL(mockSdk, CommandOnce(cmd2));

    processor.ProcessEvent(config, "com1", "swap", "short-press");
}

TEST(EventProcessorTest, ProcessEvent_StopsAtFirstMatchByDefaultForArray) {
    MockXPlaneSDK mockSdk;
    EventProcessor processor(mockSdk);

    nlohmann::json config = {
        {"modes", {
            {"com1", {
                {"swap", {
                    {"short-press", {
                        {
                            {"condition", {{"dataref", "sim/test/dr1"}, {"min", 1}, {"max", 1}}},
                            {"type", "command"},
                            {"value", "sim/test/cmd1"}
                        },
                        {
                            {"type", "command"},
                            {"value", "sim/test/cmd2"}
                        }
                    }}
                }}
            }}
        }}
    };

    void* dr1 = reinterpret_cast<void*>(0x1);
    void* cmd1 = reinterpret_cast<void*>(0x10);

    EXPECT_CALL(mockSdk, FindDataRef(::testing::StrEq("sim/test/dr1"))).WillRepeatedly(::testing::Return(dr1));
    EXPECT_CALL(mockSdk, GetDataRefTypes(dr1)).WillRepeatedly(::testing::Return(1)); // Int
    EXPECT_CALL(mockSdk, GetDatai(dr1)).WillRepeatedly(::testing::Return(1));

    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("sim/test/cmd1"))).WillOnce(::testing::Return(cmd1));
    EXPECT_CALL(mockSdk, CommandOnce(cmd1));

    // cmd2 should NOT be called
    EXPECT_CALL(mockSdk, FindCommand(::testing::StrEq("sim/test/cmd2"))).Times(0);

    processor.ProcessEvent(config, "com1", "swap", "short-press");
}
