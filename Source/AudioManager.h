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
class AudioManager
{
    juce::AudioDeviceManager mAudioDeviceManager{};
    juce::OwnedArray<jack_port_t> mInputPorts{};
    juce::OwnedArray<jack_port_t> mOutputPorts{};
    juce::HashMap<jack_port_t const *, jack_port_t *> mConnections{};
    jack_port_id_t mLastGivePortId{};

    JackProcessCallback mProcessCallback;
    JackPortConnectCallback mPortConnectCallback;
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
    void registerProcessCallback(JackProcessCallback const callback) { mProcessCallback = callback; }

    auto const & getInputPorts() const { return mInputPorts; }
    auto const & getOutputPorts() const { return mOutputPorts; }

    std::optional<jack_port_t *> getPort(char const * name) const;

    void connect(char const * sourcePortName, char const * destinationPortName);
    void disconnect(jack_port_t * source, jack_port_t * destination);
    //==============================================================================
    // Dummies
    auto * getDummyJackClient() { return &mDummyJackClient; }
    auto * getDummyJackCtlServer() { return &mDummyJackCtlServer; }
    JSList * getDummyJackCtlParameters() const { return nullptr; }
    //==============================================================================
    static AudioManager & getInstance();

private:
    //==============================================================================
    AudioManager();
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioManager)
}; // class AudioManager