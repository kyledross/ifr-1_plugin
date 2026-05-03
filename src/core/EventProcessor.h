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

#pragma once
#include "XPlaneSDK.h"
#include <nlohmann/json.hpp>
#include <string>
#include <queue>
#include <unordered_map>
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
     * @brief Pre-computes a flat lookup map from the aircraft configuration.
     * Call this whenever the aircraft configuration changes.  ProcessEvent will
     * use the map for O(1) dispatch instead of traversing the JSON hierarchy.
     * @param config The full aircraft configuration JSON.
     */
    void PrepareConfig(const nlohmann::json& config);

    /**
     * @brief Processes one command from the FIFO queue.
     * Should be called once per frame.
     */
    void ProcessQueue();

private:
    IXPlaneSDK& m_sdk;
    ConditionEvaluator m_evaluator;
    std::queue<void*> m_commandQueue;
    // Keyed by "mode\x1Fcontrol\x1Faction"; populated by PrepareConfig
    std::unordered_map<std::string, nlohmann::json> m_actionMap;

    void ExecuteAction(const nlohmann::json& actionConfig);
    static bool ShouldEvaluateNext(const nlohmann::json& actionConfig);
};
