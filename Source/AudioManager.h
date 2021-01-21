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

class JackClient;

//==============================================================================
enum class PortType { input, output };

//==============================================================================
struct jack_port_t {
    uint32_t id;
    char shortName[64]{};
    char clientName[64]{};
    char fullName[128]{};
    PortType type;
    std::optional<int> physicalPort;
    juce::AudioBuffer<float> buffer{};

    jack_port_t(uint32_t const newId,
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
    uint32_t mLastGivePortId{};
    juce::AudioBuffer<float> mInputPortsBuffer{};
    juce::AudioBuffer<float> mOutputPortsBuffer{};

    JackClient * mJackClient{};

    juce::CriticalSection mCriticalSection{};

    static std::unique_ptr<AudioManager> mInstance;

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

    void registerJackClient(JackClient * jackClient);

    [[nodiscard]] juce::Array<jack_port_t *> getInputPorts() const;
    [[nodiscard]] juce::Array<jack_port_t *> getOutputPorts() const;
    float * getBuffer(jack_port_t * port, size_t nFrames);

    [[nodiscard]] jack_port_t * getPort(char const * name) const;
    [[nodiscard]] jack_port_t * getPort(uint32_t id) const;

    [[nodiscard]] std::vector<std::string> getPortNames(PortType portType) const;

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
                     std::optional<juce::String> deviceType,
                     double sampleRate,
                     int bufferSize);
    [[nodiscard]] static AudioManager & getInstance();
    static void free();

private:
    //==============================================================================
    AudioManager(juce::String const & inputDevice,
                 juce::String const & outputDevice,
                 std::optional<juce::String> deviceType,
                 double sampleRate,
                 int bufferSize);
    //==============================================================================
    void setBufferSizes(int numSamples);
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioManager)
}; // class AudioManager
