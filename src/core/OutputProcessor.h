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
