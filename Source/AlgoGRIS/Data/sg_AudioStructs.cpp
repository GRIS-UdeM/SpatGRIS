/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sg_AudioStructs.hpp"

namespace gris
{
//==============================================================================
void SpeakerHighpassConfig::process(float * data,
                                    int const numSamples,
                                    ColdSpeakerHighpass & state,
                                    juce::Random & randNoise) const
{
    for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
        auto const sample{ static_cast<double>(data[sampleIndex])
                           + (randNoise.nextDouble() * 2.0 - 1.0) * DENORM_GAIN };
        auto const val{ ha0 * sample + ha1 * state.x1 + ha2 * state.x2 + ha1 * state.x3 + ha0 * state.x4 - b1 * state.y1
                        - b2 * state.y2 - b3 * state.y3 - b4 * state.y4 };
        state.y4 = state.y3;
        state.y3 = state.y2;
        state.y2 = state.y1;
        state.y1 = val;
        state.x4 = state.x3;
        state.x3 = state.x2;
        state.x2 = state.x1;
        state.x1 = sample;
        data[sampleIndex] = static_cast<float>(val);

        jassert(std::isfinite(state.x1));
        jassert(std::isfinite(state.x2));
        jassert(std::isfinite(state.x3));
        jassert(std::isfinite(state.x4));
        jassert(std::isfinite(state.y1));
        jassert(std::isfinite(state.y2));
        jassert(std::isfinite(state.y3));
        jassert(std::isfinite(state.y4));
        jassert(std::isfinite(data[sampleIndex]));
    }
}

//==============================================================================
void MbapAttenuationConfig::process(float * data,
                                    int const numSamples,
                                    float const distance,
                                    MbapSourceAttenuationState & state) const
{
    static constexpr auto NORMAL_DISTANCE = 1.0f;
    static constexpr auto EXTRA_DISTANCE = MBAP_EXTENDED_RADIUS - NORMAL_DISTANCE;

    auto const attenuationRatio{ std::clamp((distance - NORMAL_DISTANCE) / EXTRA_DISTANCE, 0.0f, 1.0f) };
    auto const targetGain{ 1.0f - attenuationRatio * (1.0f - linearGain) };
    auto const targetCoefficient{ attenuationRatio * lowpassCoefficient };

    auto const gainStep{ (targetGain - state.currentGain) / narrow<float>(numSamples) };
    auto const coefficientStep{ (targetCoefficient - state.currentCoefficient) / narrow<float>(numSamples) };

    jassert(std::isfinite(targetGain));
    jassert(std::isfinite(targetCoefficient));
    jassert(std::isfinite(gainStep));
    jassert(std::isfinite(coefficientStep));
    jassert(std::isfinite(state.lowpassY));
    jassert(std::isfinite(state.lowpassZ));
    jassert(std::isfinite(state.currentCoefficient));
    jassert(std::isfinite(state.currentGain));

    if (coefficientStep == 0.0f && gainStep == 0.0f) {
        if (attenuationRatio == 0.0f) {
            return;
        }

        // no ramp in coefficient and gain
        for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
            state.lowpassY = data[sampleIndex] + (state.lowpassY - data[sampleIndex]) * state.currentCoefficient;
            state.lowpassZ = state.lowpassY + (state.lowpassZ - state.lowpassY) * state.currentCoefficient;
            data[sampleIndex] = state.lowpassZ * state.currentGain;

            jassert(std::isfinite(state.lowpassY));
            jassert(std::isfinite(state.lowpassZ));
        }

        return;
    }

    // coefficient and/or gain ramp
    for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
        state.currentCoefficient += coefficientStep;
        state.currentGain += gainStep;
        state.lowpassY = data[sampleIndex] + (state.lowpassY - data[sampleIndex]) * state.currentCoefficient;
        state.lowpassZ = state.lowpassY + (state.lowpassZ - state.lowpassY) * state.currentCoefficient;
        data[sampleIndex] = state.lowpassZ * state.currentGain;

        jassert(std::isfinite(state.currentCoefficient));
        jassert(std::isfinite(state.currentGain));
        jassert(std::isfinite(state.lowpassY));
        jassert(std::isfinite(state.lowpassZ));
    }
}

//==============================================================================
void ColdSpeakerHighpass::resetValues()
{
    x1 = 0.0;
    x2 = 0.0;
    x3 = 0.0;
    x4 = 0.0;
    y1 = 0.0;
    y2 = 0.0;
    y3 = 0.0;
    y4 = 0.0;
}

} // namespace gris
