#include "EventProcessor.h"
#include <cmath>

void EventProcessor::ProcessEvent(const nlohmann::json& config, 
                                  const std::string& mode, 
                                  const std::string& control, 
                                  const std::string& action) const
{
    if (config.empty()) return;

    if (config.contains("modes") && 
        config["modes"].contains(mode) && 
        config["modes"][mode].contains(control) && 
        config["modes"][mode][control].contains(action)) {
        
        bool verbose = config.value("debug", false);
        if (verbose) {
            m_sdk.DebugString(("IFR-1 Flex: Event - mode: " + mode + ", control: " + control + ", action: " + action + "\n").c_str());
        }

        const auto& actionData = config["modes"][mode][control][action];
        if (actionData.is_array()) {
            for (const auto& actionConfig : actionData) {
                if (m_evaluator.EvaluateConditions(actionConfig, verbose)) {
                    ExecuteAction(actionConfig, verbose);
                    break; // First one that matches wins
                }
            }
        } else {
            if (m_evaluator.EvaluateConditions(actionData, verbose)) {
                ExecuteAction(actionData, verbose);
            }
        }
    }
}

void EventProcessor::ExecuteAction(const nlohmann::json& actionConfig, bool verbose) const
{
    std::string type = actionConfig.value("type", "");
    std::string value = actionConfig.value("value", "");

    if (type == "command") {
        if (void* cmdRef = m_sdk.FindCommand(value.c_str())) {
            if (verbose) {
                m_sdk.DebugString(("IFR-1 Flex: Executing command: " + value + "\n").c_str());
            }
            m_sdk.CommandOnce(cmdRef);
        } else {
            std::string msg = "IFR-1 Flex: Command not found: " + value + "\n";
            m_sdk.DebugString(msg.c_str());
        }
    } else if (type == "dataref-set") {
        if (void* drRef = m_sdk.FindDataRef(value.c_str())) {
            if (actionConfig.contains("adjustment")) {
                float adj = actionConfig["adjustment"].get<float>();
                if (verbose) {
                    m_sdk.DebugString(("IFR-1 Flex: Setting dataref: " + value + " to " + std::to_string(adj) + "\n").c_str());
                }
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
        if (void* drRef = m_sdk.FindDataRef(value.c_str())) {
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
            
            if (verbose) {
                m_sdk.DebugString(("IFR-1 Flex: Adjusting dataref: " + value + " (current: " + std::to_string(current) + ", adj: " + std::to_string(adj) + ") -> " + std::to_string(next) + "\n").c_str());
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
