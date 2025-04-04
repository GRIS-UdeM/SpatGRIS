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

#include "AlgoGRIS/Data/StrongTypes/sg_CartesianVector.hpp"
#include "AlgoGRIS/Data/sg_LogicStrucs.hpp"
#include "AlgoGRIS/Data/sg_constants.hpp"
#include "sg_Warnings.hpp"

#include <JuceHeader.h>
#include <vector>
namespace gris
{
struct SpeakerData;
enum class SpatMode;
struct SourceData;

class MainContentComponent;
class SpeakerModel;

//==============================================================================
class SpeakerViewComponent final : public juce::HighResolutionTimer
{
private:
    //==============================================================================
    MainContentComponent & mMainContentComponent;
    juce::CriticalSection mLock{};
    ViewportData mData{};
    juce::Thread::ThreadID mHighResTimerThreadID;

    juce::DatagramSocket mUdpReceiverSocket;
    static constexpr int mMaxBufferSize = 1024;

    std::string mJsonSources;
    std::string mJsonSpeakers;
    std::unique_ptr<juce::DynamicObject> mJsonSGInfos;

    bool mKillSpeakerViewProcess{};

public:
    //==============================================================================
    static constexpr auto SPHERE_RADIUS = 0.03f;
    static constexpr auto HALF_SPHERE_RADIUS = SPHERE_RADIUS / 2.0f;
    //==============================================================================
    explicit SpeakerViewComponent(MainContentComponent & mainContentComponent);

    ~SpeakerViewComponent() override;

    SG_DELETE_COPY_AND_MOVE(SpeakerViewComponent)
    //==============================================================================
    auto & getData() noexcept { return mData; }
    auto const & getData() const noexcept { return mData; }

    void startSpeakerViewNetworking();
    void stopSpeakerViewNetworking();
    bool isSpeakerViewNetworkingRunning();

    Position getCameraPosition() const noexcept;

    void setConfig(ViewportConfig const & config, SourcesData const & sources);
    void setCameraPosition(CartesianVector const & position) noexcept;
    void setTriplets(juce::Array<Triplet> triplets) noexcept;

    void shouldKillSpeakerViewProcess(bool shouldKill);

    auto const & getLock() const noexcept { return mLock; }
    //==============================================================================
    void hiResTimerCallback() override;

private:
    //==============================================================================
    void prepareSGInfos();
    bool isHiResTimerThread();
    void listenUDP();
    void sendUDP();
    void emptyUDPReceiverBuffer();

    void writeSourcesJson();
    void writeSpeakersJson();

    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerViewComponent)
}; // class SpeakerViewComponent

} // namespace gris
