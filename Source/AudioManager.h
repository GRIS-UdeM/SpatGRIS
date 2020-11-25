/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <optional>

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "JackMockup.h"

//==============================================================================
enum class PortType { input, output };

//==============================================================================
struct _jack_port {
    jack_port_id_t id;
    char shortName[64]{};
    char clientName[64]{};
    char fullName[128]{};
    PortType type;
    std::optional<int> physicalPort;
    juce::AudioBuffer<float> buffer{};

    _jack_port(jack_port_id_t const newId,
               char const * const newShortName,
               char const * const newClientName,
               PortType const newType,
               std::optional<int> const newPhysicalPort = std::nullopt)
        : id(newId)
        , type(newType)
        , physicalPort(newPhysicalPort)
    {
        std::strcpy(shortName, newShortName);
        std::strcpy(clientName, newClientName);
        std::strcpy(fullName, newClientName);
        std::strcat(fullName, ":");
        std::strcat(fullName, newShortName);
    }
};

//==============================================================================
class AudioManager final : juce::AudioSourcePlayer
{
    juce::AudioDeviceManager mAudioDeviceManager{};
    juce::OwnedArray<jack_port_t> mVirtualInputPorts{};
    juce::OwnedArray<jack_port_t> mPhysicalInputPorts{};
    juce::OwnedArray<jack_port_t> mVirtualOutputPorts{};
    juce::OwnedArray<jack_port_t> mPhysicalOutputPorts{};
    juce::HashMap<jack_port_t *, jack_port_t *> mConnections{};
    jack_port_id_t mLastGivePortId{};
    juce::AudioBuffer<float> mInputPortsBuffer{};
    juce::AudioBuffer<float> mOutputPortsBuffer{};

    JackProcessCallback mProcessCallback{};
    void * mProcessCallbackArg{};
    JackPortConnectCallback mPortConnectCallback{};

    juce::CriticalSection mCriticalSection{};

    static std::unique_ptr<AudioManager> mInstance;
    //==============================================================================
    // Dummies
    jack_client_t mDummyJackClient{};
    jackctl_server_t mDummyJackCtlServer{};

public:
    //==============================================================================
    ~AudioManager() = default;

    AudioManager(AudioManager const &) = delete;
    AudioManager(AudioManager &&) = delete;

    AudioManager & operator=(AudioManager const &) = delete;
    AudioManager & operator=(AudioManager &&) = delete;
    //==============================================================================
    [[nodiscard]] juce::AudioDeviceManager const & getAudioDeviceManager() const { return mAudioDeviceManager; }
    [[nodiscard]] juce::AudioDeviceManager & getAudioDeviceManager() { return mAudioDeviceManager; }

    [[nodiscard]] jack_port_t * registerPort(char const * newShortName,
                                             char const * newClientName,
                                             PortType newType,
                                             std::optional<int> newPhysicalPort = std::nullopt);
    void unregisterPort(jack_port_t * port);

    [[nodiscard]] bool isConnectedTo(jack_port_t const * port, char const * port_name) const;
    [[nodiscard]] jack_port_t * findPortByName(char const * name) const;

    void registerPortConnectCallback(JackPortConnectCallback const callback) { mPortConnectCallback = callback; }
    void registerProcessCallback(JackProcessCallback const callback, void * arg);

    [[nodiscard]] juce::Array<jack_port_t *> getInputPorts() const;
    [[nodiscard]] juce::Array<jack_port_t *> getOutputPorts() const;
    void * getBuffer(jack_port_t * port, jack_nframes_t nFrames);

    [[nodiscard]] std::optional<jack_port_t *> getPort(char const * name) const;

    void connect(char const * sourcePortName, char const * destinationPortName);
    void disconnect(jack_port_t * source, jack_port_t * destination);
    //==============================================================================
    // AudioSourcePlayer overrides
    void audioDeviceError(const juce::String & errorMessage) override;
    void audioDeviceIOCallback(const float ** inputChannelData,
                               int totalNumInputChannels,
                               float ** outputChannelData,
                               int totalNumOutputChannels,
                               int numSamples) override;
    void audioDeviceAboutToStart(juce::AudioIODevice * device) override;
    void audioDeviceStopped() override;
    //==============================================================================
    static void init(juce::String const & inputDevice,
                     juce::String const & outputDevice,
                     std::optional<juce::String> deviceType);
    [[nodiscard]] static AudioManager & getInstance();
    static void free();
    //==============================================================================
    // Dummies
    [[nodiscard]] auto * getDummyJackClient() { return &mDummyJackClient; }
    [[nodiscard]] auto * getDummyJackCtlServer() { return &mDummyJackCtlServer; }
    [[nodiscard]] static JSList * getDummyJackCtlParameters() { return nullptr; }

private:
    //==============================================================================
    AudioManager(juce::String const & inputDevice,
                 juce::String const & outputDevice,
                 std::optional<juce::String> deviceType);
    //==============================================================================
    void setBufferSizes(int numSamples);
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioManager)
}; // class AudioManager
