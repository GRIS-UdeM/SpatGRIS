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
jack_client_t * jackClientOpen([[maybe_unused]] const char * const client_name,
                               [[maybe_unused]] jack_options_t const options,
                               jack_status_t * status,
                               ...)
{
    jassert(juce::String{ "SpatGRIS2" } == client_name);
    jassert(options == jack_options_t::JackNullOption);

    *status = jack_status_t::JackServerStarted;
    return AudioManager::getInstance().getDummyJackClient();
}

//==============================================================================
char * jack_get_client_name([[maybe_unused]] jack_client_t * client)
{
    jassert(client == AudioManager::getInstance().getDummyJackClient());

    return "SpatGRIS2";
}

//==============================================================================
int jack_deactivate([[maybe_unused]] jack_client_t * client)
{
    jassert(client == AudioManager::getInstance().getDummyJackClient());
    jassert(AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() != nullptr);
    jassert(AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice()->isOpen());

    AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice()->close();
    return 0;
}

//==============================================================================
int jack_set_process_callback([[maybe_unused]] jack_client_t * const client,
                              JackProcessCallback const process_callback,
                              void * arg)
{
    auto & audioManager{ AudioManager::getInstance() };
    jassert(client == audioManager.getDummyJackClient());

    audioManager.registerProcessCallback(process_callback, arg);
    return 0;
}

//==============================================================================
int jack_set_port_connect_callback([[maybe_unused]] jack_client_t * client,
                                   JackPortConnectCallback const connect_callback,
                                   void * /*arg*/)
{
    auto & audioManager{ AudioManager::getInstance() };
    jassert(client == audioManager.getDummyJackClient());

    audioManager.registerPortConnectCallback(connect_callback);
    return 0;
}

//==============================================================================
jack_nframes_t jack_get_sample_rate([[maybe_unused]] jack_client_t * client)
{
    jassert(client == AudioManager::getInstance().getDummyJackClient());

    auto * audioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
    jassert(audioDevice != nullptr);

    auto const sampleRate{ audioDevice->getCurrentSampleRate() };
    jassert(std::round(sampleRate) == sampleRate);

    return static_cast<jack_nframes_t>(sampleRate);
}

//==============================================================================
jack_nframes_t jack_get_buffer_size([[maybe_unused]] jack_client_t * client)
{
    jassert(client == AudioManager::getInstance().getDummyJackClient());

    auto * audioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
    jassert(audioDevice != nullptr);

    auto const bufferSize{ audioDevice->getCurrentBufferSizeSamples() };
    jassert(bufferSize > 0);

    return static_cast<jack_nframes_t>(bufferSize);
}

//==============================================================================
float jack_cpu_load(jack_client_t * client)
{
    auto & audioManager{ AudioManager::getInstance() };
    jassert(client == audioManager.getDummyJackClient());

    auto const usage{ audioManager.getAudioDeviceManager().getCpuUsage() };
    return static_cast<float>(usage);
}

//==============================================================================
jack_port_t * jack_port_register([[maybe_unused]] jack_client_t * client,
                                 const char * port_name,
                                 [[maybe_unused]] const char * port_type,
                                 unsigned long flags,
                                 [[maybe_unused]] unsigned long buffer_size)
{
    auto & audioManager{ AudioManager::getInstance() };
    jassert(client == audioManager.getDummyJackClient());
    jassert(strcmp(port_type, "32 bit float mono audio") == 0);
    jassert(buffer_size == 0u);

    auto const isInput{ flags & JackPortFlags::JackPortIsInput };
    auto const isOutput{ flags & JackPortFlags::JackPortIsOutput };

    jassert(isInput || isOutput);              // must be input or output...
    jassert(!(isInput && isOutput));           // ...but not both...
    jassert(flags - isInput - isOutput == 0u); // ...and nothing else

    auto & deviceManager{ audioManager.getAudioDeviceManager() };
    auto * device{ deviceManager.getCurrentAudioDevice() };
    jassert(device != nullptr);

    auto * port{
        audioManager.registerPort(port_name, jack_get_client_name(client), isInput ? PortType::input : PortType::output)
    };

    return port;
}

//==============================================================================
int jack_port_unregister([[maybe_unused]] jack_client_t * client, jack_port_t * port)
{
    auto & audioManager{ AudioManager::getInstance() };
    jassert(client == audioManager.getDummyJackClient());

    audioManager.unregisterPort(port);

    return 0;
}

//==============================================================================
void * jack_port_get_buffer(jack_port_t * port, jack_nframes_t const nframes)
{
    return AudioManager::getInstance().getBuffer(port, nframes);
}

//==============================================================================
const char * jack_port_name(const jack_port_t * port)
{
    return port->fullName;
}

//==============================================================================
const char * jack_port_short_name(const jack_port_t * port)
{
    return port->shortName;
}

//==============================================================================
int jack_port_connected_to(const jack_port_t * port, const char * port_name)
{
    return AudioManager::getInstance().isConnectedTo(port, port_name);
}

//==============================================================================
int jack_connect([[maybe_unused]] jack_client_t * client, const char * source_port, const char * destination_port)
{
    auto & audioManager{ AudioManager::getInstance() };
    jassert(client == audioManager.getDummyJackClient());

    audioManager.connect(source_port, destination_port);

    return 0;
}

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
jack_port_t * jack_port_by_name([[maybe_unused]] jack_client_t * const client, const char * const port_name)
{
    auto & audioManager{ AudioManager::getInstance() };
    jassert(client == audioManager.getDummyJackClient());

    auto const maybe_result{ audioManager.getPort(port_name) };
    jassert(maybe_result);
    auto * const result{ *maybe_result };
    return result;
}

//==============================================================================
jack_port_t * jack_port_by_id(jack_client_t * /*client*/, jack_port_id_t const port_id)
{
    auto & audioManager{ AudioManager::getInstance() };
    //    jassert(client == audioManager.getDummyJackClient());

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

//==============================================================================
const JSList * jackctl_server_get_parameters(jackctl_server_t * /*server*/)
{
    return AudioManager::getInstance().getDummyJackCtlParameters();
}

#endif
