#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ConfigManager.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

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
};

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for test configs
        testConfigDir = fs::temp_directory_path() / "ifr1flex_test_configs";
        fs::create_directories(testConfigDir);
        
        ON_CALL(mockSdk, Log(::testing::_, ::testing::_)).WillByDefault(::testing::Return());
    }

    void TearDown() override {
        fs::remove_all(testConfigDir);
    }

    void CreateTestConfig(const std::string& filename, const nlohmann::json& content) {
        std::ofstream file(testConfigDir / filename);
        file << content.dump();
    }

    fs::path testConfigDir;
    MockXPlaneSDK mockSdk;
};

TEST_F(ConfigManagerTest, LoadConfigs_LoadsMultipleFiles) {
    CreateTestConfig("config1.json", {{"aircraft", {"Aircraft1"}}});
    CreateTestConfig("config2.json", {{"aircraft", {"Aircraft2"}}});
    CreateTestConfig("not_json.txt", {{"not", "json"}});

    ConfigManager manager;
    size_t loaded = manager.LoadConfigs(testConfigDir.string(), mockSdk);

    EXPECT_EQ(loaded, 2);
}

TEST_F(ConfigManagerTest, LoadConfigs_IncludesFallbackInCount) {
    CreateTestConfig("fallback.json", {{"fallback", true}});
    CreateTestConfig("specific.json", {{"aircraft", {"Specific"}}});

    ConfigManager manager;
    size_t loaded = manager.LoadConfigs(testConfigDir.string(), mockSdk);

    EXPECT_EQ(loaded, 2);
}

TEST_F(ConfigManagerTest, GetConfigForAircraft_FindsCorrectAircraft) {
    CreateTestConfig("ga.json", {
        {"aircraft", {"Cessna_172", "RV-10"}},
        {"id", "ga_config"}
    });
    CreateTestConfig("cirrus.json", {
        {"aircraft", {"Cirrus SR22"}},
        {"id", "cirrus_config"}
    });

    ConfigManager manager;
    manager.LoadConfigs(testConfigDir.string(), mockSdk);

    auto config1 = manager.GetConfigForAircraft("Aircraft/General/Cessna_172/Cessna_172.acf", mockSdk);
    ASSERT_FALSE(config1.empty());
    EXPECT_EQ(config1["id"], "ga_config");

    auto config2 = manager.GetConfigForAircraft("Aircraft/Laminar/Cirrus SR22/Cirrus SR22.acf", mockSdk);
    ASSERT_FALSE(config2.empty());
    EXPECT_EQ(config2["id"], "cirrus_config");

    auto config3 = manager.GetConfigForAircraft("Unknown Aircraft", mockSdk);
    EXPECT_TRUE(config3.empty());
}

TEST_F(ConfigManagerTest, GetConfigForAircraft_ReturnsFallbackIfNoMatch) {
    CreateTestConfig("specific.json", {
        {"aircraft", {"SpecificAircraft"}},
        {"id", "specific"}
    });
    CreateTestConfig("fallback.json", {
        {"fallback", true},
        {"id", "fallback"}
    });

    ConfigManager manager;
    manager.LoadConfigs(testConfigDir.string(), mockSdk);

    auto config1 = manager.GetConfigForAircraft("SpecificAircraft.acf", mockSdk);
    EXPECT_EQ(config1["id"], "specific");

    auto config2 = manager.GetConfigForAircraft("SomeOtherAircraft.acf", mockSdk);
    EXPECT_EQ(config2["id"], "fallback");
}

TEST_F(ConfigManagerTest, LoadConfigs_PopulatesNameFromFilenameIfMissing) {
    CreateTestConfig("my-cool-config.json", {{"aircraft", {"CoolPlane"}}});
    CreateTestConfig("with-name.json", {{"name", "Explicit Name"}, {"aircraft", {"OtherPlane"}}});

    ConfigManager manager;
    manager.LoadConfigs(testConfigDir.string(), mockSdk);

    auto config1 = manager.GetConfigForAircraft("CoolPlane", mockSdk);
    EXPECT_EQ(config1["name"], "my-cool-config");

    auto config2 = manager.GetConfigForAircraft("OtherPlane", mockSdk);
    EXPECT_EQ(config2["name"], "Explicit Name");
}

TEST_F(ConfigManagerTest, LoadConfigs_VerifiesOutputSectionAtRoot) {
    CreateTestConfig("ok.json", {
        {"name", "OK"},
        {"aircraft", {"OKPlane"}},
        {"modes", {}},
        {"output", {}}
    });
    
    CreateTestConfig("bad_nesting.json", {
        {"name", "Bad"},
        {"aircraft", {"BadPlane"}},
        {"modes", {
            {"output", {}}
        }}
    });

    ConfigManager manager;
    manager.LoadConfigs(testConfigDir.string(), mockSdk);

    auto okConfig = manager.GetConfigForAircraft("OKPlane", mockSdk);
    EXPECT_TRUE(okConfig.contains("output"));
    EXPECT_TRUE(okConfig.contains("modes"));

    auto badConfig = manager.GetConfigForAircraft("BadPlane", mockSdk);
    EXPECT_TRUE(badConfig.contains("modes"));
    // In the bad one, output is INSIDE modes, not at root.
    EXPECT_FALSE(badConfig.contains("output"));
    EXPECT_TRUE(badConfig["modes"].contains("output"));
}
