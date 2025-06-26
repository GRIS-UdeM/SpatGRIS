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

#include "sg_SpeakerViewComponent.hpp"

#include "sg_MainComponent.hpp"

#include <algorithm>

#include <charconv>

namespace gris
{
static void appendNumber(std::string & str, float val)
{
    // otherwise we get some 4.37e-38 ...
    if (std::abs(val) < 1e-6f)
        val = 0.f;

    // We are not using to_chars directly as it may not be available
    // on every platform (e.g. older macOS).
    // See notes here: https://en.cppreference.com/w/cpp/utility/to_chars
#if __cpp_lib_to_chars >= 201611L
    static constexpr auto precision = 3;
    static constexpr auto format = std::chars_format::general;
    char buf[16];
    auto res = std::to_chars(buf, buf + 16, val, format, precision);
    jassert(res.ec == std::errc{});
    str.append(buf, res.ptr);
#else
    str += std::to_string(val);
#endif
}

static void appendNumber(std::string & str, int val)
{
#if __cpp_lib_to_chars >= 201611L
    char buf[16];
    auto res = std::to_chars(buf, buf + 16, val);
    jassert(res.ec == std::errc{});
    str.append(buf, res.ptr);
#else
    str += std::to_string(val);
#endif
}


//==============================================================================
SpeakerViewComponent::SpeakerViewComponent(MainContentComponent & mainContentComponent)
    : mMainContentComponent(mainContentComponent),
      mUDPOutputAddress(localhost),
      // We use the DEFAULT_UDP_INPUT_PORT for the output socket because the naming was
      // inverted at some point. We should fix this inversion when we migrate these constants
      // from algoGRIS into spatGRIS
      mUDPOutputPort(DEFAULT_UDP_INPUT_PORT)
{
    mUdpReceiverSocket = std::make_unique<juce::DatagramSocket>();
    // Same naming inversion as explained by the comment above.
    mUdpReceiverSocket->bindToPort(DEFAULT_UDP_OUTPUT_PORT);
}

//==============================================================================
SpeakerViewComponent::~SpeakerViewComponent()
{
    stopTimer();
}

bool SpeakerViewComponent::setUDPInputPort(int const port)
{
  int oldPort = getUDPInputPort();
  juce::ScopedLock const lock{ mLock };
  // Apparently, calling bindToPort when a socket is already bound results in
  // failure every time so we reconstruct the socket.
  mUdpReceiverSocket = std::make_unique<juce::DatagramSocket>();
  bool success = mUdpReceiverSocket->bindToPort(port, mUDPOutputAddress);
  if (!success) {
    mUdpReceiverSocket = std::make_unique<juce::DatagramSocket>();
    mUdpReceiverSocket->bindToPort(oldPort, mUDPOutputAddress);
  }
  return success;
}

int SpeakerViewComponent::getUDPInputPort() const
{
  return mUdpReceiverSocket->getBoundPort();
}

//==============================================================================
void SpeakerViewComponent::startSpeakerViewNetworking()
{
    JUCE_ASSERT_MESSAGE_THREAD
    juce::ScopedLock const lock{ mLock };

    startTimer(40);
}

//==============================================================================
void SpeakerViewComponent::stopSpeakerViewNetworking()
{
    JUCE_ASSERT_MESSAGE_THREAD
    juce::ScopedLock const lock{ mLock };

    stopTimer();
    emptyUDPReceiverBuffer();
}

//==============================================================================
bool SpeakerViewComponent::isSpeakerViewNetworkingRunning()
{
    JUCE_ASSERT_MESSAGE_THREAD
    juce::ScopedLock const lock{ mLock };

    return isTimerRunning();
}

//==============================================================================
Position SpeakerViewComponent::getCameraPosition() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD
    juce::ScopedLock const lock{ mLock };

    return mData.coldData.cameraPosition;
}

//==============================================================================
void SpeakerViewComponent::setConfig(ViewportConfig const & config, SourcesData const & sources)
{
    JUCE_ASSERT_MESSAGE_THREAD
    juce::ScopedLock const lock{ mLock };

    mData.warmData = config;
    mData.hotSourcesDataUpdaters.clear();
    for (auto const & source : sources) {
        mData.hotSourcesDataUpdaters.add(source.key);
    }
}

