/*
 *   Copyright 2026 Kyle D. Ross
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "ConditionEvaluator.h"
#include "Logger.h"
#include "ParsedCondition.h"
#include <string>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace {
    struct DataRefInfo {
        std::string name;
        int index = -1;
    };

    DataRefInfo ParseDataRef(const std::string& rawName) {
        size_t bracketPos = rawName.find('[');
        if (bracketPos != std::string::npos) {
            size_t endBracketPos = rawName.find(']', bracketPos);
            if (endBracketPos != std::string::npos) {
                std::string name = rawName.substr(0, bracketPos);
                std::string indexStr = rawName.substr(bracketPos + 1, endBracketPos - bracketPos - 1);
                
                try {
                    return {name, std::stoi(indexStr)};
                } catch (...) {
                    // Fall back to treating it as non-array if parsing fails
                }
            }
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

    // Fallback for logging instead of std::format since it might not be supported in older compilers
    std::string testDescription;
    if (condition.contains("bit")) {
        testDescription = "bit " + std::to_string(condition["bit"].get<int>()) + " set";
    } else if (condition.contains("min") && condition.contains("max")) {
        testDescription = "range [" + std::to_string(condition["min"].get<double>()) + ", " + std::to_string(condition["max"].get<double>()) + "]";
    } else {
        testDescription = "unknown test";
    }

    IFR1_LOG_VERBOSE_IF(m_sdk, verbose, "Testing {} (value: {}) against {} -> {}", rawDrName, val,
        testDescription,
        result ? "TRUE" : "FALSE");

    return result;
}

bool ConditionEvaluator::EvaluateParsedCondition(const ParsedCondition& condition, bool verbose) const {
    if (!condition.drRef) {
        IFR1_LOG_VERBOSE(m_sdk, "Condition failed - DataRef not found: {}", condition.rawName);
        return false;
    }

    int types = m_sdk.GetDataRefTypes(condition.drRef);
    double val = 0.0;
    if (condition.index != -1) {
        if (types & static_cast<int>(DataRefType::IntArray)) {
            val = static_cast<double>(m_sdk.GetDataiArray(condition.drRef, condition.index));
        } else {
            val = static_cast<double>(m_sdk.GetDatafArray(condition.drRef, condition.index));
        }
    } else {
        if (types & static_cast<int>(DataRefType::Int)) {
            val = static_cast<double>(m_sdk.GetDatai(condition.drRef));
        } else {
            val = static_cast<double>(m_sdk.GetDataf(condition.drRef));
        }
    }

    bool result = false;

    if (condition.bit) {
        result = (static_cast<int>(val) & (1 << *condition.bit)) != 0;
    } else if (condition.minVal && condition.maxVal) {
        result = (val >= *condition.minVal && val <= *condition.maxVal);
    }

    IFR1_LOG_VERBOSE_IF(m_sdk, verbose, "Testing {} (value: {}) against {} -> {}", condition.rawName, val,
        [&] {
            if (condition.bit) return std::string("bit ") + std::to_string(*condition.bit) + " set";
            if (condition.minVal && condition.maxVal) return std::string("range [") + std::to_string(*condition.minVal) + ", " + std::to_string(*condition.maxVal) + "]";
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
            return std::all_of(conditions.begin(), conditions.end(),
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
