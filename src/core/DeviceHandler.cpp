#include "DeviceHandler.h"
#include <iostream>

DeviceHandler::DeviceHandler(IHardwareManager& hw, EventProcessor& eventProc, OutputProcessor& outputProc, IXPlaneSDK& sdk)
    : m_hw(hw), m_eventProc(eventProc), m_outputProc(outputProc), m_sdk(sdk) {
    for (auto& state : m_buttonStates) {
        state.currentlyHeld = false;
        state.pressStartTime = 0.0f;
        state.longPressDetected = false;
    }
}

void DeviceHandler::Update(const nlohmann::json& config, float currentTime) {
    if (!m_hw.IsConnected()) {
        if (!m_hw.Connect(IFR1::VENDOR_ID, IFR1::PRODUCT_ID)) {
            return;
        }
        m_sdk.DebugString("IFR-1 Flex: Device connected.\n");
        m_shifted = false; // Reset state on reconnect
    }

    uint8_t buffer[IFR1::HID_REPORT_SIZE + 1];
    int bytesRead;
    int reportsProcessed = 0;

    // Read all available reports
    while ((bytesRead = m_hw.Read(buffer, IFR1::HID_REPORT_SIZE, 0)) > 0) {
        ProcessReport(buffer, config, currentTime);
        reportsProcessed++;
        if (reportsProcessed > 10) break; // Safety break
    }

    if (bytesRead < 0) {
        m_sdk.DebugString("IFR-1 Flex: Device disconnected (read error).\n");
        m_hw.Disconnect();
    }

    // Process long presses even if no new HID report (timer based)
    for (int i = 0; i < 12; ++i) {
        if (m_buttonStates[i].currentlyHeld && !m_buttonStates[i].longPressDetected) {
            if (currentTime - m_buttonStates[i].pressStartTime >= 0.5f) {
                m_buttonStates[i].longPressDetected = true;
                
                IFR1::Button btn = static_cast<IFR1::Button>(i);
                if (btn == IFR1::Button::INNER_KNOB) {
                    m_shifted = !m_shifted;
                } else {
                    m_eventProc.ProcessEvent(config, GetModeString(m_currentMode, m_shifted), GetControlString(btn), "long-press");
                }
            }
        }
    }
}

void DeviceHandler::UpdateLEDs(const nlohmann::json& config, float currentTime) {
    if (!m_hw.IsConnected()) return;

    uint8_t ledBits = m_outputProc.EvaluateLEDs(config, currentTime);
    
    // Add mode flash bit if shifted
    if (m_shifted) {
        ledBits |= IFR1::LEDMask::MODE_FLASH;
    }

    if (ledBits != m_lastLedBits) {
        uint8_t report[2] = { IFR1::HID_LED_REPORT_ID, ledBits };
        m_hw.Write(report, 2);
        m_lastLedBits = ledBits;
    }
}

