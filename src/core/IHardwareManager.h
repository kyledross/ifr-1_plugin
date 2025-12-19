#pragma once
#include <cstdint>
#include <vector>

class IHardwareManager {
public:
    virtual ~IHardwareManager() = default;

    virtual bool Connect(uint16_t vendorId, uint16_t productId) = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    virtual int Read(uint8_t* data, size_t length, int timeoutMs) = 0;
    virtual int Write(const uint8_t* data, size_t length) = 0;
};
