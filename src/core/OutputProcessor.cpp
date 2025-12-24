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

#include "OutputProcessor.h"
#include "IFR1Protocol.h"
#include "Logger.h"
#include <cmath>

uint8_t OutputProcessor::EvaluateLEDs(const nlohmann::json& config, float currentTime) const
{
    if (config.empty() || !config.contains("output")) {
        if (!config.empty() && config.is_object()) {
            IFR1_LOG_VERBOSE(m_sdk, "Config missing 'output'. Keys: {}", [&] {
                std::string keys;
                for (auto it = config.begin(); it != config.end(); ++it) {
                    keys += it.key() + ", ";
                }
                return keys;
            }());
        }
        return IFR1::LEDMask::OFF;
    }

    uint8_t ledBits = IFR1::LEDMask::OFF;
    const auto& output = config["output"];
    bool verbose = false; // config.value("debug", false); // Hard-coded to false to avoid log spam from high-frequency LED updates

    auto processLED = [&](const std::string& name, uint8_t mask) {
        if (output.contains(name) && output[name].contains("conditions")) {
            for (const auto& condition : output[name]["conditions"]) {
                if (m_evaluator.EvaluateCondition(condition, verbose)) {
                    std::string mode = condition.value("mode", "solid");
                    if (mode == "solid") {
                        ledBits |= mask;
                    } else if (mode == "blink") {
                        float blinkRate = condition.value("blink-rate", IFR1::DEFAULT_BLINK_RATE_HZ);
                        if (blinkRate > 0) {
                            float x = 1.0f;
                            float period = x / blinkRate;
                            float phase = std::fmod(currentTime, period);
                            if (phase < period / 2.0f) {
                                ledBits |= mask;
                            }
                        }
                    }
                    return; // Precedence: first condition met wins
                }
            }
        }
    };

    processLED("ap", IFR1::LEDMask::AP);
    processLED("hdg", IFR1::LEDMask::HDG);
    processLED("nav", IFR1::LEDMask::NAV);
    processLED("apr", IFR1::LEDMask::APR);
    processLED("alt", IFR1::LEDMask::ALT);
    processLED("vs", IFR1::LEDMask::VS);

    return ledBits;
}
