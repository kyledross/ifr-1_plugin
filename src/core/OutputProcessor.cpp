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

    // We'll read it based on its type
    int types = m_sdk.GetDataRefTypes(drRef);
    double val = 0.0;
    if (types & static_cast<int>(DataRefType::Int)) {
        val = static_cast<double>(m_sdk.GetDatai(drRef));
    } else {
        // Fallback to float for everything else
        val = static_cast<double>(m_sdk.GetDataf(drRef));
    }

    if (test.contains("bit")) {
        int bit = test["bit"].get<int>();
        return (static_cast<int>(val) & (1 << bit)) != 0;
    } else if (test.contains("min") && test.contains("max")) {
        double minVal = test["min"].get<double>();
        double maxVal = test["max"].get<double>();
        return (val >= minVal && val <= maxVal);
    }

    return false;
}
