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

#include "Configuration.h"

class AudioProcessor;

//==============================================================================
enum class PortType { input, output };

//==============================================================================
// TODO : this needs to go
struct audio_port_t {
    uint32_t id;
    char shortName[64]{};
    char clientName[64]{};
    char fullName[128]{};
    PortType type;
    std::optional<int> physicalPort;
    juce::AudioBuffer<float> buffer{};

    audio_port_t(uint32_t const newId,
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
public:
    struct RecordingOptions {
        juce::String path;
        RecordingFormat format;
        RecordingConfig config;
        double sampleRate;
    };

private:
    juce::AudioDeviceManager mAudioDeviceManager{};
    juce::OwnedArray<audio_port_t> mVirtualInputPorts{};
    juce::OwnedArray<audio_port_t> mPhysicalInputPorts{};
    juce::OwnedArray<audio_port_t> mVirtualOutputPorts{};
    juce::OwnedArray<audio_port_t> mPhysicalOutputPorts{};
    juce::HashMap<audio_port_t *, audio_port_t *> mConnections{};
    uint32_t mLastGivePortId{};
    juce::AudioBuffer<float> mInputPortsBuffer{};
    juce::AudioBuffer<float> mOutputPortsBuffer{};

    AudioProcessor * mAudioProcessor{};

    juce::CriticalSection mCriticalSection{};
    bool mIsRecording{};
    juce::OwnedArray<juce::AudioFormatWriter> mRecorders{};
    RecordingConfig mRecordingConfig{};

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

    audio_port_t * registerPort(char const * newShortName,
                                char const * newClientName,
                                PortType newType,
                                std::optional<int> newPhysicalPort = std::nullopt);
    void unregisterPort(audio_port_t * port);

    [[nodiscard]] bool isConnectedTo(audio_port_t const * port, char const * port_name) const;
    [[nodiscard]] bool isConnectedTo(audio_port_t const * portA, audio_port_t const * portB) const;

    void registerAudioProcessor(AudioProcessor * audioProcessor);

    [[nodiscard]] juce::Array<audio_port_t *> getInputPorts() const;
    [[nodiscard]] juce::Array<audio_port_t *> getOutputPorts() const;
    float * getBuffer(audio_port_t * port, size_t nFrames) noexcept;

    [[nodiscard]] audio_port_t * getPort(char const * name) const;
    [[nodiscard]] audio_port_t * getPort(uint32_t id) const;

    [[nodiscard]] std::vector<std::string> getPortNames(PortType portType) const;

    void connect(char const * sourcePortName, char const * destinationPortName);
    void connect(audio_port_t * sourcePort, audio_port_t * destinationPort);
    void disconnect(audio_port_t * sourcePort, audio_port_t * destinationPort);

    juce::StringArray getAvailableDeviceTypeNames();

    bool prepareToRecord(RecordingOptions const & recordingOptions);
    void startRecording();
    void stopRecording();
    bool isRecording() const { return mIsRecording; }
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
    static void init(juce::String const & deviceType,
                     juce::String const & inputDevice,
                     juce::String const & outputDevice,
                     double sampleRate,
                     int bufferSize);
    [[nodiscard]] static AudioManager & getInstance();
    static void free();

private:
    //==============================================================================
    AudioManager(juce::String const & deviceType,
                 juce::String const & inputDevice,
                 juce::String const & outputDevice,
                 double sampleRate,
                 int bufferSize);
    //==============================================================================
    void setBufferSizes(int numSamples);
    bool tryInitAudioDevice(juce::String const & deviceType,
                            juce::String const & inputDevice,
                            juce::String const & outputDevice,
                            double sampleRate,
                            int bufferSize);
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioManager)
}; // class AudioManager
