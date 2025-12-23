#include "ConditionEvaluator.h"
#include "Logger.h"
#include <string>
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

bool ConditionEvaluator::EvaluateCondition(const nlohmann::json& condition, bool verbose) const {
    if (!condition.contains("dataref")) return false;

    std::string rawDrName = condition["dataref"];
    auto info = ParseDataRef(rawDrName);
    void* drRef = m_sdk.FindDataRef(info.name.c_str());
    if (!drRef) {
        IFR1_LOG_VERBOSE(m_sdk, "Condition failed - DataRef not found: {}", info.name);
        return false;
    }

    int types = m_sdk.GetDataRefTypes(drRef);
    double val = 0.0;
    if (info.index != -1) {
        if (types & static_cast<int>(DataRefType::IntArray)) {
            val = static_cast<double>(m_sdk.GetDataiArray(drRef, info.index));
        } else {
            val = static_cast<double>(m_sdk.GetDatafArray(drRef, info.index));
        }
    } else {
        if (types & static_cast<int>(DataRefType::Int)) {
            val = static_cast<double>(m_sdk.GetDatai(drRef));
        } else {
            val = static_cast<double>(m_sdk.GetDataf(drRef));
        }
    }

    bool result = false;

    if (condition.contains("bit")) {
        int bit = condition["bit"].get<int>();
        result = (static_cast<int>(val) & (1 << bit)) != 0;
    } else if (condition.contains("min") && condition.contains("max")) {
        double minVal = condition["min"].get<double>();
        double maxVal = condition["max"].get<double>();
        result = (val >= minVal && val <= maxVal);
    }

    IFR1_LOG_VERBOSE_IF(m_sdk, verbose, "Testing {} (value: {}) against {} -> {}", rawDrName, val,
        [&] {
            if (condition.contains("bit")) return std::format("bit {} set", condition["bit"].get<int>());
            if (condition.contains("min") && condition.contains("max")) return std::format("range [{}, {}]", condition["min"].get<double>(), condition["max"].get<double>());
            return std::string("unknown test");
        }(),
        result ? "TRUE" : "FALSE");

    return result;
}

bool ConditionEvaluator::EvaluateConditions(const nlohmann::json& actionConfig, bool verbose) const {
    // If there is no "conditions" or "condition" key, it's always true (default action)
    if (!actionConfig.contains("conditions") && !actionConfig.contains("condition")) {
        IFR1_LOG_VERBOSE(m_sdk, "No conditions for this action, assuming TRUE");
        return true;
    }

    if (actionConfig.contains("conditions")) {
        const auto& conditions = actionConfig["conditions"];
        if (conditions.is_array()) {
            return std::ranges::all_of(conditions,
                                       [this, verbose](const auto& condition)
                                       {
                                           return EvaluateCondition(condition, verbose);
                                       });
        } else {
            return EvaluateCondition(conditions, verbose);
        }
    }

    if (actionConfig.contains("condition")) {
        return EvaluateCondition(actionConfig["condition"], verbose);
    }

    return true;
}
