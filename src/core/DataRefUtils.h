/*
 *   Copyright 2026 Kyle D. Ross
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
#include <string>

/**
 * @brief Result of parsing a dataref name that may include an array index.
 *
 * For a plain name like "sim/cockpit/autopilot/heading_mag", index is -1.
 * For an array name like "sim/some/array[3]", name is the bare dataref name
 * and index is 3.
 */
struct ParsedDataRef {
    std::string name;
    int index = -1;
};

/**
 * @brief Parses a raw dataref string that may contain an array index suffix.
 *
 * Handles the form "dataref_name[N]".  If parsing the index fails or the
 * brackets are malformed, the full rawName is returned with index = -1.
 */
inline ParsedDataRef ParseDataRef(const std::string& rawName) {
    const size_t bracketPos = rawName.find('[');
    if (bracketPos != std::string::npos) {
        const size_t endBracketPos = rawName.find(']', bracketPos);
        if (endBracketPos != std::string::npos) {
            std::string baseName = rawName.substr(0, bracketPos);
            std::string indexStr = rawName.substr(bracketPos + 1, endBracketPos - bracketPos - 1);
            try {
                return {std::move(baseName), std::stoi(indexStr)};
            } catch (...) {
                // Fall back to treating the whole string as a plain name
            }
        }
    }
    return {rawName, -1};
}
