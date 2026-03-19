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

#include "OutputProcessor.h"
#include "IFR1Protocol.h"
#include "Logger.h"
#include <cmath>

namespace {
    std::pair<std::string, int> ParseDataRefStr(const std::string& rawName) {
        size_t bracketPos = rawName.find('[');
        if (bracketPos != std::string::npos) {
            size_t endBracketPos = rawName.find(']', bracketPos);
            if (endBracketPos != std::string::npos) {
                std::string name = rawName.substr(0, bracketPos);
                std::string indexStr = rawName.substr(bracketPos + 1, endBracketPos - bracketPos - 1);
                try {
                    return {name, std::stoi(indexStr)};
                } catch (...) { }
            }
        }
        return {rawName, -1};
    }
}

void OutputProcessor::ParseOutputConfig(const nlohmann::json& config) {
    m_parsedLEDs.clear();

    if (config.empty() || !config.contains("output")) {
        return;
    }

    const auto& output = config["output"];

    auto parseLED = [&](const std::string& name, uint8_t mask) {
        if (output.contains(name) && output[name].contains("conditions")) {
            ParsedLED ledDef;
            ledDef.mask = mask;

            for (const auto& condition : output[name]["conditions"]) {
                if (!condition.contains("dataref")) continue;

                ParsedCondition parsed;
                parsed.rawName = condition["dataref"];
                
                auto info = ParseDataRefStr(parsed.rawName);
                parsed.drRef = m_sdk.FindDataRef(info.first.c_str());
                parsed.index = info.second;

                if (condition.contains("bit")) {
                    parsed.bit = condition["bit"].get<int>();
                } else if (condition.contains("min") && condition.contains("max")) {
                    parsed.minVal = condition["min"].get<double>();
                    parsed.maxVal = condition["max"].get<double>();
                }
                
                parsed.mode = condition.value("mode", "solid");
                if (parsed.mode == "blink") {
                    parsed.blinkRate = condition.contains("blink-rate") ? static_cast<float>(condition["blink-rate"].get<double>()) : IFR1::DEFAULT_BLINK_RATE_HZ;
                }

                ledDef.conditions.push_back(parsed);
            }
            m_parsedLEDs.push_back(ledDef);
        }
    };

    parseLED("ap", IFR1::LEDMask::AP);
    parseLED("hdg", IFR1::LEDMask::HDG);
    parseLED("nav", IFR1::LEDMask::NAV);
    parseLED("apr", IFR1::LEDMask::APR);
    parseLED("alt", IFR1::LEDMask::ALT);
    parseLED("vs", IFR1::LEDMask::VS);
}

uint8_t OutputProcessor::EvaluateLEDs(float currentTime) const
{
    if (m_parsedLEDs.empty()) {
        return IFR1::LEDMask::OFF;
    }

    uint8_t ledBits = IFR1::LEDMask::OFF;
    bool verbose = false; // Hard-coded to false to avoid log spam from high-frequency LED updates

    for (const auto& ledDef : m_parsedLEDs) {
        for (const auto& condition : ledDef.conditions) {
            if (m_evaluator.EvaluateParsedCondition(condition, verbose)) {
                if (condition.mode == "solid") {
                    ledBits |= ledDef.mask;
                } else if (condition.mode == "blink") {
                    if (condition.blinkRate > 0) {
                        float period = 1.0f / condition.blinkRate;
                        float phase = std::fmod(currentTime, period);
                        if (phase < period / 2.0f) {
                            ledBits |= ledDef.mask;
                        }
                    }
                }
                break; // Precedence: first condition met wins
            }
        }
    }

    return ledBits;
}
