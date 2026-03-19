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

#pragma once
#include "XPlaneSDK.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include "ConditionEvaluator.h"
#include "ParsedCondition.h"

class OutputProcessor {
public:
    explicit OutputProcessor(IXPlaneSDK& sdk) : m_sdk(sdk), m_evaluator(sdk) {}

    /**
     * @brief Parses the output configuration and resolves datarefs.
     * @param config The full aircraft configuration JSON.
     */
    void ParseOutputConfig(const nlohmann::json& config);

    /**
     * @brief Evaluates LED states based on parsed conditions.
     * @param currentTime Current time in seconds (for blinking).
     * @return 8-bit mask of LEDs to be lit.
     */
    [[nodiscard]] uint8_t EvaluateLEDs(float currentTime) const;

private:
    IXPlaneSDK& m_sdk;
    ConditionEvaluator m_evaluator;

    struct ParsedLED {
        uint8_t mask;
        std::vector<ParsedCondition> conditions;
    };
    std::vector<ParsedLED> m_parsedLEDs;
};
