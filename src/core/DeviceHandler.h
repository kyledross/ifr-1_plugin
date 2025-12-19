#pragma once
#include "IHardwareManager.h"
#include "IFR1Protocol.h"
#include "EventProcessor.h"
#include "OutputProcessor.h"
#include "XPlaneSDK.h"
#include <memory>
#include <map>
#include <array>

class DeviceHandler {
public:
    DeviceHandler(IHardwareManager& hw, EventProcessor& eventProc, OutputProcessor& outputProc, IXPlaneSDK& sdk);

    /**
     * @brief Polls the device and processes any new data.
     * @param config The current aircraft configuration.
     * @param currentTime Current time in seconds.
     */
    void Update(const nlohmann::json& config, float currentTime);

    /**
     * @brief Updates the LEDs on the device.
     * @param config The current aircraft configuration.
     * @param currentTime Current time in seconds.
     */
    void UpdateLEDs(const nlohmann::json& config, float currentTime);

private:
    void ProcessReport(const uint8_t* data, const nlohmann::json& config, float currentTime);
    void HandleKnobs(const IFR1::HardwareEvent& event, const nlohmann::json& config);
    void HandleButtons(const IFR1::HardwareEvent& event, const nlohmann::json& config);
    
    std::string GetModeString(IFR1::Mode mode, bool shifted) const;
    std::string GetControlString(IFR1::Button button) const;

    IHardwareManager& m_hw;
    EventProcessor& m_eventProc;
    OutputProcessor& m_outputProc;
    IXPlaneSDK& m_sdk;

    // State
    IFR1::Mode m_currentMode = IFR1::Mode::COM1;
    bool m_shifted = false;
    uint8_t m_lastLedBits = 0;
    
    // Button state tracking
    struct ButtonState {
        bool currentlyHeld = false;
        float pressStartTime = 0.0f;
        bool longPressDetected = false;
    };
    std::array<ButtonState, 12> m_buttonStates;
    
    // Last raw report to detect changes
    std::array<uint8_t, IFR1::HID_REPORT_SIZE> m_lastReport{};
};