//==============================================================================
void SpeakerViewComponent::setCameraPosition(CartesianVector const & position) noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD
    juce::ScopedLock const lock{ mLock };

    mData.coldData.cameraPosition = PolarVector{ position };
}

//==============================================================================
void SpeakerViewComponent::setTriplets(juce::Array<Triplet> triplets) noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD
    juce::ScopedLock const lock{ mLock };
    mData.coldData.triplets = std::move(triplets);
}

//==============================================================================
void SpeakerViewComponent::shouldKillSpeakerViewProcess(bool shouldKill)
{
    JUCE_ASSERT_MESSAGE_THREAD
    juce::ScopedLock const lock{ mLock };

    mKillSpeakerViewProcess = shouldKill;

    if (shouldKill) {
        // asking SpeakView to quit itself. SpeakView will send stop message before closing
        prepareSGInfos();
        sendUDP();
    }
}

//==============================================================================
void SpeakerViewComponent::prepareSourcesJson()
{
    mJsonSources.clear();
    mJsonSources.reserve(4096);
    mJsonSources += "[\"sources\",";

    int processedSources = 0;
    for (auto & source : mData.hotSourcesDataUpdaters) {
        auto & exchanger{ source.value };
        auto *& ticket{ mData.coldData.mostRecentSourcesData[source.key] };
        exchanger.getMostRecent(ticket);
        if (ticket == nullptr) {
            continue;
        }
        auto const & sourceData{ ticket->get() };
        if (!sourceData) {
            continue;
        }
        /* Order is :
            srcNum
            pos[x, y, z]  (note: SG is XZ-Y, Godot is XYZ. Conversion happens in SpeakerView)
            color[r, g, b, a]
            hybridSpatMode
            azimuth
            elevation
        */
        auto const & pos{ sourceData->position.getCartesian() };
        auto const & color{ sourceData->colour };
        mJsonSources += "[";
        appendNumber(mJsonSources, source.key.get());
        mJsonSources += ",[";
        appendNumber(mJsonSources, pos.x);
        mJsonSources += ",";
        appendNumber(mJsonSources, pos.y);
        mJsonSources += ",";
        appendNumber(mJsonSources, pos.z);
        mJsonSources += "],[";
        appendNumber(mJsonSources, color.getFloatRed());
        mJsonSources += ",";
        appendNumber(mJsonSources, color.getFloatGreen());
        mJsonSources += ",";
        appendNumber(mJsonSources, color.getFloatBlue());
        mJsonSources += ",";
        appendNumber(mJsonSources, color.getFloatAlpha());
        mJsonSources += "],";
        appendNumber(mJsonSources, static_cast<int>(sourceData->hybridSpatMode));
        mJsonSources += ",";
        appendNumber(mJsonSources, sourceData->azimuthSpan);
        mJsonSources += ",";
        appendNumber(mJsonSources, sourceData->zenithSpan);
        mJsonSources += "],";

        processedSources++;
    }
    if (processedSources > 0)
        mJsonSources.pop_back(); // Remove the last ,
    mJsonSources += "]";
}

