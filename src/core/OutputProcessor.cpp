#include "OutputProcessor.h"
#include "IFR1Protocol.h"
#include <cmath>

uint8_t OutputProcessor::EvaluateLEDs(const nlohmann::json& config, float currentTime) {
    if (config.empty() || !config.contains("output")) {
        return IFR1::LEDMask::OFF;
    }

    uint8_t ledBits = IFR1::LEDMask::OFF;
    const auto& output = config["output"];

    auto processLED = [&](const std::string& name, uint8_t mask) {
        if (output.contains(name) && output[name].contains("tests")) {
            for (const auto& test : output[name]["tests"]) {
                if (EvaluateTest(test)) {
                    std::string mode = test.value("mode", "solid");
                    if (mode == "solid") {
                        ledBits |= mask;
                    } else if (mode == "blink") {
                        float blinkRate = test.value("blink-rate", 0.5f);
                        if (blinkRate > 0) {
                            float period = 1.0f / blinkRate;
                            float phase = std::fmod(currentTime, period);
                            if (phase < period / 2.0f) {
                                ledBits |= mask;
                            }
                        }
                    }
                    return; // Precedence: first test met wins
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

bool OutputProcessor::EvaluateTest(const nlohmann::json& test) {
    if (!test.contains("dataref")) return false;

    std::string drName = test["dataref"];
    void* drRef = m_sdk.FindDataRef(drName.c_str());
    if (!drRef) return false;

    // We'll read it as float by default, or int if bit is provided
    if (test.contains("bit")) {
        int val = m_sdk.GetDatai(drRef);
        int bit = test["bit"].get<int>();
        // IFR1Protocol.h says bit is 1-based usually? 
        // Wait, IFR-1_Native's HasBit used (1U << (bit - 1)).
        // But our JSON bit might be 0-based or a mask.
        // Let's assume it's a bit index (0-based) for now, or check documentation.
        // documentation/aircraft_configs.md says: "If a bit is provided, the dataref value will be checked if that bit is set (using bitwise AND)."
        // This suggests it might be a mask? Or an index.
        // IFR-1_Native's beechcraft.json used "bit": 1 for heading (bit 1 of autopilot_state is 2).
        // Wait, bit 1 usually means 2^1 = 2.
        
        return (val & (1 << bit)) != 0;
    } else if (test.contains("min") && test.contains("max")) {
        float val = m_sdk.GetDataf(drRef);
        float minVal = test["min"].get<float>();
        float maxVal = test["max"].get<float>();
        return (val >= minVal && val <= maxVal);
    }

    return false;
}
