#include "EventProcessor.h"
#include <cmath>

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
                m_sdk.DebugString(("IFR-1 Flex: Executing command: " + value + "\n").c_str());
                m_sdk.CommandOnce(cmdRef);
            } else {
                std::string msg = "IFR-1 Flex: Command not found: " + value + "\n";
                m_sdk.DebugString(msg.c_str());
            }
        } else if (type == "dataref-set") {
            void* drRef = m_sdk.FindDataRef(value.c_str());
            if (drRef) {
                if (actionConfig.contains("adjustment")) {
                    float adj = actionConfig["adjustment"].get<float>();
                    int types = m_sdk.GetDataRefTypes(drRef);
                    if (types & static_cast<int>(DataRefType::Int)) {
                        m_sdk.SetDatai(drRef, static_cast<int>(adj));
                    } else {
                        m_sdk.SetDataf(drRef, adj);
                    }
                }
            } else {
                std::string msg = "IFR-1 Flex: DataRef not found: " + value + "\n";
                m_sdk.DebugString(msg.c_str());
            }
        } else if (type == "dataref-adjust") {
            void* drRef = m_sdk.FindDataRef(value.c_str());
            if (drRef) {
                int types = m_sdk.GetDataRefTypes(drRef);
                float current = 0.0f;
                bool isInt = (types & static_cast<int>(DataRefType::Int));
                
                if (isInt) {
                    current = static_cast<float>(m_sdk.GetDatai(drRef));
                } else {
                    current = m_sdk.GetDataf(drRef);
                }

                float adj = actionConfig.value("adjustment", 0.0f);
                float next = current + adj;

                if (actionConfig.contains("min") && actionConfig.contains("max")) {
                    float minVal = actionConfig["min"].get<float>();
                    float maxVal = actionConfig["max"].get<float>();
                    std::string limitType = actionConfig.value("limit-type", "stop");

                    if (limitType == "wrap") {
                        float range = maxVal - minVal + 1.0f;
                        while (next < minVal) next += range;
                        while (next > maxVal) next -= range;
                    } else {
                        if (next < minVal) next = minVal;
                        if (next > maxVal) next = maxVal;
                    }
                }
                
                if (isInt) {
                    m_sdk.SetDatai(drRef, static_cast<int>(std::round(next)));
                } else {
                    m_sdk.SetDataf(drRef, next);
                }
            } else {
                std::string msg = "IFR-1 Flex: DataRef not found: " + value + "\n";
                m_sdk.DebugString(msg.c_str());
            }
        }
    }
}