//==============================================================================
void SpeakerViewComponent::prepareSpeakersJson()
{
    auto const & viewSettings{ mData.warmData.viewSettings };

    int processedSpeakers = 0;
    mJsonSpeakers.clear();
    mJsonSpeakers.reserve(4096);
    mJsonSpeakers += "[\"speakers\",";
    auto speaker_centers = mMainContentComponent.getSpeakersGroupCenters();
    if (viewSettings.showSpeakers) {
        for (auto const & speaker : mData.warmData.speakers) {
            static constexpr auto DEFAULT_ALPHA = 0.75f;

            auto const & showSpeakerLevels{ mData.warmData.viewSettings.showSpeakerLevels };
            auto const getAlpha = [&]() {
                if (!showSpeakerLevels) {
                    return DEFAULT_ALPHA;
                }
                auto & exchanger{ mData.hotSpeakersAlphaUpdaters[speaker.key] };
                auto *& ticket{ mData.coldData.mostRecentSpeakersAlpha[speaker.key] };
                exchanger.getMostRecent(ticket);
                if (ticket == nullptr) {
                    return DEFAULT_ALPHA;
                }
                return ticket->get();
            };
            /* Order is :
                spkNum
                pos
                isSelected
                isDirectOutOnly
                alpha
                (group center position (if speaker is in a group))
            */

            auto const & pos{ speaker.value.position.getCartesian() };

            mJsonSpeakers += "[";
            appendNumber(mJsonSpeakers, speaker.key.get());
            mJsonSpeakers += ",[";
            appendNumber(mJsonSpeakers, pos.x);
            mJsonSpeakers += ",";
            appendNumber(mJsonSpeakers, pos.y);
            mJsonSpeakers += ",";
            appendNumber(mJsonSpeakers, pos.z);
            mJsonSpeakers += "],";
            mJsonSpeakers += speaker.value.isSelected ? "1" : "0";
            mJsonSpeakers += ",";
            mJsonSpeakers += speaker.value.isDirectOutOnly ? "1" : "0";
            mJsonSpeakers += ",";
            appendNumber(mJsonSpeakers, getAlpha());
            // if the speaker is in a group, add its center's position.

            // This will insert a default tl::nullopt if the key is not in
            // the map. In this case we can live with it.
            auto center_position = speaker_centers[speaker.key];

            if (center_position) {
              auto const & center_cartesion_pos = center_position->getCartesian();
              mJsonSpeakers += ",[";
              appendNumber(mJsonSpeakers, center_cartesion_pos.x);
              mJsonSpeakers += ",";
              appendNumber(mJsonSpeakers, center_cartesion_pos.y);
              mJsonSpeakers += ",";
              appendNumber(mJsonSpeakers, center_cartesion_pos.z);
              mJsonSpeakers += "]";
            }
            mJsonSpeakers += "],";

            processedSpeakers++;
        }
    }
    if (processedSpeakers > 0)
        mJsonSpeakers.pop_back(); // Remove the last ,
    mJsonSpeakers += "]";
}

void SpeakerViewComponent::hiResTimerCallback()
{
    mHighResTimerThreadID = juce::Thread::getCurrentThreadId();

    juce::ScopedLock const lock{ mLock };

    listenUDP();

    prepareSourcesJson();
    prepareSpeakersJson();
    prepareSGInfos();

    sendUDP();
}

//==============================================================================
void SpeakerViewComponent::prepareSGInfos()
{
    auto * topLevelComp = mMainContentComponent.getTopLevelComponent();
    auto const & spatMode{ static_cast<int>(mData.warmData.spatMode) };
    auto const & viewSettings{ mData.warmData.viewSettings };
    auto appendProperty = [&str=mJsonSGInfos] <typename P> (std::string_view name, const P& prop) {
      str += '"';
      str += name;
      str += "\":";
      if constexpr(std::is_same_v<bool, P>) {
        str += prop ? "true" : "false";
      }
      else if constexpr(std::is_same_v<juce::String, P>) {
        str += '"';
        str += prop.toStdString();
        str += '"';
      }
      else if constexpr(std::is_arithmetic_v<P>){
        appendNumber(str, prop);
      } else {
        static_assert(P::is_not_a_known_type);
      }
      str += ",";
    };

    mJsonSGInfos.clear();
    mJsonSGInfos.reserve(4096);
    mJsonSGInfos += "{";

    appendProperty("killSV", mKillSpeakerViewProcess);
    appendProperty("spkStpName", mData.warmData.title);
    appendProperty("SGHasFocus", topLevelComp->hasKeyboardFocus(true));
    appendProperty("KeepSVOnTop", viewSettings.keepSpeakerViewWindowOnTop);
    appendProperty("SVGrabFocus", mMainContentComponent.speakerViewShouldGrabFocus());
    appendProperty("showHall", viewSettings.showHall);
    appendProperty("spatMode", spatMode); // -1, 0, 1, 2
    appendProperty("showSourceNumber", viewSettings.showSourceNumbers);
    appendProperty("showSpeakerNumber", viewSettings.showSpeakerNumbers);
    appendProperty("showSpeakers", viewSettings.showSpeakers);
    appendProperty("showSpeakerTriplets", viewSettings.showSpeakerTriplets);
    appendProperty("showSourceActivity", viewSettings.showSourceActivity);
    appendProperty("showSpeakerLevel", viewSettings.showSpeakerLevels);
    appendProperty("showSphereOrCube", viewSettings.showSphereOrCube);
    appendProperty("genMute", mMainContentComponent.getData().speakerSetup.generalMute);

    mJsonSGInfos += "\"spkTriplets\":[";

    if(!mData.coldData.triplets.isEmpty()) {
        for (auto const & triplet : mData.coldData.triplets) {
            mJsonSGInfos += '[';
            appendNumber(mJsonSGInfos, triplet.id1.get());
            mJsonSGInfos += ',';
            appendNumber(mJsonSGInfos, triplet.id2.get());
            mJsonSGInfos += ',';
            appendNumber(mJsonSGInfos, triplet.id3.get());
            mJsonSGInfos += "],";
        }
        mJsonSGInfos.pop_back(); // Remove the last ,
    }
    mJsonSGInfos += "]";

    mJsonSGInfos += "}";
    if (mMainContentComponent.speakerViewShouldGrabFocus()) {
        juce::MessageManager::callAsync([this] { mMainContentComponent.resetSpeakerViewShouldGrabFocus(); });
    }
}

