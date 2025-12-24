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
#include "ConditionEvaluator.h"

class OutputProcessor {
public:
    explicit OutputProcessor(IXPlaneSDK& sdk) : m_sdk(sdk), m_evaluator(sdk) {}

    /**
     * @brief Evaluates LED states based on the current configuration and X-Plane state.
     * @param config The current aircraft configuration JSON.
     * @param currentTime Current time in seconds (for blinking).
     * @return 8-bit mask of LEDs to be lit.
     */
    [[nodiscard]] uint8_t EvaluateLEDs(const nlohmann::json& config, float currentTime) const;

private:
    IXPlaneSDK& m_sdk;
    ConditionEvaluator m_evaluator;
};
