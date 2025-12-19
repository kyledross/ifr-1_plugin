#include "HIDManager.h"

HIDManager::HIDManager() : m_device(nullptr) {
    hid_init();
}

HIDManager::~HIDManager() {
    HIDManager::Disconnect();
    hid_exit();
}

bool HIDManager::Connect(uint16_t vendorId, uint16_t productId) {
    Disconnect();
    m_device = hid_open(vendorId, productId, nullptr);
    if (m_device) {
        hid_set_nonblocking(m_device, 1);
    }
    return m_device != nullptr;
}

void HIDManager::Disconnect() {
    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
    }
}

bool HIDManager::IsConnected() const {
    return m_device != nullptr;
}

int HIDManager::Read(uint8_t* data, size_t length, int timeoutMs) {
    if (!m_device) return -1;
    return hid_read_timeout(m_device, data, length, timeoutMs);
}

int HIDManager::Write(const uint8_t* data, size_t length) {
    if (!m_device) return -1;
    return hid_write(m_device, data, length);
}