//==============================================================================
bool SpeakerViewComponent::isHiResTimerThread()
{
    auto currentThreadID = juce::Thread::getCurrentThreadId();
    return mHighResTimerThreadID == currentThreadID;
}

//==============================================================================
void SpeakerViewComponent::listenUDP()
{
    if (!isHiResTimerThread()) {
        return;
    }

    juce::String senderAddress;
    int senderPort;
    char receiveBuffer[mMaxBufferSize];
    auto packetSize = mUdpReceiverSocket->read(receiveBuffer, mMaxBufferSize, false, senderAddress, senderPort);

    if (packetSize > 0) {
        juce::String receivedData(receiveBuffer, packetSize);
        juce::var jsonResult;
        auto res = juce::JSON::parse(receivedData, jsonResult);

        if (res == juce::Result::ok()) {
            if (jsonResult.isObject()) {
                const auto keys = jsonResult.getDynamicObject()->getProperties();

                for (int i{}; i < keys.size(); ++i) {
                    const auto property = keys.getName(i).toString();
                    juce::var value = keys.getValueAt(i);

                    if (property == "selSpkNum") {
                        juce::String selSpkNumValues = value;
                        auto spkIsSelectedWithMouseStr = selSpkNumValues.fromLastOccurrenceOf(",", false, true);
                        selSpkNumValues = selSpkNumValues.dropLastCharacters(spkIsSelectedWithMouseStr.length() + 1);
                        auto selectedSpkNumStr = selSpkNumValues.fromLastOccurrenceOf(",", false, true);
                        const auto selectedSpkNum = selectedSpkNumStr.getIntValue();

                        if (spkIsSelectedWithMouseStr.compare("true") == 0) {
                            tl::optional<output_patch_t> iBestSpeaker{};
                            iBestSpeaker = static_cast<output_patch_t>(selectedSpkNum);
                            juce::MessageManager::callAsync([this, speaker = *iBestSpeaker] {
                                mMainContentComponent.setSelectedSpeakers(juce::Array<output_patch_t>{ speaker });
                            });
                        }
                    } else if (property == "keepSVTop") {
                        auto keepSVOnTopValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, keepSVOnTopValue] {
                            mMainContentComponent.handleKeepSVOnTopFromSpeakerView(keepSVOnTopValue);
                        });
                    } else if (property == "showHall") {
                        auto showHallValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showHallValue] {
                            mMainContentComponent.handleShowHallFromSpeakerView(showHallValue);
                        });
                    } else if (property == "showSrcNum") {
                        auto showSrcValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSrcValue] {
                            mMainContentComponent.handleShowSourceNumbersFromSpeakerView(showSrcValue);
                        });
                    } else if (property == "showSpkNum") {
                        auto showSpkValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSpkValue] {
                            mMainContentComponent.handleShowSpeakerNumbersFromSpeakerView(showSpkValue);
                        });
                    } else if (property == "showSpks") {
                        auto showSpksValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSpksValue] {
                            mMainContentComponent.handleShowSpeakersFromSpeakerView(showSpksValue);
                        });
                    } else if (property == "showSpkTriplets") {
                        auto showSpksTripletsValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSpksTripletsValue] {
                            mMainContentComponent.handleShowSpeakerTripletsFromSpeakerView(showSpksTripletsValue);
                        });
                    } else if (property == "showSrcActivity") {
                        auto showSrcActivityValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSrcActivityValue] {
                            mMainContentComponent.handleShowSourceActivityFromSpeakerView(showSrcActivityValue);
                        });
                    } else if (property == "showSpkLevel") {
                        auto showSpkLevelValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSpkLevelValue] {
                            mMainContentComponent.handleShowSpeakerLevelFromSpeakerView(showSpkLevelValue);
                        });
                    } else if (property == "showSphereCube") {
                        auto showSphereOrCubeValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSphereOrCubeValue] {
                            mMainContentComponent.handleShowSphereOrCubeFromSpeakerView(showSphereOrCubeValue);
                        });
                    } else if (property == "resetSrcPos") {
                        if (static_cast<int>(value) != 0) {
                            juce::MessageManager::callAsync(
                                [this] { mMainContentComponent.handleResetSourcesPositionsFromSpeakerView(); });
                        }
                    } else if (property == "genMute") {
                        auto generalMute = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, generalMute] {
                            mMainContentComponent.handleGeneralMuteFromSpeakerView(generalMute);
                        });
                    } else if (property == "winPos") {
                        auto winPosValue = value;
                        juce::MessageManager::callAsync([this, winPosValue] {
                            mMainContentComponent.handleWindowPositionFromSpeakerView(winPosValue);
                        });
                    } else if (property == "winSize") {
                        auto winSizeValue = value;
                        juce::MessageManager::callAsync([this, winSizeValue] {
                            mMainContentComponent.handleWindowSizeFromSpeakerView(winSizeValue);
                        });
                    } else if (property == "camPos") {
                        auto camPosValue = value;
                        juce::MessageManager::callAsync([this, camPosValue] {
                            mMainContentComponent.handleCameraPositionFromSpeakerView(camPosValue);
                        });
                    } else if (property == "quitting") {
                        bool quittingValue = value;
                        if (quittingValue) {
                            stopTimer();
                            emptyUDPReceiverBuffer();
                        }
                    }
                }
            }
        }
    }
}

