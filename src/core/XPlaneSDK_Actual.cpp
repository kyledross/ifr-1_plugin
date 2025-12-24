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

#include "XPlaneSDK.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMProcessing.h"
#include "XPLMSound.h"
#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <cstring>
#include <filesystem>
#include <format>

struct SoundBuffer {
    std::vector<char> data;
    int frequency{};
    int channels{};
};

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
    
    int GetDataiArray(void* dataRef, int index) override {
        int val = 0;
        XPLMGetDatavi(static_cast<XPLMDataRef>(dataRef), &val, index, 1);
        return val;
    }

    void SetDataiArray(void* dataRef, int value, int index) override {
        XPLMSetDatavi(static_cast<XPLMDataRef>(dataRef), &value, index, 1);
    }

    float GetDatafArray(void* dataRef, int index) override {
        float val = 0.0f;
        XPLMGetDatavf(static_cast<XPLMDataRef>(dataRef), &val, index, 1);
        return val;
    }

    void SetDatafArray(void* dataRef, float value, int index) override {
        XPLMSetDatavf(static_cast<XPLMDataRef>(dataRef), &value, index, 1);
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

    [[nodiscard]] LogLevel GetLogLevel() const override {
        return m_logLevel;
    }

    float GetElapsedTime() override {
        return XPLMGetElapsedTime();
    }

    std::string GetSystemPath() override {
        char path[512];
        XPLMGetSystemPath(path);
        return std::string{path};
    }

    bool FileExists(const std::string& path) override {
        return std::filesystem::exists(path);
    }

    void PlaySound(const std::string& path) override {
        auto it = m_sounds.find(path);
        if (it == m_sounds.end()) {
            SoundBuffer buffer;
            if (LoadWav(path, buffer)) {
                it = m_sounds.emplace(path, std::move(buffer)).first;
            } else {
                Log(LogLevel::Error, std::format("Failed to load sound: {}", path).c_str());
                return;
            }
        }

        const auto& buf = it->second;
        XPLMPlayPCMOnBus(
            const_cast<char*>(buf.data.data()),
            static_cast<uint32_t>(buf.data.size()),
            FMOD_SOUND_FORMAT_PCM16,
            buf.frequency,
            buf.channels,
            0, // loop
            xplm_AudioUI,
            nullptr, // callback
            nullptr  // refcon
        );
    }

private:
    LogLevel m_logLevel = LogLevel::Info;
    std::map<std::string, SoundBuffer> m_sounds;

    static bool LoadWav(const std::string& path, SoundBuffer& buffer) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;

        auto read4 = [&](char* out) { return file.read(out, 4).gcount() == 4; };
        auto read2 = [&](uint16_t& out) { return file.read(reinterpret_cast<char*>(&out), 2).gcount() == 2; };
        auto read4u = [&](uint32_t& out) { return file.read(reinterpret_cast<char*>(&out), 4).gcount() == 4; };

        char id[4];
        if (!read4(id) || std::memcmp(id, "RIFF", 4) != 0) return false;
        uint32_t riffSize;
        if (!read4u(riffSize)) return false;
        if (!read4(id) || std::memcmp(id, "WAVE", 4) != 0) return false;

        bool fmtFound = false;
        bool dataFound = false;

        while (read4(id)) {
            uint32_t chunkSize;
            if (!read4u(chunkSize)) break;

            if (std::memcmp(id, "fmt ", 4) == 0) {
                uint16_t formatTag;
                if (!read2(formatTag) || formatTag != 1) return false; // Not PCM
                uint16_t channels;
                if (!read2(channels)) return false;
                buffer.channels = channels;
                uint32_t frequency;
                if (!read4u(frequency)) return false;
                buffer.frequency = static_cast<int>(frequency);
                file.seekg(6, std::ios::cur); // Skip bytes/sec and block align
                uint16_t bitsPerSample;
                if (!read2(bitsPerSample) || bitsPerSample != 16) return false;
                if (chunkSize > 16) file.seekg(chunkSize - 16, std::ios::cur);
                fmtFound = true;
            } else if (std::memcmp(id, "data", 4) == 0) {
                buffer.data.resize(chunkSize);
                file.read(buffer.data.data(), chunkSize);
                dataFound = true;
                break; // Found data, we can stop
            } else {
                file.seekg(chunkSize, std::ios::cur);
            }
        }

        return fmtFound && dataFound;
    }
};

std::unique_ptr<IXPlaneSDK> CreateXPlaneSDK() {
    return std::make_unique<XPlaneSDK>();
}
