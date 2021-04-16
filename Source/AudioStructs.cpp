#include "AudioStructs.hpp"

#pragma once

//==============================================================================
void SpeakerHighpassConfig::process(float * data, int const numSamples, SpeakerHighpassState & state) const
{
    for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
        auto const inval{ static_cast<double>(data[sampleIndex]) };
        auto const val{ ha0 * inval + ha1 * state.x1 + ha2 * state.x2 + ha1 * state.x3 + ha0 * state.x4 - b1 * state.y1
                        - b2 * state.y2 - b3 * state.y3 - b4 * state.y4 };
        state.y4 = state.y3;
        state.y3 = state.y2;
        state.y2 = state.y1;
        state.y1 = val;
        state.x4 = state.x3;
        state.x3 = state.x2;
        state.x2 = state.x1;
        state.x1 = inval;
        data[sampleIndex] = static_cast<float>(val);
    }
}

//==============================================================================
void LbapAttenuationConfig::process(float * data,
                                    int const numSamples,
                                    float const distance,
                                    LbapSourceAttenuationState & state) const
{
    auto const distanceGain{ (1.0f - distance) * (1.0f - linearGain) + linearGain };
    auto const distanceCoefficient{ distance * lowpassCoefficient };
    auto const diffGain{ (distanceGain - state.lastGain) / narrow<float>(numSamples) };
    auto const diffCoefficient = (distanceCoefficient - state.lastCoefficient) / narrow<float>(numSamples);
    auto filterInY{ state.lowpassY };
    auto filterInZ{ state.lowpassZ };
    auto lastCoefficient{ state.lastCoefficient };
    auto lastGain{ state.lastGain };
    // TODO : this could be greatly optimized
    if (diffCoefficient == 0.0f && diffGain == 0.0f) {
        // simplified version
        for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
            filterInY = data[sampleIndex] + (filterInY - data[sampleIndex]) * lastCoefficient;
            filterInZ = filterInY + (filterInZ - filterInY) * lastCoefficient;
            data[sampleIndex] = filterInZ * lastGain;
        }
    } else {
        // full version
        for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
            lastCoefficient += diffCoefficient;
            lastGain += diffGain;
            filterInY = data[sampleIndex] + (filterInY - data[sampleIndex]) * lastCoefficient;
            filterInZ = filterInY + (filterInZ - filterInY) * lastCoefficient;
            data[sampleIndex] = filterInZ * lastGain;
        }
    }
    state.lowpassY = filterInY;
    state.lowpassZ = filterInZ;
    state.lastGain = distanceGain;
    state.lastCoefficient = distanceCoefficient;
}
