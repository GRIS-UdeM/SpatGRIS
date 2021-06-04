/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "LogicStrucs.hpp"
#include "Triplet.hpp"

#define ASSERT_OSC_THREAD // jassert(juce::Thread::getCurrentThread()->getThreadName() == "OSC_SERVER")
#define ASSERT_AUDIO_THREAD //                                                                                           \
    jassert(juce::Thread::getCurrentThread()->getThreadName() != "OSC_SERVER"                                          \
            && !juce::MessageManager::getInstance()->isThisTheMessageThread())

//==============================================================================
class AbstractSpatAlgorithm
{
public:
    AbstractSpatAlgorithm() = default;
    virtual ~AbstractSpatAlgorithm() = default;
    //==============================================================================
    AbstractSpatAlgorithm(AbstractSpatAlgorithm const &) = delete;
    AbstractSpatAlgorithm(AbstractSpatAlgorithm &&) = delete;
    AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm const &) = delete;
    AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm &&) = delete;
    //==============================================================================
    void fixDirectOutsIntoPlace(SourcesData const & sources, SpeakerSetup const & speakerSetup) noexcept;
    //==============================================================================
    virtual void updateSpatData(source_index_t sourceIndex, SourceData const & sourceData) noexcept = 0;
    virtual void process(AudioConfig const & config,
                         SourceAudioBuffer & sourcesBuffer,
                         SpeakerAudioBuffer & speakersBuffer,
                         SourcePeaks const & sourcePeaks,
                         SpeakersAudioConfig const * altSpeakerConfig)
        = 0;
    [[nodiscard]] virtual juce::Array<Triplet> getTriplets() const noexcept = 0;
    [[nodiscard]] virtual bool hasTriplets() const noexcept = 0;
    //==============================================================================
    [[nodiscard]] static std::unique_ptr<AbstractSpatAlgorithm>
        make(SpeakerSetup const & speakerSetup, tl::optional<StereoMode> stereoMode, SourcesData const & sources);

private:
    JUCE_LEAK_DETECTOR(AbstractSpatAlgorithm)
};
