#include <cstring>
#include "XPLMPlugin.h"
#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "ConfigManager.h"
#include <string>
#include <print>

// Global variables
ConfigManager gConfigManager;

PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc)
{
    std::strcpy(outName, "IFR-1 Flex");
    std::strcpy(outSig, "com.kyle.ifr1flex");
    std::strcpy(outDesc, "Flexible Octavi IFR-1 interface.");

    XPLMDebugString("IFR-1 Flex: Plugin starting...\n");

    // In a real scenario, we'd get the plugin directory and load configs from there
    size_t loaded = gConfigManager.LoadConfigs("Resources/plugins/ifr1flex/configs");

    char msg[128];
    std::snprintf(msg, sizeof(msg), "IFR-1 Flex: Loaded %zu configurations.\n", loaded);
    XPLMDebugString(msg);

    return 1;
}

PLUGIN_API void	XPluginStop(void)
{
    XPLMDebugString("IFR-1 Flex: Stop.\n");
}

PLUGIN_API void XPluginDisable(void)
{
    // Triggered when user disables plugin in menu
}

PLUGIN_API int XPluginEnable(void)
{
    // Triggered when user enables plugin in menu
    return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void *inParam)
{
    // Handle messages from other plugins or X-Plane
}