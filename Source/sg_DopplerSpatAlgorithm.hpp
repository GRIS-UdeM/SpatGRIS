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

#pragma once

#include "sg_SpatMode.hpp"

/** Experimental : a stereo reduction algorithm based on doppler-shifting. */
#if defined(USE_DOPPLER) || defined(DOXYGEN)

    #include "sg_AbstractSpatAlgorithm.hpp"
    #include "sg_Meters.hpp"

namespace gris
{
static constexpr meters_t FIELD_RADIUS{ 8.0f };
static constexpr meters_t HEAD_RADIUS{ 0.075f };

static constexpr CartesianVector LEFT_EAR_POSITION{ -HEAD_RADIUS / FIELD_RADIUS, 0.0f, 0.0f };
static constexpr CartesianVector RIGHT_EAR_POSITION{ HEAD_RADIUS / FIELD_RADIUS, 0.0f, 0.0f };
static constexpr std::array<CartesianVector, 2> EARS_POSITIONS{ LEFT_EAR_POSITION, RIGHT_EAR_POSITION };

static constexpr CartesianVector UPPER_LEFT_CORNER{ 1.0f, 1.0f, 1.0f };

static auto constexpr MAX_RELATIVE_DISTANCE{ (RIGHT_EAR_POSITION - UPPER_LEFT_CORNER).constexprLength() };
static auto constexpr MAX_DISTANCE{ FIELD_RADIUS * MAX_RELATIVE_DISTANCE };

static auto constexpr MAX_SAMPLE_RATE = 48000.0;
static auto constexpr MAX_BUFFER_SIZE = 4096;
static auto constexpr DOPPLER_BUFFER_SIZE
    = static_cast<int>(MAX_DISTANCE.get() * MAX_SAMPLE_RATE + narrow<double>(MAX_BUFFER_SIZE) + 1.0);

static constexpr auto SOUND_METERS_PER_SECOND = 400.0f;

using DopplerSpatData = std::array<float, 2>;
using DopplerSpatDataQueue = AtomicUpdater<DopplerSpatData>;

struct DopplerSourceData {
    DopplerSpatDataQueue spatDataQueue{};
    DopplerSpatDataQueue::Token * mostRecentSpatData{};
};

struct DopplerData {
    StrongArray<source_index_t, DopplerSourceData, MAX_NUM_SOURCES> sourcesData{};
    juce::AudioBuffer<float> dopplerLines{};
    StrongArray<source_index_t, DopplerSpatData, MAX_NUM_SOURCES> lastSpatData{};
    double sampleRate{};
};

//==============================================================================
class DopplerSpatAlgorithm final : public AbstractSpatAlgorithm
{
    using Interpolator = juce::Interpolators::Linear;

    DopplerData mData{};
    StrongArray<source_index_t, std::array<Interpolator, 2>, MAX_NUM_SOURCES> mInterpolators{};

public:
    //==============================================================================
    DopplerSpatAlgorithm(double sampleRate, int bufferSize);
    ~DopplerSpatAlgorithm() override = default;
    //==============================================================================
    DopplerSpatAlgorithm(DopplerSpatAlgorithm const &) = delete;
    DopplerSpatAlgorithm(DopplerSpatAlgorithm &&) = delete;
    DopplerSpatAlgorithm & operator=(DopplerSpatAlgorithm const &) = delete;
    DopplerSpatAlgorithm & operator=(DopplerSpatAlgorithm &&) = delete;
    //==============================================================================
    void updateSpatData(source_index_t sourceIndex, SourceData const & sourceData) noexcept override;
    [[nodiscard]] juce::Array<Triplet> getTriplets() const noexcept override;
    [[nodiscard]] bool hasTriplets() const noexcept override;
    void process(AudioConfig const & config,
                 SourceAudioBuffer & sourcesBuffer,
                 SpeakerAudioBuffer & speakersBuffer,
                 juce::AudioBuffer<float> & stereoBuffer,
                 SourcePeaks const & sourcePeaks,
                 SpeakersAudioConfig const * altSpeakerConfig) override;
    [[nodiscard]] tl::optional<Error> getError() const noexcept override { return tl::nullopt; }
    //==============================================================================
    static std::unique_ptr<AbstractSpatAlgorithm> make(double sampleRate, int bufferSize) noexcept;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(DopplerSpatAlgorithm)
};

} // namespace gris

#endif