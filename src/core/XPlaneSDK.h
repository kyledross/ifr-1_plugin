#pragma once

// Dataref types from XPLMDataAccess.h
enum class DataRefType {
    Unknown = 0,
    Int = 1,
    Float = 2,
    Double = 4,
    FloatArray = 8,
    IntArray = 16,
    Data = 32
};

enum class LogLevel {
    Error = 0,
    Info = 1,
    Verbose = 2
};

/**
 * @brief Interface for X-Plane SDK functions to allow mocking in unit tests.
 */
class IXPlaneSDK {
public:
    virtual ~IXPlaneSDK() = default;

    // Data Access
    virtual void* FindDataRef(const char* name) = 0;
    virtual int GetDataRefTypes(void* dataRef) = 0;
    virtual int GetDatai(void* dataRef) = 0;
    virtual void SetDatai(void* dataRef, int value) = 0;
    virtual float GetDataf(void* dataRef) = 0;
    virtual void SetDataf(void* dataRef, float value) = 0;
    virtual int GetDatab(void* dataRef, void* outData, int offset, int maxLength) = 0;

    // Commands
    virtual void* FindCommand(const char* name) = 0;
    virtual void CommandOnce(void* commandRef) = 0;
    virtual void CommandBegin(void* commandRef) = 0;
    virtual void CommandEnd(void* commandRef) = 0;

    // Utilities
    virtual void Log(LogLevel level, const char* string) = 0;
    virtual void SetLogLevel(LogLevel level) = 0;
    virtual LogLevel GetLogLevel() const = 0;
    virtual float GetElapsedTime() = 0;
};

#include <memory>
std::unique_ptr<IXPlaneSDK> CreateXPlaneSDK();
