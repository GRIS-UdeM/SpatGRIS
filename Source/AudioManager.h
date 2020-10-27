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

    _jack_port(jack_port_id_t const newId,
               char const * const newShortName,
               char const * const newClientName,
               PortType const newType,
               std::optional<int> newPhysicalPort = std::nullopt)
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
    juce::OwnedArray<jack_port_t> mInputPorts{};
    juce::OwnedArray<jack_port_t> mOutputPorts{};
    juce::HashMap<jack_port_t const *, jack_port_t *> mConnections{};
    jack_port_id_t mLastGivePortId{};
    juce::AudioBuffer<float> mInputBuffer{};
    juce::AudioBuffer<float> mOutputBuffer{};

    JackProcessCallback mProcessCallback;
    void * mProcessCallbackArg{};
    JackPortConnectCallback mPortConnectCallback;

    juce::CriticalSection mCriticalSection{};

    // temp
    juce::AudioDeviceSelectorComponent mAudioDeviceSelectorComponent{
        mAudioDeviceManager, 2, 256, 2, 256, false, false, false, false
    };
    //==============================================================================
    // Dummies
    jack_client_t mDummyJackClient{};
    jackctl_server_t mDummyJackCtlServer{};

public:
    //==============================================================================
    // temp
    auto & getSelectorComponent() { return mAudioDeviceSelectorComponent; }
    //==============================================================================
    ~AudioManager() = default;

    AudioManager(AudioManager const &) = delete;
    AudioManager(AudioManager &&) = delete;

    AudioManager & operator=(AudioManager const &) = delete;
    //==============================================================================
    juce::AudioDeviceManager const & getAudioDeviceManager() const { return mAudioDeviceManager; }
    juce::AudioDeviceManager & getAudioDeviceManager() { return mAudioDeviceManager; }

    jack_port_t * registerPort(char const * newShortName,
                               char const * newClientName,
                               PortType newType,
                               std::optional<int> newPhysicalPort = std::nullopt);
    void unregisterPort(jack_port_t * port);

    bool isConnectedTo(jack_port_t const * port, char const * port_name) const;
    jack_port_t * findPortByName(char const * name) const;

    void registerPortConnectCallback(JackPortConnectCallback const callback) { mPortConnectCallback = callback; }
    void registerProcessCallback(JackProcessCallback const callback, void * arg);

    auto const & getInputPorts() const { return mInputPorts; }
    auto const & getOutputPorts() const { return mOutputPorts; }
    void * getBuffer(jack_port_t * port, jack_nframes_t nFrames);

    std::optional<jack_port_t *> getPort(char const * name) const;

    void connect(char const * sourcePortName, char const * destinationPortName);
    void disconnect(jack_port_t * source, jack_port_t * destination);
    //==============================================================================
    // Dummies
    auto * getDummyJackClient() { return &mDummyJackClient; }
    auto * getDummyJackCtlServer() { return &mDummyJackCtlServer; }
    JSList * getDummyJackCtlParameters() const { return nullptr; }
    //==============================================================================
    // AudioSourcePlayer overrides
    void audioDeviceError(const String & errorMessage) override;
    void audioDeviceIOCallback(const float ** inputChannelData,
                               int totalNumInputChannels,
                               float ** outputChannelData,
                               int totalNumOutputChannels,
                               int numSamples) override;
    void audioDeviceAboutToStart(AudioIODevice * device) override;
    void audioDeviceStopped() override;
    //==============================================================================
    static AudioManager & getInstance();

private:
    //==============================================================================
    void setBufferSizes(int numSamples);
    AudioManager();
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioManager)
}; // class AudioManager