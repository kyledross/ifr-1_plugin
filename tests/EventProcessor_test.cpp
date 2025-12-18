#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "EventProcessor.h"
#include "XPlaneSDK.h"
#include <nlohmann/json.hpp>
#include <string>

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
    EXPECT_CALL(mockSdk, SetDataf(dummyDr, 121500.0f));

    processor.ProcessEvent(config, "com1", "swap", "long-press");
}
