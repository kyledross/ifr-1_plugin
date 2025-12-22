#include "OutputProcessor.h"
#include "IFR1Protocol.h"
#include <cmath>

uint8_t OutputProcessor::EvaluateLEDs(const nlohmann::json& config, float currentTime) const
{
    if (config.empty() || !config.contains("output")) {
        if (!config.empty() && config.is_object() && m_sdk.GetLogLevel() >= LogLevel::Verbose) {
            std::string keys;
            for (auto it = config.begin(); it != config.end(); ++it) {
                keys += it.key() + ", ";
            }
            m_sdk.Log(LogLevel::Verbose, ("Config missing 'output'. Keys: " + keys).c_str());
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
