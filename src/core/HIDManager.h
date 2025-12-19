#pragma once
#include "IHardwareManager.h"
#include <hidapi/hidapi.h>

class HIDManager : public IHardwareManager {
public:
    HIDManager();
    ~HIDManager() override;

    bool Connect(uint16_t vendorId, uint16_t productId) override;
    void Disconnect() override;
    bool IsConnected() const override;

    int Read(uint8_t* data, size_t length, int timeoutMs) override;
    int Write(const uint8_t* data, size_t length) override;

private:
    hid_device* m_device;
};