//==============================================================================
void SpeakerViewComponent::sendUDP()
{
    {
        [[maybe_unused]] int numBytesWrittenSources = udpSenderSocket.write(mUDPOutputAddress,
                                                                            mUDPOutputPort,
                                                                            mJsonSources.c_str(),
                                                                            static_cast<int>(mJsonSources.size()));
        jassert(!(numBytesWrittenSources < 0));
    }
{
        [[maybe_unused]] int numBytesWrittenSpeakers = udpSenderSocket.write(mUDPOutputAddress,
                                                                             mUDPOutputPort,
                                                                             mJsonSpeakers.c_str(),
                                                                             static_cast<int>(mJsonSpeakers.size()));
        jassert(!(numBytesWrittenSpeakers < 0));
    }

    if (!mJsonSGInfos.empty()) {{
            [[maybe_unused]] int numBytesWrittenSGInfos
                = udpSenderSocket.write(mUDPOutputAddress,
                                        mUDPOutputPort,
                                        mJsonSGInfos.c_str(),
                                        static_cast<int>(mJsonSGInfos.size()));
            jassert(!(numBytesWrittenSGInfos < 0));
        }
    }
}

//==============================================================================
void SpeakerViewComponent::emptyUDPReceiverBuffer()
{
    juce::String senderAddress;
    int senderPort;
    char receiveBuffer[mMaxBufferSize];
    [[maybe_unused]] auto packetSize
        = mUdpReceiverSocket->read(receiveBuffer, mMaxBufferSize, false, senderAddress, senderPort);
}

} // namespace gris
