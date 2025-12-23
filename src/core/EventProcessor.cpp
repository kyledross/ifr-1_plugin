#include "EventProcessor.h"
#include <cmath>
#include <regex>

namespace {
    struct DataRefInfo {
        std::string name;
        int index = -1;
    };

    DataRefInfo ParseDataRef(const std::string& rawName) {
        std::regex re("(.+)\\[([0-9]+)\\]");
        std::smatch match;
        if (std::regex_match(rawName, match, re)) {
            return {match[1].str(), std::stoi(match[2].str())};
        }
        return {rawName, -1};
    }
}

void EventProcessor::ProcessEvent(const nlohmann::json& config, 
                                  const std::string& mode, 
                                  const std::string& control, 
                                  const std::string& action)
{
    if (config.empty()) return;

    if (config.contains("modes") && 
        config["modes"].contains(mode) && 
        config["modes"][mode].contains(control) && 
        config["modes"][mode][control].contains(action)) {
        
        m_sdk.Log(LogLevel::Verbose, ("Event - mode: " + mode + ", control: " + control + ", action: " + action).c_str());

        const auto& eventConfig = config["modes"][mode][control][action];
        
        auto processActions = [&](const nlohmann::json& actions) {
            for (const auto& actionConfig : actions) {
                if (m_evaluator.EvaluateConditions(actionConfig, m_sdk.GetLogLevel() >= LogLevel::Verbose)) {
                    ExecuteAction(actionConfig);
                    if (!ShouldEvaluateNext(actionConfig)) {
                        break; // First one that matches wins (unless requested to continue)
                    }
                }
            }
        };

        if (eventConfig.is_object() && eventConfig.contains("actions") && eventConfig["actions"].is_array()) {
            processActions(eventConfig["actions"]);
        } else {
            m_sdk.Log(LogLevel::Error, ("Event " + control + "/" + action + " in mode " + mode + " missing required 'actions' array").c_str());
        }
    }
}

void EventProcessor::ExecuteAction(const nlohmann::json& actionConfig)
{
    std::string type = actionConfig.value("type", "");
    std::string value = actionConfig.value("value", "");

    if (type == "command") {
        if (void* cmdRef = m_sdk.FindCommand(value.c_str())) {
            int times = actionConfig.value("send-count", 1);
            if (times < 0) times = -times;
            
            if (times > 0) {
                m_sdk.Log(LogLevel::Verbose, ("Queueing command: " + value + " (" + std::to_string(times) + " times)").c_str());
                for (int i = 0; i < times; ++i) {
                    if (m_commandQueue.size() < 10) {
                        m_commandQueue.push(cmdRef);
                    } else {
                        m_sdk.Log(LogLevel::Verbose, "Command queue full, discarding command");
                    }
                }
            } else {
                m_sdk.Log(LogLevel::Verbose, ("Skipping command: " + value + " (send-count is 0)").c_str());
            }
        } else {
            m_sdk.Log(LogLevel::Error, ("Command not found: " + value).c_str());
        }
    } else if (type == "dataref-set") {
        auto info = ParseDataRef(value);
        if (void* drRef = m_sdk.FindDataRef(info.name.c_str())) {
            if (actionConfig.contains("adjustment")) {
                float adj = actionConfig["adjustment"].get<float>();
                m_sdk.Log(LogLevel::Verbose, ("Setting dataref: " + value + " to " + std::to_string(adj)).c_str());
                int types = m_sdk.GetDataRefTypes(drRef);
                if (info.index != -1) {
                    if (types & static_cast<int>(DataRefType::IntArray)) {
                        m_sdk.SetDataiArray(drRef, static_cast<int>(adj), info.index);
                    } else {
                        m_sdk.SetDatafArray(drRef, adj, info.index);
                    }
                } else {
                    if (types & static_cast<int>(DataRefType::Int)) {
                        m_sdk.SetDatai(drRef, static_cast<int>(adj));
                    } else {
                        m_sdk.SetDataf(drRef, adj);
                    }
                }
            }
        } else {
            m_sdk.Log(LogLevel::Error, ("DataRef not found: " + value).c_str());
        }
    } else if (type == "dataref-adjust") {
        auto info = ParseDataRef(value);
        if (void* drRef = m_sdk.FindDataRef(info.name.c_str())) {
            int types = m_sdk.GetDataRefTypes(drRef);
            float current = 0.0f;
            bool isInt = false;
            
            if (info.index != -1) {
                isInt = (types & static_cast<int>(DataRefType::IntArray));
                if (isInt) {
                    current = static_cast<float>(m_sdk.GetDataiArray(drRef, info.index));
                } else {
                    current = m_sdk.GetDatafArray(drRef, info.index);
                }
            } else {
                isInt = (types & static_cast<int>(DataRefType::Int));
                if (isInt) {
                    current = static_cast<float>(m_sdk.GetDatai(drRef));
                } else {
                    current = m_sdk.GetDataf(drRef);
                }
            }

            float adj = actionConfig.value("adjustment", 0.0f);
            float next = current + adj;

            if (actionConfig.contains("min") && actionConfig.contains("max")) {
                float minVal = actionConfig["min"].get<float>();
                float maxVal = actionConfig["max"].get<float>();
                std::string limitType = actionConfig.value("limit-type", "clamp");

                if (limitType == "wrap") {
                    float range = maxVal - minVal + 1.0f;
                    while (next < minVal) next += range;
                    while (next > maxVal) next -= range;
                } else {
                    if (next < minVal) next = minVal;
                    if (next > maxVal) next = maxVal;
                }
            }
            
            m_sdk.Log(LogLevel::Verbose, ("Adjusting dataref: " + value + " (current: " + std::to_string(current) + ", adj: " + std::to_string(adj) + ") -> " + std::to_string(next)).c_str());

            if (info.index != -1) {
                if (isInt) {
                    m_sdk.SetDataiArray(drRef, static_cast<int>(std::round(next)), info.index);
                } else {
                    m_sdk.SetDatafArray(drRef, next, info.index);
                }
            } else {
                if (isInt) {
                    m_sdk.SetDatai(drRef, static_cast<int>(std::round(next)));
                } else {
                    m_sdk.SetDataf(drRef, next);
                }
            }
        } else {
            m_sdk.Log(LogLevel::Error, ("DataRef not found: " + value).c_str());
        }
    }
}

bool EventProcessor::ShouldEvaluateNext(const nlohmann::json& actionConfig) const
{
    // Check at action level
    if (actionConfig.value("continue-to-next-action", false)) return true;

    // Check at single condition level
    if (actionConfig.contains("condition")) {
        if (actionConfig["condition"].value("continue-to-next-action", false)) return true;
    }

    // Check at multiple conditions level
    if (actionConfig.contains("conditions")) {
        const auto& conditions = actionConfig["conditions"];
        if (conditions.is_array()) {
            for (const auto& cond : conditions) {
                if (cond.is_object() && cond.value("continue-to-next-action", false)) return true;
            }
        } else if (conditions.is_object()) {
            if (conditions.value("continue-to-next-action", false)) return true;
        }
    }

    return false;
}

void EventProcessor::ProcessQueue()
{
    if (!m_commandQueue.empty()) {
        void* cmdRef = m_commandQueue.front();
        m_commandQueue.pop();
        m_sdk.CommandOnce(cmdRef);
    }
}
