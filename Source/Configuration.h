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

#include "SpatMode.hpp"

//==============================================================================
enum class RecordingFormat { wav, aiff };
enum class RecordingConfig { mono, interleaved };

static constexpr auto DEFAULT_OSC_INPUT_PORT = 18032;
static constexpr auto MAX_OSC_INPUT_PORT = 65535;

extern juce::StringArray const RECORDING_FORMAT_STRINGS;
extern juce::StringArray const RECORDING_CONFIG_STRINGS;
extern juce::StringArray const ATTENUATION_DB_STRINGS;
extern juce::StringArray const ATTENUATION_FREQUENCY_STRINGS;

class Configuration
{
    struct Tags {
        static juce::String const DEVICE_TYPE;
        static juce::String const INPUT_DEVICE;
        static juce::String const OUTPUT_DEVICE;
        static juce::String const BUFFER_SIZE;
        static juce::String const SAMPLE_RATE;
        static juce::String const OSC_INPUT_PORT;
        static juce::String const RECORDING_FORMAT;
        static juce::String const RECORDING_CONFIG;
        static juce::String const ATTENUATION_DB;
        static juce::String const ATTENUATION_HZ;
        static juce::String const LAST_PRESET;
        static juce::String const LAST_SPEAKER_SETUP;
        static juce::String const LAST_RECORDING_DIRECTORY;
        static juce::String const SASH_POSITION;
        static juce::String const WINDOW_X;
        static juce::String const WINDOW_Y;
        static juce::String const WINDOW_WIDTH;
        static juce::String const WINDOW_HEIGHT;
        static juce::String const LAST_SPAT_MODE;
    };

    juce::ApplicationProperties mApplicationProperties{};
    juce::PropertiesFile * mUserSettings{};

public:
    //==============================================================================
    Configuration();
    ~Configuration();
    //==============================================================================
    Configuration(Configuration const &) = delete;
    Configuration(Configuration &&) = delete;
    Configuration & operator=(Configuration const &) = delete;
    Configuration & operator=(Configuration &&) = delete;
    //==============================================================================
    void setDeviceType(juce::String const & deviceType) const;
    void setInputDevice(juce::String const & inputDevice) const;
    void setOutputDevice(juce::String const & outputDevice) const;
    void setSampleRate(double sampleRate) const;
    void setBufferSize(int bufferSize) const;
    void setAttenuationDbIndex(int attenuationIndex) const;
    void setAttenuationFrequencyIndex(int frequencyIndex) const;

    void setRecordingFormat(RecordingFormat recordingFormat) const;
    void setRecordingConfig(RecordingConfig recordingConfig) const;
    void setLastRecordingDirectory(juce::File const & lastRecordingDirectory) const;
    void setLastSpatMode(SpatMode spatMode) const;

    void setOscInputPort(int oscInputPort) const;

    void setWindowX(int windowX) const;
    void setWindowY(int windowY) const;
    void setWindowWidth(int windowWidth) const;
    void setWindowHeight(int windowHeight) const;
    void setSashPosition(double sashPosition) const;

    void setLastOpenProject(juce::File const & lastOpenPreset) const;
    void setLastSpeakerSetup_(juce::File const & lastOpenSpeakerSetup) const;
    //==============================================================================
    [[nodiscard]] juce::String getDeviceType() const;
    [[nodiscard]] juce::String getInputDevice() const;
    [[nodiscard]] juce::String getOutputDevice() const;
    [[nodiscard]] double getSampleRate() const;
    [[nodiscard]] int getBufferSize() const;
    [[nodiscard]] int getAttenuationDbIndex() const;
    [[nodiscard]] int getAttenuationFrequencyIndex() const;

    [[nodiscard]] RecordingFormat getRecordingFormat() const;
    [[nodiscard]] RecordingConfig getRecordingConfig() const;
    [[nodiscard]] juce::File getLastRecordingDirectory() const;
    [[nodiscard]] SpatMode getLastSpatMode() const;

    [[nodiscard]] int getOscInputPort() const;

    [[nodiscard]] int getWindowX() const;
    [[nodiscard]] int getWindowY() const;
    [[nodiscard]] int getWindowWidth() const;
    [[nodiscard]] int getWindowHeight() const;
    [[nodiscard]] std::optional<double> getSashPosition() const;

    [[nodiscard]] juce::File getLastOpenProject() const;
    [[nodiscard]] juce::File getLastSpeakerSetup_() const;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Configuration)
};