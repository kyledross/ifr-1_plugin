#pragma once
#include "XPlaneSDK.h"
#include <nlohmann/json.hpp>
#include <string>

class EventProcessor {
public:
    explicit EventProcessor(IXPlaneSDK& sdk) : m_sdk(sdk) {}

    /**
     * @brief Processes an input event based on the current configuration.
     * @param config The current aircraft configuration JSON.
     * @param mode The current mode (e.g., "com1").
     * @param control The control being used (e.g., "inner-knob").
     * @param action The action performed (e.g., "rotate-clockwise").
     */
    void ProcessEvent(const nlohmann::json& config, 
                      const std::string& mode, 
                      const std::string& control, 
                      const std::string& action) const;

private:
    IXPlaneSDK& m_sdk;
};
