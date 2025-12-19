#include "ConditionEvaluator.h"
#include <string>

bool ConditionEvaluator::EvaluateCondition(const nlohmann::json& condition, bool verbose) const {
    if (!condition.contains("dataref")) return false;

    std::string drName = condition["dataref"];
    void* drRef = m_sdk.FindDataRef(drName.c_str());
    if (!drRef) {
        m_sdk.Log(LogLevel::Verbose, ("Condition failed - DataRef not found: " + drName).c_str());
        return false;
    }

    int types = m_sdk.GetDataRefTypes(drRef);
    double val = 0.0;
    if (types & static_cast<int>(DataRefType::Int)) {
        val = static_cast<double>(m_sdk.GetDatai(drRef));
    } else {
        val = static_cast<double>(m_sdk.GetDataf(drRef));
    }

    bool result = false;
    std::string testDesc;

    if (condition.contains("bit")) {
        int bit = condition["bit"].get<int>();
        result = (static_cast<int>(val) & (1 << bit)) != 0;
        if (verbose) {
            testDesc = "bit " + std::to_string(bit) + " set";
        }
    } else if (condition.contains("min") && condition.contains("max")) {
        double minVal = condition["min"].get<double>();
        double maxVal = condition["max"].get<double>();
        result = (val >= minVal && val <= maxVal);
        if (verbose) {
            testDesc = "range [" + std::to_string(minVal) + ", " + std::to_string(maxVal) + "]";
        }
    }

    if (verbose) {
        std::string msg = "Testing " + drName + " (value: " + std::to_string(val) + ") against " + testDesc + " -> " + (result ? "TRUE" : "FALSE");
        m_sdk.Log(LogLevel::Verbose, msg.c_str());
    }

    return result;
}

bool ConditionEvaluator::EvaluateConditions(const nlohmann::json& actionConfig, bool verbose) const {
    // If there is no "conditions" or "condition" key, it's always true (default action)
    if (!actionConfig.contains("conditions") && !actionConfig.contains("condition")) {
        m_sdk.Log(LogLevel::Verbose, "No conditions for this action, assuming TRUE");
        return true;
    }

    if (actionConfig.contains("conditions")) {
        const auto& conditions = actionConfig["conditions"];
        if (conditions.is_array()) {
            for (const auto& condition : conditions) {
                if (!EvaluateCondition(condition, verbose)) return false;
            }
            return true;
        } else {
            return EvaluateCondition(conditions, verbose);
        }
    }

    if (actionConfig.contains("condition")) {
        return EvaluateCondition(actionConfig["condition"], verbose);
    }

    return true;
}
