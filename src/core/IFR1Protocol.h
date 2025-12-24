/*
 *   Copyright 2025 Kyle D. Ross
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
#include <cstdint>

namespace IFR1 {

constexpr uint16_t VENDOR_ID = 0x04D8;
constexpr uint16_t PRODUCT_ID = 0xE6D6;
constexpr uint8_t HID_REPORT_SIZE = 9;
constexpr uint8_t HID_LED_REPORT_ID = 11;
constexpr float DEFAULT_BLINK_RATE_HZ = 1.0f;

enum class Button : uint8_t {
    DIRECT = 0,
    MENU,
    CLR,
    ENT,
    SWAP,
    AP,
    HDG,
    NAV,
    APR,
    ALT,
    VS,
    INNER_KNOB
};

enum class Mode : uint8_t { 
    COM1 = 0, 
    COM2, 
    NAV1, 
    NAV2, 
    FMS1, 
    FMS2, 
    AP, 
    XPDR 
};

struct HardwareEvent {
    int8_t outerKnobRotation = 0;
    int8_t innerKnobRotation = 0;
    Mode mode = Mode::COM1;
    bool buttonStates[12]{};
    
    // One-shot press indicators (helper for EventProcessor)
    bool shortPress[12]{};
    bool longPress[12]{};
};

namespace LEDMask {
    constexpr uint8_t OFF = 0x00;
    constexpr uint8_t AP = 0x01;
    constexpr uint8_t HDG = 0x02;
    constexpr uint8_t NAV = 0x04;
    constexpr uint8_t APR = 0x08;
    constexpr uint8_t ALT = 0x10;
    constexpr uint8_t VS = 0x20;
    constexpr uint8_t MODE_FLASH = 0x40;
}

namespace BitPosition {
    // Right buttons (in data[1])
    constexpr uint8_t DIRECT = 5;
    constexpr uint8_t MENU = 6;
    constexpr uint8_t CLR = 7;
    constexpr uint8_t ENT = 8;

    // Bottom-left buttons (in data[2])
    constexpr uint8_t SWAP = 1;
    constexpr uint8_t INNER_KNOB = 2;
    constexpr uint8_t AP = 7;
    constexpr uint8_t HDG = 8;

    // Bottom-right buttons (in data[3])
    constexpr uint8_t NAV = 1;
    constexpr uint8_t APR = 2;
    constexpr uint8_t ALT = 3;
    constexpr uint8_t VS = 4;
}

} // namespace IFR1
