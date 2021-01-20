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

#if !USE_JACK

    #include "JackMockup.h"

    #include "macros.h"

DISABLE_WARNINGS
    #include <JuceHeader.h>
ENABLE_WARNINGS

    #include "AudioManager.h"
    #include "MainComponent.h"

juce::HashMap<void *, std::function<void()>> freeing_functions{};

//==============================================================================
char const ** jack_get_ports([[maybe_unused]] jack_client_t * const client,
                             [[maybe_unused]] const char * const port_name_pattern,
                             [[maybe_unused]] const char * const type_name_pattern,
                             [[maybe_unused]] unsigned long const flags)
{
    auto & audioManager{ AudioManager::getInstance() };
    jassert(client == audioManager.getDummyJackClient());
    jassert(port_name_pattern == nullptr);
    jassert(type_name_pattern == nullptr || juce::String{ "32 bit float mono audio" } == type_name_pattern);

    static auto constexpr MAX_NUM_PORTS{ 256u };
    static auto constexpr MAX_PORT_NAME_LENGTH{ 64u };
    auto ** result{ new char *[MAX_NUM_PORTS] };
    std::fill(result, result + MAX_NUM_PORTS, nullptr);

    auto const isInput{ flags & JackPortFlags::JackPortIsInput };
    auto const isOutput{ flags & JackPortFlags::JackPortIsOutput };

    jassert(isInput || isOutput);
    jassert(!(isInput && isOutput));
    jassert(flags - isInput - isOutput == 0u);

    auto const ports{ isInput ? audioManager.getInputPorts() : audioManager.getOutputPorts() };

    size_t currentInputChannelCount{};
    for (auto const & port : ports) {
        auto * newInputChannelName{ new char[MAX_PORT_NAME_LENGTH] };
        std::strcpy(newInputChannelName, port->fullName);
        result[currentInputChannelCount++] = newInputChannelName;
    }

    freeing_functions.set(result, [=]() {
        size_t currentIndex{};
        while (result[currentIndex] != nullptr) {
            delete[] result[currentIndex++];
        }
        delete[] result;
    });

    return const_cast<char const **>(result);
}

//==============================================================================
jack_port_t * jack_port_by_id(jack_client_t * /*client*/, jack_port_id_t const port_id)
{
    auto & audioManager{ AudioManager::getInstance() };

    for (auto * port : audioManager.getInputPorts()) {
        if (port->id == port_id) {
            return port;
        }
    }
    for (auto * port : audioManager.getOutputPorts()) {
        if (port->id == port_id) {
            return port;
        }
    }

    return nullptr;
}

//==============================================================================
void jack_free(void * ptr)
{
    jassert(freeing_functions.contains(ptr));
    freeing_functions.getReference(ptr)();
    freeing_functions.remove(ptr);
}

#endif