void DeviceHandler::ProcessReport(const uint8_t* data, const nlohmann::json& config, float currentTime) {
    // data[1..3] buttons, data[5] outer knob, data[6] inner knob, data[7] mode
    
    IFR1::HardwareEvent event;
    event.outerKnobRotation = static_cast<int8_t>(data[5]);
    event.innerKnobRotation = static_cast<int8_t>(data[6]);
    event.mode = static_cast<IFR1::Mode>(data[7]);
    
    if (event.mode != m_currentMode) {
        m_shifted = false;
        m_currentMode = event.mode;
    }

    auto checkBit = [](uint8_t val, uint8_t bit) {
        return (val & (1 << (bit - 1))) != 0;
    };

    event.buttonStates[static_cast<int>(IFR1::Button::DIRECT)] = checkBit(data[1], IFR1::BitPosition::DIRECT);
    event.buttonStates[static_cast<int>(IFR1::Button::MENU)] = checkBit(data[1], IFR1::BitPosition::MENU);
    event.buttonStates[static_cast<int>(IFR1::Button::CLR)] = checkBit(data[1], IFR1::BitPosition::CLR);
    event.buttonStates[static_cast<int>(IFR1::Button::ENT)] = checkBit(data[1], IFR1::BitPosition::ENT);

    event.buttonStates[static_cast<int>(IFR1::Button::SWAP)] = checkBit(data[2], IFR1::BitPosition::SWAP);
    event.buttonStates[static_cast<int>(IFR1::Button::INNER_KNOB)] = checkBit(data[2], IFR1::BitPosition::INNER_KNOB);
    event.buttonStates[static_cast<int>(IFR1::Button::AP)] = checkBit(data[2], IFR1::BitPosition::AP);
    event.buttonStates[static_cast<int>(IFR1::Button::HDG)] = checkBit(data[2], IFR1::BitPosition::HDG);

    event.buttonStates[static_cast<int>(IFR1::Button::NAV)] = checkBit(data[3], IFR1::BitPosition::NAV);
    event.buttonStates[static_cast<int>(IFR1::Button::APR)] = checkBit(data[3], IFR1::BitPosition::APR);
    event.buttonStates[static_cast<int>(IFR1::Button::ALT)] = checkBit(data[3], IFR1::BitPosition::ALT);
    event.buttonStates[static_cast<int>(IFR1::Button::VS)] = checkBit(data[3], IFR1::BitPosition::VS);

    // Handle knobs
    if (event.outerKnobRotation != 0) {
        std::string action = (event.outerKnobRotation > 0) ? "rotate-clockwise" : "rotate-counterclockwise";
        m_sdk.DebugString(("IFR-1 Flex: Outer knob " + action + "\n").c_str());
        for (int i = 0; i < std::abs(event.outerKnobRotation); ++i) {
            m_eventProc.ProcessEvent(config, GetModeString(m_currentMode, m_shifted), "outer-knob", action);
        }
    }
    if (event.innerKnobRotation != 0) {
        std::string action = (event.innerKnobRotation > 0) ? "rotate-clockwise" : "rotate-counterclockwise";
        m_sdk.DebugString(("IFR-1 Flex: Inner knob " + action + "\n").c_str());
        for (int i = 0; i < std::abs(event.innerKnobRotation); ++i) {
            m_eventProc.ProcessEvent(config, GetModeString(m_currentMode, m_shifted), "inner-knob", action);
        }
    }

    // Handle button transitions
    for (int i = 0; i < 12; ++i) {
        bool current = event.buttonStates[i];
        bool last = m_buttonStates[i].currentlyHeld;
        
        if (current && !last) {
            // Pressed
            m_sdk.DebugString(("IFR-1 Flex: Button " + GetControlString(static_cast<IFR1::Button>(i)) + " pressed\n").c_str());
            m_buttonStates[i].currentlyHeld = true;
            m_buttonStates[i].pressStartTime = currentTime;
            m_buttonStates[i].longPressDetected = false;
        } else if (!current && last) {
            // Released
            m_sdk.DebugString(("IFR-1 Flex: Button " + GetControlString(static_cast<IFR1::Button>(i)) + " released\n").c_str());
            if (!m_buttonStates[i].longPressDetected) {
                // Short press
                m_eventProc.ProcessEvent(config, GetModeString(m_currentMode, m_shifted), GetControlString(static_cast<IFR1::Button>(i)), "short-press");
            }
            m_buttonStates[i].currentlyHeld = false;
            m_buttonStates[i].longPressDetected = false;
        }
    }
}

std::string DeviceHandler::GetModeString(IFR1::Mode mode, bool shifted) const {
    if (!shifted) {
        switch (mode) {
            case IFR1::Mode::COM1: return "com1";
            case IFR1::Mode::COM2: return "com2";
            case IFR1::Mode::NAV1: return "nav1";
            case IFR1::Mode::NAV2: return "nav2";
            case IFR1::Mode::FMS1: return "fms1";
            case IFR1::Mode::FMS2: return "fms2";
            case IFR1::Mode::AP: return "ap";
            case IFR1::Mode::XPDR: return "xpdr";
        }
    } else {
        switch (mode) {
            case IFR1::Mode::COM1: return "hdg";
            case IFR1::Mode::COM2: return "baro";
            case IFR1::Mode::NAV1: return "crs1";
            case IFR1::Mode::NAV2: return "crs2";
            case IFR1::Mode::FMS1: return "fms1-alt";
            case IFR1::Mode::FMS2: return "fms2-alt";
            case IFR1::Mode::AP: return "ap-alt";
            case IFR1::Mode::XPDR: return "xpdr-mode";
        }
    }
    return "";
}

std::string DeviceHandler::GetControlString(IFR1::Button button) const {
    switch (button) {
        case IFR1::Button::DIRECT: return "direct-to";
        case IFR1::Button::MENU: return "menu";
        case IFR1::Button::CLR: return "clr";
        case IFR1::Button::ENT: return "ent";
        case IFR1::Button::SWAP: return "swap";
        case IFR1::Button::AP: return "ap";
        case IFR1::Button::HDG: return "hdg";
        case IFR1::Button::NAV: return "nav";
        case IFR1::Button::APR: return "apr";
        case IFR1::Button::ALT: return "alt";
        case IFR1::Button::VS: return "vs";
        case IFR1::Button::INNER_KNOB: return "inner-knob-button";
        default: return "";
    }
}
