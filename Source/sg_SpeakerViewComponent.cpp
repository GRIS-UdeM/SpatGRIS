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

namespace gris
{
//==============================================================================
SpeakerViewComponent::SpeakerViewComponent(MainContentComponent & mainContentComponent)
    : mMainContentComponent(mainContentComponent)
{
    mUdpReceiverSocket.bindToPort(DEFAULT_UDP_OUTPUT_PORT, "127.0.0.1");
}

//==============================================================================
SpeakerViewComponent::~SpeakerViewComponent()
{
    stopTimer();
    mUdpReceiverSocket.shutdown();
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
void SpeakerViewComponent::hiResTimerCallback()
{
    mHighResTimerThreadID = juce::Thread::getCurrentThreadId();

    juce::ScopedLock const lock{ mLock };

    listenUDP();

    auto const & viewSettings{ mData.warmData.viewSettings };

    // Prepare json sources data
    mJsonSources.clear();
    mJsonSources.add(juce::var("sources"));

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
            pos[x, y, z]
            color[r, g, b, a]
            hybridSpatMode
            azimuth
            elevation
        */
        auto const & pos{ sourceData->position.getCartesian() };
        auto const & color{ sourceData->colour };
        auto const & hybridSpatMode{ static_cast<int>(sourceData->hybridSpatMode) };
        auto const & azimuth{ sourceData->azimuthSpan };
        auto const & elevation{ sourceData->zenithSpan };

        juce::Array<juce::var> jsonSource;
        jsonSource.add(juce::var(source.key.get())); // srcNum
        // SG is XZ-Y, Godot is XYZ. Conversion happens in SpeakerView
        jsonSource.add(juce::var(juce::Array<juce::var>({ pos.x, pos.y, pos.z }))); // pos
        jsonSource.add(juce::var(juce::Array<juce::var>(color.getFloatRed(),
                                                        color.getFloatGreen(),
                                                        color.getFloatBlue(),
                                                        color.getFloatAlpha()))); // color
        jsonSource.add(juce::var(hybridSpatMode)); // 0=Dome ou 1=Cube              // hybridSpatMode
        jsonSource.add(juce::var(azimuth));        // azimuth
        jsonSource.add(juce::var(elevation));      // elevation
        mJsonSources.add(jsonSource);
    }

    // Prepare json speakers data
    mJsonSpeakers.clear();
    mJsonSpeakers.add(juce::var("speakers"));
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
            */

            auto const & pos{ speaker.value.position.getCartesian() };
            auto const & isSelected{ speaker.value.isSelected };
            auto const & isDirectOutOnly{ speaker.value.isDirectOutOnly };
            auto const & alpha{ getAlpha() };

            juce::Array<juce::var> jsonSpeaker;
            jsonSpeaker.add(juce::var(speaker.key.get()));                               // spkNum
            jsonSpeaker.add(juce::var(juce::Array<juce::var>({ pos.x, pos.y, pos.z }))); // pos
            jsonSpeaker.add(juce::var(isSelected));                                      // isSelected
            jsonSpeaker.add(juce::var(isDirectOutOnly));                                 // isDirectOutOnly
            jsonSpeaker.add(juce::var(alpha));                                           // alpha
            mJsonSpeakers.add(jsonSpeaker);
        }
    }

    // Prepare json SpatGris infos
    prepareSGInfos();

    sendUDP();
}

