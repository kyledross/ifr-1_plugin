#include <gtest/gtest.h>
#include "ConfigManager.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for test configs
        testConfigDir = fs::temp_directory_path() / "ifr1flex_test_configs";
        fs::create_directories(testConfigDir);
    }

    void TearDown() override {
        fs::remove_all(testConfigDir);
    }

    void CreateTestConfig(const std::string& filename, const nlohmann::json& content) {
        std::ofstream file(testConfigDir / filename);
        file << content.dump();
    }

    fs::path testConfigDir;
};

TEST_F(ConfigManagerTest, LoadConfigs_LoadsMultipleFiles) {
    CreateTestConfig("config1.json", {{"aircraft", {"Aircraft1"}}});
    CreateTestConfig("config2.json", {{"aircraft", {"Aircraft2"}}});
    CreateTestConfig("not_json.txt", {{"not", "json"}});

    ConfigManager manager;
    size_t loaded = manager.LoadConfigs(testConfigDir.string());

    EXPECT_EQ(loaded, 2);
}

TEST_F(ConfigManagerTest, LoadConfigs_IncludesFallbackInCount) {
    CreateTestConfig("fallback.json", {{"fallback", true}});
    CreateTestConfig("specific.json", {{"aircraft", {"Specific"}}});

    ConfigManager manager;
    size_t loaded = manager.LoadConfigs(testConfigDir.string());

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
    manager.LoadConfigs(testConfigDir.string());

    auto config1 = manager.GetConfigForAircraft("Aircraft/General/Cessna_172/Cessna_172.acf");
    ASSERT_FALSE(config1.empty());
    EXPECT_EQ(config1["id"], "ga_config");

    auto config2 = manager.GetConfigForAircraft("Aircraft/Laminar/Cirrus SR22/Cirrus SR22.acf");
    ASSERT_FALSE(config2.empty());
    EXPECT_EQ(config2["id"], "cirrus_config");

    auto config3 = manager.GetConfigForAircraft("Unknown Aircraft");
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
    manager.LoadConfigs(testConfigDir.string());

    auto config1 = manager.GetConfigForAircraft("SpecificAircraft.acf");
    EXPECT_EQ(config1["id"], "specific");

    auto config2 = manager.GetConfigForAircraft("SomeOtherAircraft.acf");
    EXPECT_EQ(config2["id"], "fallback");
}

TEST_F(ConfigManagerTest, LoadConfigs_PopulatesNameFromFilenameIfMissing) {
    CreateTestConfig("my-cool-config.json", {{"aircraft", {"CoolPlane"}}});
    CreateTestConfig("with-name.json", {{"name", "Explicit Name"}, {"aircraft", {"OtherPlane"}}});

    ConfigManager manager;
    manager.LoadConfigs(testConfigDir.string());

    auto config1 = manager.GetConfigForAircraft("CoolPlane");
    EXPECT_EQ(config1["name"], "my-cool-config");

    auto config2 = manager.GetConfigForAircraft("OtherPlane");
    EXPECT_EQ(config2["name"], "Explicit Name");
}
