#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

#include "XPLMPlugin.h"
#include "XPLMDisplay.h"
#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include "XPLMMenus.h"

#include "ConfigManager.h"
#include "EventProcessor.h"
#include "OutputProcessor.h"
#include "HIDManager.h"
#include "DeviceHandler.h"
#include "XPlaneSDK.h"
#include "ui/AboutWindow.h"

namespace fs = std::filesystem;

// Global state
static std::unique_ptr<IXPlaneSDK> gSDK;
static std::unique_ptr<ConfigManager> gConfigManager;
static std::unique_ptr<EventProcessor> gEventProcessor;
static std::unique_ptr<OutputProcessor> gOutputProcessor;
static std::unique_ptr<HIDManager> gHIDManager;
static std::unique_ptr<DeviceHandler> gDeviceHandler;

static nlohmann::json gCurrentConfig;
static std::string gCurrentAircraftPath;
static void* gAcfPathRef = nullptr;

static XPLMFlightLoopID gFlightLoop = nullptr;

// Menu and UI
static XPLMMenuID gSubMenu = nullptr;
static int gSubMenuIndex = -1;

// Menu handler
static void MenuHandler(void* /*inMenuRef*/, void* inItemRef) {
    auto item = reinterpret_cast<intptr_t>(inItemRef);
    if (item == 1) {
        ui::about::Show();
    }
}

// ReSharper disable once CppDFAConstantFunctionResult
float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon) {
    if (!gSDK || !gDeviceHandler) return -1.0f;

    float now = gSDK->GetElapsedTime();

    // 1. Aircraft detection
    char path[512];
    int bytes = XPLMGetDatab(static_cast<XPLMDataRef>(gAcfPathRef), path, 0, sizeof(path) - 1);
    if (bytes > 0) {
        path[bytes] = '\0';
        std::string currentPath(path);
        if (currentPath != gCurrentAircraftPath) {
            gSDK->Log(LogLevel::Info, ("Aircraft changed to " + currentPath).c_str());
            gCurrentAircraftPath = currentPath;
            gDeviceHandler->ClearLEDs();
            gCurrentConfig = gConfigManager->GetConfigForAircraft(currentPath, *gSDK);

            if (!gCurrentConfig.empty() && !gCurrentConfig.contains("output")) {
                gSDK->Log(LogLevel::Error, ("Loaded config '" + gCurrentConfig.value("name", "unknown") + "' is missing 'output' section!").c_str());
            }

            // Update log level based on config
            if (gCurrentConfig.value("debug", false)) {
                gSDK->SetLogLevel(LogLevel::Verbose);
            } else {
                gSDK->SetLogLevel(LogLevel::Info);
            }

            if (gCurrentConfig.empty()) {
                gSDK->Log(LogLevel::Info, "No configuration found for this aircraft.");
            } else {
                std::string configName = gCurrentConfig.value("name", "Unknown");
                if (gCurrentConfig.value("fallback", false)) {
                    gSDK->Log(LogLevel::Info, ("Using fallback configuration: " + configName).c_str());
                } else {
                    gSDK->Log(LogLevel::Info, ("Configuration loaded: " + configName).c_str());
                }
            }
        }
    }

    // 2. Update hardware input
    gDeviceHandler->Update(gCurrentConfig, now);

    // 3. Update LEDs
    gDeviceHandler->UpdateLEDs(gCurrentConfig, now);

    return -1.0f; // Run every frame
}

PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc) {
    std::strcpy(outName, "IFR-1 Plugin");
    std::strcpy(outSig, "com.kyleross.ifr1flex");
    std::strcpy(outDesc, "Flexible IFR-1 interface.");

    gSDK = CreateXPlaneSDK();
    gConfigManager = std::make_unique<ConfigManager>();
    gEventProcessor = std::make_unique<EventProcessor>(*gSDK);
    gOutputProcessor = std::make_unique<OutputProcessor>(*gSDK);
    gHIDManager = std::make_unique<HIDManager>();
    gDeviceHandler = std::make_unique<DeviceHandler>(*gHIDManager, *gEventProcessor, *gOutputProcessor, *gSDK);

    char pluginPath[512];
    XPLMGetPluginInfo(XPLMGetMyID(), nullptr, pluginPath, nullptr, nullptr);
    fs::path p(pluginPath);
    
    // Config directory discovery:
    // 1. Check parent folder of the plugin binary (e.g. plugins/ifr1flex/64/lin.xpl -> plugins/ifr1flex/configs)
    // 2. Check the plugin binary folder itself (e.g. plugins/ifr1flex/lin.xpl -> plugins/ifr1flex/configs)
    
    fs::path configDir;
    fs::path p1 = p.parent_path().parent_path() / "configs";
    fs::path p2 = p.parent_path() / "configs";

    if (fs::exists(p1) && fs::is_directory(p1)) {
        configDir = p1;
    } else if (fs::exists(p2) && fs::is_directory(p2)) {
        configDir = p2;
    }
    
    size_t loaded = 0;
    if (!configDir.empty()) {
        loaded = gConfigManager->LoadConfigs(configDir.string(), *gSDK);
    }
    
    char msg[1024];
    if (loaded == 0) {
        if (configDir.empty()) {
            std::snprintf(msg, sizeof(msg), "ERROR: Could not find 'configs' directory. Tried:\n  1. %s\n  2. %s\n", 
                         p1.string().c_str(), p2.string().c_str());
            gSDK->Log(LogLevel::Error, msg);
        } else {
            std::snprintf(msg, sizeof(msg), "WARNING: No configuration files found in %s\n", configDir.string().c_str());
            gSDK->Log(LogLevel::Info, msg);
        }
    } else {
        std::snprintf(msg, sizeof(msg), "Loaded %zu configurations from %s\n", loaded, configDir.string().c_str());
        gSDK->Log(LogLevel::Info, msg);
    }

    // Create Plugins menu: "IFR-1" -> "About..."
    if (XPLMMenuID pluginsMenu = XPLMFindPluginsMenu()) {
        gSubMenuIndex = XPLMAppendMenuItem(pluginsMenu, "IFR-1 Flex", nullptr, 0);
        gSubMenu = XPLMCreateMenu("IFR-1 Flex", pluginsMenu, gSubMenuIndex, MenuHandler, nullptr);
        if (gSubMenu) {
            XPLMAppendMenuItem(gSubMenu, "About...", reinterpret_cast<void*>(1), 0);
        }
    }

    return 1;
}

PLUGIN_API void XPluginStop(void) {
    if (gFlightLoop) {
        XPLMDestroyFlightLoop(gFlightLoop);
        gFlightLoop = nullptr;
    }
    
    // Destroy UI/menu if still present
    ui::about::Close();
    if (gSubMenu) {
        XPLMDestroyMenu(gSubMenu);
        gSubMenu = nullptr;
        gSubMenuIndex = -1;
    }

    gDeviceHandler.reset();
    gHIDManager.reset();
    gOutputProcessor.reset();
    gEventProcessor.reset();
    gConfigManager.reset();
    gSDK.reset();
}

PLUGIN_API void XPluginDisable(void) {
    if (gDeviceHandler) {
        gDeviceHandler->ClearLEDs();
    }
    if (gFlightLoop) {
        XPLMDestroyFlightLoop(gFlightLoop);
        gFlightLoop = nullptr;
    }
    if (gHIDManager) {
        gHIDManager->Disconnect();
    }

    // Ensure any modal is closed when disabling
    ui::about::Close();
}

PLUGIN_API int XPluginEnable(void) {
    gAcfPathRef = XPLMFindDataRef("sim/aircraft/view/acf_relative_path");
    gCurrentAircraftPath.clear();
    
    if (gDeviceHandler) {
        gDeviceHandler->ClearLEDs();
    }

    XPLMCreateFlightLoop_t params;
    params.structSize = sizeof(XPLMCreateFlightLoop_t);
    params.phase = xplm_FlightLoop_Phase_BeforeFlightModel;
    params.callbackFunc = FlightLoopCallback;
    params.refcon = nullptr;

    gFlightLoop = XPLMCreateFlightLoop(&params);
    XPLMScheduleFlightLoop(gFlightLoop, -1.0f, 1);

    return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void *inParam) {
}