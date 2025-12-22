#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

namespace fs = std::filesystem;

class ConfigValidationTest : public ::testing::Test {
protected:
    static std::string GetConfigDir() {
        // Try several locations to find the configs directory
        std::vector<std::string> searchPaths = {
            "configs",           // Current directory (if run from project root)
            "../configs",        // One level up (if run from build directory)
            "../../configs",     // Two levels up (if run from deep in build directory)
            "../../../configs"   // Three levels up
        };

        for (const auto& path : searchPaths) {
            if (fs::exists(path) && fs::is_directory(path)) {
                return path;
            }
        }
        return "";
    }
};

TEST_F(ConfigValidationTest, ValidateAllConfigs) {
    std::string configDir = GetConfigDir();
    ASSERT_NE(configDir, "") << "Could not find configs directory.";

    int validatedCount = 0;
    for (const auto& entry : fs::directory_iterator(configDir)) {
        if (entry.path().extension() == ".json") {
            std::string filename = entry.path().filename().string();
            // Use SCOPED_TRACE to identify which file failed if an assertion fails
            SCOPED_TRACE("Validating config file: " + filename);

            std::ifstream file(entry.path());
            ASSERT_TRUE(file.is_open()) << "Could not open file";

            nlohmann::json config;
            try {
                file >> config;
            } catch (const nlohmann::json::parse_error& e) {
                FAIL() << "JSON parse error: " << e.what();
            }

            // Basic structure check
            EXPECT_TRUE(config.contains("name")) << "Missing root key 'name'";
            EXPECT_TRUE(config.contains("modes")) << "Missing root key 'modes'";
            EXPECT_TRUE(config.contains("output")) << "Missing root key 'output'";
            
            if (config.contains("modes")) {
                EXPECT_TRUE(config["modes"].is_object()) << "'modes' is not an object";
            }
            if (config.contains("output")) {
                EXPECT_TRUE(config["output"].is_object()) << "'output' is not an object";
            }

            validatedCount++;
        }
    }
    
    std::cout << "[          ] Validated " << validatedCount << " configuration files." << std::endl;
    EXPECT_GT(validatedCount, 0) << "No config files were found to validate.";
}
