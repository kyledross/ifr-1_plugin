#include "XPlaneSDK.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMProcessing.h"
#include <memory>

class XPlaneSDK : public IXPlaneSDK {
public:
    void* FindDataRef(const char* name) override {
        return XPLMFindDataRef(name);
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

    void DebugString(const char* string) override {
        XPLMDebugString(string);
    }

    float GetElapsedTime() override {
        return XPLMGetElapsedTime();
    }
};

std::unique_ptr<IXPlaneSDK> CreateXPlaneSDK() {
    return std::make_unique<XPlaneSDK>();
}
