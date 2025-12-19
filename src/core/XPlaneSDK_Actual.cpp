#include "XPlaneSDK.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMProcessing.h"
#include <memory>
#include <string>

class XPlaneSDK : public IXPlaneSDK {
public:
    void* FindDataRef(const char* name) override {
        return XPLMFindDataRef(name);
    }

    int GetDataRefTypes(void* dataRef) override {
        return XPLMGetDataRefTypes(static_cast<XPLMDataRef>(dataRef));
    }

    int GetDatai(void* dataRef) override {
        return XPLMGetDatai(static_cast<XPLMDataRef>(dataRef));
    }

    void SetDatai(void* dataRef, int value) override {
        XPLMSetDatai(static_cast<XPLMDataRef>(dataRef), value);
    }

    float GetDataf(void* dataRef) override {
        return XPLMGetDataf(static_cast<XPLMDataRef>(dataRef));
    }

    void SetDataf(void* dataRef, float value) override {
        XPLMSetDataf(static_cast<XPLMDataRef>(dataRef), value);
    }

    int GetDatab(void* dataRef, void* outData, int offset, int maxLength) override {
        return XPLMGetDatab(static_cast<XPLMDataRef>(dataRef), outData, offset, maxLength);
    }

    void* FindCommand(const char* name) override {
        return XPLMFindCommand(name);
    }

    void CommandOnce(void* commandRef) override {
        XPLMCommandOnce(static_cast<XPLMCommandRef>(commandRef));
    }

    void CommandBegin(void* commandRef) override {
        XPLMCommandBegin(static_cast<XPLMCommandRef>(commandRef));
    }

    void CommandEnd(void* commandRef) override {
        XPLMCommandEnd(static_cast<XPLMCommandRef>(commandRef));
    }

    void Log(LogLevel level, const char* string) override {
        if (level > m_logLevel) return;

        const char* prefix = "[INFO] ";
        switch (level) {
            case LogLevel::Error: prefix = "[ERROR] "; break;
            case LogLevel::Info: prefix = "[INFO] "; break;
            case LogLevel::Verbose: prefix = "[DEBUG] "; break;
        }
        
        std::string fullMsg = "IFR-1 Flex: ";
        fullMsg += prefix;
        fullMsg += string;
        if (fullMsg.back() != '\n') {
            fullMsg += '\n';
        }
        XPLMDebugString(fullMsg.c_str());
    }

    void SetLogLevel(LogLevel level) override {
        m_logLevel = level;
    }

    LogLevel GetLogLevel() const override {
        return m_logLevel;
    }

    float GetElapsedTime() override {
        return XPLMGetElapsedTime();
    }

private:
    LogLevel m_logLevel = LogLevel::Info;
};

std::unique_ptr<IXPlaneSDK> CreateXPlaneSDK() {
    return std::make_unique<XPlaneSDK>();
}
