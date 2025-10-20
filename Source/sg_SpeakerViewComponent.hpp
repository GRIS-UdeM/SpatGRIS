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

#include "Data/StrongTypes/sg_CartesianVector.hpp"
#include "Data/sg_LogicStrucs.hpp"
#include "Data/sg_constants.hpp"
#include "Data/sg_SpatMode.hpp"
#include "sg_Warnings.hpp"

#include <JuceHeader.h>
#include <vector>
namespace gris
{
struct SpeakerData;
struct SourceData;

class MainContentComponent;
class SpeakerModel;

//==============================================================================
/**
 * @brief Manages network interaction with the SpeakerView process.
 *
 * The communication is based on JSON over raw UDP sockets.
 * The protocol is documented in [doc/SpeakerView.md](SpeakerView.md) at the root of the repository.
 */
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

    // We use the json strings to see if the data has changed.
    // It would maybe be a bit more efficient to avoid playing with
    // strings unless we are sure that the data has changed.
    std::string mOldJsonSGInfos = "nothing";
    std::string mOldJsonSpeakers = "nothing";
    std::string mOldJsonSources = "nothing";

    std::string mJsonSources;
    std::string mJsonSpeakers;
    std::string mJsonSGInfos;

    juce::DatagramSocket udpSenderSocket;
    /**
     * This socket is optionaly used to send udp data to a standalone SpeakerView instance
     * (potentially on another computer).
     */
    std::unique_ptr<juce::DatagramSocket> extraUdpSenderSocket;
    /**
     * This socket is optionaly used to receive udp data from a standalone SpeakerView instanc
     */
    std::unique_ptr<juce::DatagramSocket> extraUdpReceiverSocket;

    bool mKillSpeakerViewProcess{};

    uint32_t mTicksSinceKeepalive{};

public:
    //==============================================================================
    static constexpr auto SPHERE_RADIUS = 0.03f;
    static constexpr auto HALF_SPHERE_RADIUS = SPHERE_RADIUS / 2.0f;
    static inline const juce::String localhost{"127.0.0.1"};


/**
 * @brief This macro creates a `juce::Identifier` variable with the same name as its string content,
 * reducing boilerplate and ensuring consistency between the variable name and its value.
 */
#define MAKE_IDENTIFIER(name) static inline const juce::Identifier name{ #name };
    MAKE_IDENTIFIER(selSpkNum)
    MAKE_IDENTIFIER(keepSVTop)
    MAKE_IDENTIFIER(showHall)
    MAKE_IDENTIFIER(showSrcNum)
    MAKE_IDENTIFIER(showSpkNum)
    MAKE_IDENTIFIER(showSpks)
    MAKE_IDENTIFIER(showSpkTriplets)
    MAKE_IDENTIFIER(showSrcActivity)
    MAKE_IDENTIFIER(showSpkLevel)
    MAKE_IDENTIFIER(showSphereCube)
    MAKE_IDENTIFIER(resetSrcPos)
    MAKE_IDENTIFIER(genMute)
    MAKE_IDENTIFIER(winPos)
    MAKE_IDENTIFIER(winSize)
    MAKE_IDENTIFIER(camPos)
    MAKE_IDENTIFIER(quitting)
#undef MAKE_IDENTIFIER

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

    tl::optional<int> getExtraUDPInputPort() const;
    tl::optional<int> getExtraUDPOutputPort() const;
    tl::optional<juce::String> getExtraUDPOutputAddress() const;


    /**
     * Manages all the extra ports if you pass it the data contained in the project data.
     */
    void initExtraPorts(tl::optional<int> uDPExtraInputPort, tl::optional<int> uDPExtraOutputPort, tl::optional<juce::String> uDPExtraOutputAddress);

    void disableExtraUDPOutput();
    void disableExtraUDPInput();
    /**
     * Tries to set the extra udp input port to the given port. Reverts to the old port and show a warning
     * if it fails.
     */
    bool setExtraUDPInputPort(int const port);

    void setExtraUDPOutput(int const port, const juce::StringRef address);

    int mUDPDefaultOutputPort;
    juce::String mUDPDefaultOutputAddress;

private:
    //==============================================================================
    void prepareSourcesJson();
    void prepareSpeakersJson();
    void prepareSGInfos();
    bool isHiResTimerThread();
    void listenUDP(juce::DatagramSocket & socket);
    void sendUDP(const std::string & content);
    void sendSpeakersUDP();
    void sendSourcesUDP();
    void sendSpatGRISUDP();
    void emptyUDPReceiverBuffer();

    tl::optional<int> mUDPExtraOutputPort;
    tl::optional<juce::String> mUDPExtraOutputAddress;

    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerViewComponent)
}; // class SpeakerViewComponent

} // namespace gris
