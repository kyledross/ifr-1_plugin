#pragma once
#include "XPlaneSDK.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class OutputProcessor {
public:
    explicit OutputProcessor(IXPlaneSDK& sdk) : m_sdk(sdk) {}

    /**
     * @brief Evaluates LED states based on the current configuration and X-Plane state.
     * @param config The current aircraft configuration JSON.
     * @param currentTime Current time in seconds (for blinking).
     * @return 8-bit mask of LEDs to be lit.
     */
    uint8_t EvaluateLEDs(const nlohmann::json& config, float currentTime);

private:
    IXPlaneSDK& m_sdk;

    bool EvaluateTest(const nlohmann::json& test);
};
