#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

//==============================================================================
class AudioManager
{
public:
    //==============================================================================
    static AudioManager & getInstance();

private:
    //==============================================================================
    AudioManager() = default;
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioManager)
}; // class AudioManager