//==============================================================================
void SpeakerViewComponent::prepareSGInfos()
{
    auto * topLevelComp = mMainContentComponent.getTopLevelComponent();
    auto const & spatMode{ static_cast<int>(mData.warmData.spatMode) };
    auto const & viewSettings{ mData.warmData.viewSettings };
    mJsonSGInfos.reset(new juce::DynamicObject());

    mJsonSGInfos->setProperty("killSV", mKillSpeakerViewProcess);
    mJsonSGInfos->setProperty("spkStpName", mData.warmData.title);
    mJsonSGInfos->setProperty("SGHasFocus", topLevelComp->hasKeyboardFocus(true));
    mJsonSGInfos->setProperty("KeepSVOnTop", viewSettings.keepSpeakerViewWindowOnTop);
    mJsonSGInfos->setProperty("SVGrabFocus", mMainContentComponent.speakerViewShouldGrabFocus());
    mJsonSGInfos->setProperty("showHall", viewSettings.showHall);
    mJsonSGInfos->setProperty("spatMode", spatMode); // -1, 0, 1, 2
    mJsonSGInfos->setProperty("showSourceNumber", viewSettings.showSourceNumbers);
    mJsonSGInfos->setProperty("showSpeakerNumber", viewSettings.showSpeakerNumbers);
    mJsonSGInfos->setProperty("showSpeakers", viewSettings.showSpeakers);
    mJsonSGInfos->setProperty("showSpeakerTriplets", viewSettings.showSpeakerTriplets);
    mJsonSGInfos->setProperty("showSourceActivity", viewSettings.showSourceActivity);
    mJsonSGInfos->setProperty("showSpeakerLevel", viewSettings.showSpeakerLevels);
    mJsonSGInfos->setProperty("showSphereOrCube", viewSettings.showSphereOrCube);
    mJsonSGInfos->setProperty("genMute", mMainContentComponent.getData().speakerSetup.generalMute);

    juce::Array<juce::var> triplets;
    for (auto const & triplet : mData.coldData.triplets) {
        juce::Array<juce::var> tripletsData;

        tripletsData.add(triplet.id1.get());
        tripletsData.add(triplet.id2.get());
        tripletsData.add(triplet.id3.get());

        triplets.add(tripletsData);
    }

    mJsonSGInfos->setProperty("spkTriplets", triplets);

    if (mMainContentComponent.speakerViewShouldGrabFocus()) {
        juce::MessageManager::callAsync([this] {
            mMainContentComponent.resetSpeakerViewShouldGrabFocus();
        });
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
    auto packetSize = mUdpReceiverSocket.read(receiveBuffer, mMaxBufferSize, false, senderAddress, senderPort);

    if (packetSize > 0) {
        juce::String receivedData(receiveBuffer, packetSize);
        juce::var jsonResult;
        auto res = juce::JSON::parse(receivedData, jsonResult);

        if (res == juce::Result::ok()) {
            if (jsonResult.isObject()) {
                const auto keys = jsonResult.getDynamicObject()->getProperties();

                for (int i{}; i < keys.size(); ++i) {
                    auto & property = keys.getName(i).toString();
                    juce::var value = keys.getValueAt(i);

                    if (property.compare(juce::String("selSpkNum")) == 0) {
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
                    } else if (property.compare(juce::String("keepSVTop")) == 0) {
                 
                        auto keepSVOnTopValue = value.isBool() ? static_cast<bool>(value) : value.toString().compareIgnoreCase("true") == 0;
                        juce::MessageManager::callAsync([this, keepSVOnTopValue] {
                            mMainContentComponent.handleKeepSVOnTopFromSpeakerView(keepSVOnTopValue);
                        });
                    } else if (property.compare(juce::String("showHall")) == 0) {
                        auto showHallValue = value.isBool() ? static_cast<bool>(value) : value.toString().compareIgnoreCase("true") == 0;
                        juce::MessageManager::callAsync([this, showHallValue] {
                            mMainContentComponent.handleShowHallFromSpeakerView(showHallValue);
                        });
                    } else if (property.compare(juce::String("showSrcNum")) == 0) {
                        auto showSrcValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSrcValue] {
                            mMainContentComponent.handleShowSourceNumbersFromSpeakerView(showSrcValue);
                        });
                    } else if (property.compare(juce::String("showSpkNum")) == 0) {
                        auto showSpkValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSpkValue] {
                            mMainContentComponent.handleShowSpeakerNumbersFromSpeakerView(showSpkValue);
                        });
                    } else if (property.compare(juce::String("showSpks")) == 0) {
                        auto showSpksValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSpksValue] {
                            mMainContentComponent.handleShowSpeakersFromSpeakerView(showSpksValue);
                        });
                    } else if (property.compare(juce::String("showSpkTriplets")) == 0) {
                        auto showSpksTripletsValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSpksTripletsValue] {
                            mMainContentComponent.handleShowSpeakerTripletsFromSpeakerView(showSpksTripletsValue);
                        });
                    } else if (property.compare(juce::String("showSrcActivity")) == 0) {
                        auto showSrcActivityValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSrcActivityValue] {
                            mMainContentComponent.handleShowSourceActivityFromSpeakerView(showSrcActivityValue);
                        });
                    } else if (property.compare(juce::String("showSpkLevel")) == 0) {
                        auto showSpkLevelValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSpkLevelValue] {
                            mMainContentComponent.handleShowSpeakerLevelFromSpeakerView(showSpkLevelValue);
                        });
                    } else if (property.compare(juce::String("showSphereCube")) == 0) {
                        auto showSphereOrCubeValue = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, showSphereOrCubeValue] {
                            mMainContentComponent.handleShowSphereOrCubeFromSpeakerView(showSphereOrCubeValue);
                        });
                    } else if (property.compare(juce::String("resetSrcPos")) == 0) {
                        if (static_cast<int>(value) != 0) {
                            juce::MessageManager::callAsync(
                                [this] { mMainContentComponent.handleResetSourcesPositionsFromSpeakerView(); });
                        }
                    } else if (property.compare(juce::String("genMute")) == 0) {
                        auto generalMute = static_cast<bool>(value);
                        juce::MessageManager::callAsync([this, generalMute] {
                            mMainContentComponent.handleGeneralMuteFromSpeakerView(generalMute);
                        });
                    } else if (property.compare(juce::String("winPos")) == 0) {
                        auto winPosValue = value;
                        juce::MessageManager::callAsync([this, winPosValue] {
                            mMainContentComponent.handleWindowPositionFromSpeakerView(winPosValue);
                        });
                    } else if (property.compare(juce::String("winSize")) == 0) {
                        auto winSizeValue = value;
                        juce::MessageManager::callAsync([this, winSizeValue] {
                            mMainContentComponent.handleWindowSizeFromSpeakerView(winSizeValue);
                        });
                    } else if (property.compare(juce::String("camPos")) == 0) {
                        auto camPosValue = value;
                        juce::MessageManager::callAsync([this, camPosValue] {
                            mMainContentComponent.handleCameraPositionFromSpeakerView(camPosValue);
                        });
                    } else if (property.compare(juce::String("quitting")) == 0) {
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
    juce::DatagramSocket udpSenderSocket;
    juce::String remoteHostname = "127.0.0.1";

    udpSenderSocket.bindToPort(DEFAULT_UDP_INPUT_PORT, remoteHostname);

    juce::String jsonSourcesStr = juce::JSON::toString(juce::var(mJsonSources));
    if (udpSenderSocket.waitUntilReady(true, 0) == 0) {
        [[maybe_unused]] int numBytesWrittenSources
            = udpSenderSocket.write(remoteHostname,
                                    DEFAULT_UDP_INPUT_PORT,
                                    jsonSourcesStr.toStdString().c_str(),
                                    static_cast<int>(jsonSourcesStr.toStdString().length()));
        jassert(!(numBytesWrittenSources < 0));
    }

    juce::String jsonSpeakersStr = juce::JSON::toString(juce::var(mJsonSpeakers));
    if (udpSenderSocket.waitUntilReady(true, 0) == 0) {
        [[maybe_unused]] int numBytesWrittenSpeakers
            = udpSenderSocket.write(remoteHostname,
                                    DEFAULT_UDP_INPUT_PORT,
                                    jsonSpeakersStr.toStdString().c_str(),
                                    static_cast<int>(jsonSpeakersStr.toStdString().length()));
        jassert(!(numBytesWrittenSpeakers < 0));
    }

    if (mJsonSGInfos != nullptr) {
        juce::var jsonSGInfos(mJsonSGInfos.release());
        juce::String jsonSGInfosStr = juce::JSON::toString(jsonSGInfos);
        if (udpSenderSocket.waitUntilReady(true, 0) == 0) {
            [[maybe_unused]] int numBytesWrittenSGInfos
                = udpSenderSocket.write(remoteHostname,
                                        DEFAULT_UDP_INPUT_PORT,
                                        jsonSGInfosStr.toStdString().c_str(),
                                        static_cast<int>(jsonSGInfosStr.toStdString().length()));
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
        = mUdpReceiverSocket.read(receiveBuffer, mMaxBufferSize, false, senderAddress, senderPort);
}

} // namespace gris
