#include "OutputProcessor.h"
#include "IFR1Protocol.h"
#include <cmath>

uint8_t OutputProcessor::EvaluateLEDs(const nlohmann::json& config, float currentTime) const
{
    if (config.empty() || !config.contains("output")) {
        return IFR1::LEDMask::OFF;
    }

    uint8_t ledBits = IFR1::LEDMask::OFF;
    const auto& output = config["output"];

    auto processLED = [&](const std::string& name, uint8_t mask) {
        if (output.contains(name) && output[name].contains("conditions")) {
            for (const auto& condition : output[name]["conditions"]) {
                if (EvaluateCondition(condition)) {
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

bool OutputProcessor::EvaluateCondition(const nlohmann::json& condition) const
{
    if (!condition.contains("dataref")) return false;

    std::string drName = condition["dataref"];
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

    if (condition.contains("bit")) {
        int bit = condition["bit"].get<int>();
        return (static_cast<int>(val) & (1 << bit)) != 0;
    } else if (condition.contains("min") && condition.contains("max")) {
        double minVal = condition["min"].get<double>();
        double maxVal = condition["max"].get<double>();
        return (val >= minVal && val <= maxVal);
    }

    return false;
}
