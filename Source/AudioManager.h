#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "JackMockup.h"

//==============================================================================
class AudioManager
{
    jack_client_t mDummyJackClient{};
    jackctl_server_t mDummyJackCtlServer{};
    JSList mDummyJackCtlParameters{};

public:
    auto * getDummyJackClient() { return &mDummyJackClient; }
    auto * getDummyJackCtlServer() { return &mDummyJackCtlServer; }
    auto * getDummyJackCtlParameters() { return &mDummyJackCtlParameters; }
    //==============================================================================
    static AudioManager & getInstance();

private:
    //==============================================================================
    AudioManager() = default;
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioManager)
}; // class AudioManager