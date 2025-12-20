#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "EventProcessor.h"
#include "XPlaneSDK.h"
#include <nlohmann/json.hpp>

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
using ::testing::StrEq;
using ::testing::_;

TEST(ConditionalActionTest, ProcessEvent_FirstMatchingConditionWins) {
    MockXPlaneSDK mockSdk;
    EventProcessor processor(mockSdk);

    nlohmann::json config = {
        {"modes", {
            {"ap", {
                {"inner-knob", {
                    {"rotate-clockwise", {
                        {
                            {"conditions", {{{"dataref", "sim/cockpit2/autopilot/altitude_mode"}, {"min", 5}, {"max", 5}}}},
                            {"type", "dataref-adjust"},
                            {"value", "sim/cockpit/autopilot/airspeed_select"},
                            {"adjustment", 1.0}
                        },
                        {
                            {"type", "dataref-adjust"},
                            {"value", "sim/cockpit/autopilot/vertical_velocity"},
                            {"adjustment", 100.0}
                        }
                    }}
                }}
            }}
        }}
    };

    void* modeDr = reinterpret_cast<void*>(0x1);
    void* iasDr = reinterpret_cast<void*>(0x2);
    void* vsDr = reinterpret_cast<void*>(0x3);

    // Scenario 1: FLC Mode (altitude_mode = 5)
    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit2/autopilot/altitude_mode"))).WillRepeatedly(Return(modeDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(modeDr)).WillRepeatedly(Return(1)); // Int
    EXPECT_CALL(mockSdk, GetDatai(modeDr)).WillRepeatedly(Return(5));

    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit/autopilot/airspeed_select"))).WillOnce(Return(iasDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(iasDr)).WillOnce(Return(2)); // Float
    EXPECT_CALL(mockSdk, GetDataf(iasDr)).WillOnce(Return(100.0f));
    EXPECT_CALL(mockSdk, SetDataf(iasDr, 101.0f));

    processor.ProcessEvent(config, "ap", "inner-knob", "rotate-clockwise");

    // Scenario 2: Not FLC Mode (altitude_mode = 4)
    EXPECT_CALL(mockSdk, GetDatai(modeDr)).WillRepeatedly(Return(4));
    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit/autopilot/vertical_velocity"))).WillOnce(Return(vsDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(vsDr)).WillOnce(Return(2)); // Float
    EXPECT_CALL(mockSdk, GetDataf(vsDr)).WillOnce(Return(500.0f));
    EXPECT_CALL(mockSdk, SetDataf(vsDr, 600.0f));

    processor.ProcessEvent(config, "ap", "inner-knob", "rotate-clockwise");
}

TEST(ConditionalActionTest, ProcessEvent_SingleActionWithCondition) {
    MockXPlaneSDK mockSdk;
    EventProcessor processor(mockSdk);

    nlohmann::json config = {
        {"modes", {
            {"ap", {
                {"inner-knob", {
                    {"rotate-clockwise", {
                        {"conditions", {{{"dataref", "sim/cockpit2/autopilot/altitude_mode"}, {"min", 5}, {"max", 5}}}},
                        {"type", "dataref-adjust"},
                        {"value", "sim/cockpit/autopilot/airspeed_select"},
                        {"adjustment", 1.0}
                    }}
                }}
            }}
        }}
    };

    void* modeDr = reinterpret_cast<void*>(0x1);
    void* iasDr = reinterpret_cast<void*>(0x2);

    // Condition met
    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit2/autopilot/altitude_mode"))).WillRepeatedly(Return(modeDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(modeDr)).WillRepeatedly(Return(1)); // Int
    EXPECT_CALL(mockSdk, GetDatai(modeDr)).WillOnce(Return(5));

    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit/autopilot/airspeed_select"))).WillOnce(Return(iasDr));
    EXPECT_CALL(mockSdk, GetDataRefTypes(iasDr)).WillOnce(Return(2)); // Float
    EXPECT_CALL(mockSdk, GetDataf(iasDr)).WillOnce(Return(100.0f));
    EXPECT_CALL(mockSdk, SetDataf(iasDr, 101.0f));

    processor.ProcessEvent(config, "ap", "inner-knob", "rotate-clockwise");

    // Condition NOT met
    EXPECT_CALL(mockSdk, GetDatai(modeDr)).WillOnce(Return(4));
    // No other calls expected
    processor.ProcessEvent(config, "ap", "inner-knob", "rotate-clockwise");
}

TEST(ConditionalActionTest, Condition_ArrayDataRef) {
    MockXPlaneSDK mockSdk;
    ConditionEvaluator evaluator(mockSdk);

    void* drRef = reinterpret_cast<void*>(0x1234);
    std::string drName = "sim/cockpit2/switches/panel_brightness_ratio[3]";
    
    EXPECT_CALL(mockSdk, FindDataRef(StrEq("sim/cockpit2/switches/panel_brightness_ratio"))).WillOnce(Return(drRef));
    EXPECT_CALL(mockSdk, GetDataRefTypes(drRef)).WillOnce(Return(static_cast<int>(DataRefType::FloatArray)));
    EXPECT_CALL(mockSdk, GetDatafArray(drRef, 3)).WillOnce(Return(0.75f));

    nlohmann::json condition = {
        {"dataref", drName},
        {"min", 0.7},
        {"max", 0.8}
    };

    EXPECT_TRUE(evaluator.EvaluateCondition(condition, false));
}
