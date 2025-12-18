#include "EventProcessor.h"

void EventProcessor::ProcessEvent(const nlohmann::json& config, 
                                  const std::string& mode, 
                                  const std::string& control, 
                                  const std::string& action) {
    if (config.empty()) return;

    if (config.contains("modes") && 
        config["modes"].contains(mode) && 
        config["modes"][mode].contains(control) && 
        config["modes"][mode][control].contains(action)) {
        
        auto actionConfig = config["modes"][mode][control][action];
        std::string type = actionConfig.value("type", "");
        std::string value = actionConfig.value("value", "");

        if (type == "command") {
            void* cmdRef = m_sdk.FindCommand(value.c_str());
            if (cmdRef) {
                m_sdk.CommandOnce(cmdRef);
            }
        } else if (type == "dataref-set") {
            void* drRef = m_sdk.FindDataRef(value.c_str());
            if (drRef) {
                if (actionConfig.contains("adjustment")) {
                    float adj = actionConfig["adjustment"].get<float>();
                    m_sdk.SetDataf(drRef, adj);
                }
            }
        }
        // More types like dataref-adjust can be added here
    }
}
