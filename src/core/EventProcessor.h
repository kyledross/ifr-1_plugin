#pragma once
#include "XPlaneSDK.h"
#include <nlohmann/json.hpp>
#include <string>
#include <queue>
#include "ConditionEvaluator.h"

class EventProcessor {
public:
    explicit EventProcessor(IXPlaneSDK& sdk) : m_sdk(sdk), m_evaluator(sdk) {}

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
                      const std::string& action);

    /**
     * @brief Processes one command from the FIFO queue.
     * Should be called once per frame.
     */
    void ProcessQueue();

private:
    IXPlaneSDK& m_sdk;
    ConditionEvaluator m_evaluator;
    std::queue<void*> m_commandQueue;

    void ExecuteAction(const nlohmann::json& actionConfig);
    static bool ShouldEvaluateNext(const nlohmann::json& actionConfig);
};
