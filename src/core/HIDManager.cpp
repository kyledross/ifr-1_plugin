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

#include "HIDManager.h"

HIDManager::HIDManager() : m_device(nullptr) {
    hid_init();
}

HIDManager::~HIDManager() {
    HIDManager::Disconnect();
    hid_exit();
}

bool HIDManager::Connect(uint16_t vendorId, uint16_t productId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    DisconnectInternal();
    m_device = hid_open(vendorId, productId, nullptr);
    if (m_device) {
        hid_set_nonblocking(m_device, 0);
    }
    return m_device != nullptr;
}

void HIDManager::Disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    DisconnectInternal();
}

void HIDManager::DisconnectInternal() {
    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
    }
}

bool HIDManager::IsConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_device != nullptr;
}

int HIDManager::Read(uint8_t* data, size_t length, int timeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_device) return -1;
    return hid_read_timeout(m_device, data, length, timeoutMs);
}

int HIDManager::Write(const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_device) return -1;
    return hid_write(m_device, data, length);
}
