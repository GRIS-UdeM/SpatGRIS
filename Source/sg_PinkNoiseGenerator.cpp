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

#include "sg_PinkNoiseGenerator.hpp"

#include "sg_Dbfs.hpp"
#include "sg_Narrow.hpp"

namespace
{
float pinkNoiseC0{};
float pinkNoiseC1{};
float pinkNoiseC2{};
float pinkNoiseC3{};
float pinkNoiseC4{};
float pinkNoiseC5{};
float pinkNoiseC6{};

} // namespace

namespace gris
{
//==============================================================================
void fillWithPinkNoise(float * const * samples, int const numSamples, int const numChannels, float const gain)
{
    static constexpr auto FAC{ 1.0f / (static_cast<float>(RAND_MAX) / 2.0f) };
    static constexpr dbfs_t CORRECTION_DB{ -18.2f };
    static int pulseCounter = 0;
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int PULSE_INTERVAL = SAMPLE_RATE; // 1 second
    static bool pulseOn = true;


    static auto const CORRECTION{ CORRECTION_DB.toGain() };

    for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex)
         {
           ++pulseCounter;
          if (pulseCounter >= PULSE_INTERVAL) {
        pulseOn = !pulseOn;
       pulseCounter = 0;
       } 
        auto const rnd{ narrow<float>(rand()) * FAC - 1.0f };
        pinkNoiseC0 = pinkNoiseC0 * 0.99886f + rnd * 0.0555179f;
        pinkNoiseC1 = pinkNoiseC1 * 0.99332f + rnd * 0.0750759f;
        pinkNoiseC2 = pinkNoiseC2 * 0.96900f + rnd * 0.1538520f;
        pinkNoiseC3 = pinkNoiseC3 * 0.86650f + rnd * 0.3104856f;
        pinkNoiseC4 = pinkNoiseC4 * 0.55000f + rnd * 0.5329522f;
        pinkNoiseC5 = pinkNoiseC5 * -0.7616f - rnd * 0.0168980f;
        auto sampleValue{ pinkNoiseC0 + pinkNoiseC1 + pinkNoiseC2 + pinkNoiseC3 + pinkNoiseC4 + pinkNoiseC5
                          + pinkNoiseC6 + rnd * 0.5362f };
        sampleValue *= gain * CORRECTION;
        pinkNoiseC6 = rnd * 0.115926f;

       if (pulseOn) {
         for (int channelIndex{}; channelIndex < numChannels; channelIndex++) {
             samples[channelIndex][sampleIndex] += sampleValue;
             }
          }

    }
}

} // namespace gris
