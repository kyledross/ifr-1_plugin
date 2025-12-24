/*
 *   Copyright 2025 Kyle D. Ross
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

#pragma once
#include "XPlaneSDK.h"
#include <nlohmann/json.hpp>

class ConditionEvaluator {
public:
    explicit ConditionEvaluator(IXPlaneSDK& sdk) : m_sdk(sdk) {}

    /**
     * @brief Evaluates a single condition.
     * @param condition The condition JSON object.
     * @param verbose If true, logs evaluation details to X-Plane debug.
     * @return true if the condition is met, false otherwise.
     */
    [[nodiscard]] bool EvaluateCondition(const nlohmann::json& condition, bool verbose = false) const;

    /**
     * @brief Evaluates a list of conditions. All must be true.
     * @param actionConfig The JSON object that may contain "conditions" or "condition".
     * @param verbose If true, logs evaluation details to X-Plane debug.
     * @return true if all conditions are met, false otherwise.
     */
    [[nodiscard]] bool EvaluateConditions(const nlohmann::json& actionConfig, bool verbose = false) const;

private:
    IXPlaneSDK& m_sdk;
};
