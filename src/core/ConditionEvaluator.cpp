#include "ConditionEvaluator.h"
#include <string>

bool ConditionEvaluator::EvaluateCondition(const nlohmann::json& condition, bool verbose) const {
    if (!condition.contains("dataref")) return false;

    std::string drName = condition["dataref"];
    void* drRef = m_sdk.FindDataRef(drName.c_str());
    if (!drRef) {
        if (verbose) {
            m_sdk.DebugString(("IFR-1 Flex: Condition failed - DataRef not found: " + drName + "\n").c_str());
        }
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
        std::string msg = "IFR-1 Flex: Testing " + drName + " (value: " + std::to_string(val) + ") against " + testDesc + " -> " + (result ? "TRUE" : "FALSE") + "\n";
        m_sdk.DebugString(msg.c_str());
    }

    return result;
}

bool ConditionEvaluator::EvaluateConditions(const nlohmann::json& actionConfig, bool verbose) const {
    // If there is no "conditions" or "condition" key, it's always true (default action)
    if (!actionConfig.contains("conditions") && !actionConfig.contains("condition")) {
        if (verbose) {
            m_sdk.DebugString("IFR-1 Flex: No conditions for this action, assuming TRUE\n");
        }
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
