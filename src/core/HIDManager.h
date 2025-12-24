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
#include "IHardwareManager.h"
#include <hidapi/hidapi.h>

#include <mutex>

class HIDManager : public IHardwareManager {
public:
    HIDManager();
    ~HIDManager() override;

    bool Connect(uint16_t vendorId, uint16_t productId) override;
    void Disconnect() override;
    [[nodiscard]] bool IsConnected() const override;

    int Read(uint8_t* data, size_t length, int timeoutMs) override;
    int Write(const uint8_t* data, size_t length) override;

private:
    void DisconnectInternal();
    hid_device* m_device;
    mutable std::mutex m_mutex;
};
