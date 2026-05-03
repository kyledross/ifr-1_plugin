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
#include <algorithm>

/**
 * @brief Computed geometry for a scrollbar thumb within its track.
 *
 * All coordinates are in the same space as the track bounds passed to
 * ComputeScrollbarThumb().
 */
struct ScrollbarThumbMetrics {
    int thumbH    = 0;  ///< Height of the thumb in pixels
    int thumbTop  = 0;  ///< Top coordinate of the thumb
    int thumbBot  = 0;  ///< Bottom coordinate of the thumb
};

/**
 * @brief Computes scrollbar thumb position and size.
 *
 * @param trackT      Top coordinate of the scrollbar track
 * @param trackB      Bottom coordinate of the scrollbar track
 * @param totalLines  Total number of content lines
 * @param linesVisible Number of lines visible in the viewport
 * @param scrollPx    Current scroll offset in pixels
 * @param maxScrollPx Maximum scroll offset in pixels
 * @param minThumbH   Minimum thumb height in pixels
 * @return Thumb geometry, or a zero-height thumb if content fits entirely.
 */
inline ScrollbarThumbMetrics ComputeScrollbarThumb(
    int trackT, int trackB,
    int totalLines, int linesVisible,
    int scrollPx, int maxScrollPx,
    int minThumbH = 10)
{
    if (totalLines <= linesVisible || maxScrollPx <= 0) {
        return {};
    }

    const int trackH = trackT - trackB;
    const float visibleRatio = static_cast<float>(linesVisible) / static_cast<float>(totalLines);
    int thumbH = static_cast<int>(static_cast<float>(trackH) * visibleRatio);
    thumbH = std::max(thumbH, minThumbH);
    thumbH = std::min(thumbH, trackH - 1);

    const float scrollRatio = static_cast<float>(scrollPx) / static_cast<float>(maxScrollPx);
    const int thumbTop = trackT - static_cast<int>(static_cast<float>(trackH - thumbH) * scrollRatio);
    const int thumbBot = thumbTop - thumbH;

    return {thumbH, thumbTop, thumbBot};
}
