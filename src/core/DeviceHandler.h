#pragma once
#include "IHardwareManager.h"
#include "IFR1Protocol.h"
#include "EventProcessor.h"
#include "OutputProcessor.h"
#include "XPlaneSDK.h"
#include "ThreadSafeQueue.h"
#include <array>
#include <thread>
#include <atomic>
#include <vector>

class DeviceHandler {
public:
    DeviceHandler(IHardwareManager& hw, EventProcessor& eventProc, OutputProcessor& outputProc, IXPlaneSDK& sdk, bool startThread = true);
    ~DeviceHandler();

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

    /**
     * @brief Turns off all LEDs and clears the flash bit.
     */
    void ClearLEDs();

    /**
     * @brief Performs one iteration of the hardware communication logic.
     * Used by the worker thread or manually in tests.
     */
    void ProcessHardware();

private:
    void ProcessReport(const IFR1::HardwareEvent& event, const nlohmann::json& config, float currentTime);
    static std::string GetModeString(IFR1::Mode mode, bool shifted);
    static std::string GetControlString(IFR1::Button button);
    void HandleKnobs(const IFR1::HardwareEvent& event, const nlohmann::json& config) const;
    void HandleButtons(const IFR1::HardwareEvent& event, const nlohmann::json& config, float currentTime);
    
    void WorkerThread();
    static IFR1::HardwareEvent ParseReport(const uint8_t* data);

    IHardwareManager& m_hw;
    EventProcessor& m_eventProc;
    OutputProcessor& m_outputProc;
    IXPlaneSDK& m_sdk;

    // Threading
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    ThreadSafeQueue<IFR1::HardwareEvent> m_inputQueue;
    ThreadSafeQueue<uint8_t> m_outputQueue;
    std::atomic<bool> m_isConnected{false};
    std::atomic<uint32_t> m_totalWrites{0};
    std::atomic<uint32_t> m_failedWrites{0};
    uint32_t m_lastTotalWrites = 0;
    uint32_t m_lastFailedWrites = 0;
    float m_lastStatsLogTime = 0.0f;

    // State
    IFR1::Mode m_currentMode = IFR1::Mode::COM1;
    bool m_shifted = false;
    uint8_t m_lastLedBits = 0;
    bool m_lastConnectedState = false;
    
    // Button state tracking
    struct ButtonState {
        bool currentlyHeld = false;
        float pressStartTime = 0.0f;
        bool longPressDetected = false;
    };
    std::array<ButtonState, 12> m_buttonStates;
    std::vector<int> m_heldButtons;
    
    // Last raw report to detect changes
    std::array<uint8_t, IFR1::HID_REPORT_SIZE> m_lastReport{};

    std::string m_clickSoundPath;
    bool m_clickSoundExists = false;
};
