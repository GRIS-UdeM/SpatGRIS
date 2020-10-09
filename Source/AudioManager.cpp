#include "AudioManager.h"

#if !USE_JACK

//==============================================================================
AudioManager & AudioManager::getInstance()
{
    static AudioManager instance{};
    return instance;
}

#endif
