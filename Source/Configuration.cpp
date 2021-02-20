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

#include "Configuration.h"

#include "AudioManager.h"

juce::String const Configuration::Tags::DEVICE_TYPE = "DEVICE_TYPE";
juce::String const Configuration::Tags::INPUT_DEVICE = "INPUT_DEVICE";
juce::String const Configuration::Tags::OUTPUT_DEVICE = "OUTPUT_DEVICE";
juce::String const Configuration::Tags::BUFFER_SIZE = "BUFFER_SIZE";
juce::String const Configuration::Tags::SAMPLE_RATE = "SAMPLE_RATE";
juce::String const Configuration::Tags::OSC_INPUT_PORT = "OSC_INPUT_PORT";
juce::String const Configuration::Tags::RECORDING_FORMAT = "RECORDING_FORMAT";
juce::String const Configuration::Tags::RECORDING_CONFIG = "RECORDING_CONFIG";
juce::String const Configuration::Tags::ATTENUATION_DB = "ATTENUATION_DB";
juce::String const Configuration::Tags::ATTENUATION_HZ = "ATTENUATION_HZ";
juce::String const Configuration::Tags::LAST_VBAP_SPEAKER_SETUP = "LAST_VBAP_SPEAKER_SETUP";
juce::String const Configuration::Tags::LAST_OPEN_PRESET = "LAST_OPEN_PRESET";
juce::String const Configuration::Tags::LAST_OPEN_SPEAKER_SETUP = "LAST_OPEN_SPEAKER_SETUP";
juce::String const Configuration::Tags::LAST_RECORDING_DIRECTORY = "LAST_RECORDING_DIRECTORY";
juce::String const Configuration::Tags::LAST_PRESET_DIRECTORY = "LAST_PRESET_DIRECTORY";
juce::String const Configuration::Tags::SASH_POSITION = "SASH_POSITION";
juce::String const Configuration::Tags::LAST_SPEAKER_SETUP_DIRECTORY = "LAST_SPEAKER_SETUP_DIRECTORY";
juce::String const Configuration::Tags::WINDOW_X = "WINDOW_X";
juce::String const Configuration::Tags::WINDOW_Y = "WINDOW_Y";
juce::String const Configuration::Tags::WINDOW_WIDTH = "WINDOW_WIDTH";
juce::String const Configuration::Tags::WINDOW_HEIGHT = "WINDOW_HEIGHT";

juce::StringArray const RECORDING_FORMAT_STRINGS{ "WAV", "AIFF" };
juce::StringArray const RECORDING_CONFIG_STRINGS{ "Multiple Mono Files", "Single Interleaved" };
juce::StringArray const ATTENUATION_DB_STRINGS{ "0", "-12", "-24", "-36", "-48", "-60", "-72" };
juce::StringArray const ATTENUATION_FREQUENCY_STRINGS{ "125", "250", "500", "1000", "2000", "4000", "8000", "16000 " };

//==============================================================================
Configuration::Configuration()
{
    // App user settings storage file.
    juce::PropertiesFile::Options options{};
    options.applicationName = juce::JUCEApplicationBase::getInstance()->getApplicationName();
    options.commonToAllUsers = false;
    options.filenameSuffix = "xml";
    options.folderName = "GRIS";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.ignoreCaseOfKeyNames = true;
    options.osxLibrarySubFolder = "Application Support";
    mApplicationProperties.setStorageParameters(options);
    mUserSettings = mApplicationProperties.getUserSettings();
}

//==============================================================================
Configuration::~Configuration()
{
    mApplicationProperties.saveIfNeeded();
    mApplicationProperties.closeFiles();
}

//==============================================================================
void Configuration::setDeviceType(juce::String const & deviceType) const
{
    mUserSettings->setValue(Tags::DEVICE_TYPE, deviceType);
}

//==============================================================================
void Configuration::setInputDevice(juce::String const & inputDevice) const
{
    mUserSettings->setValue(Tags::INPUT_DEVICE, inputDevice);
}

//==============================================================================
void Configuration::setOutputDevice(juce::String const & outputDevice) const
{
    mUserSettings->setValue(Tags::OUTPUT_DEVICE, outputDevice);
}

//==============================================================================
void Configuration::setSampleRate(double const sampleRate) const
{
    mUserSettings->setValue(Tags::SAMPLE_RATE, sampleRate);
}

//==============================================================================
void Configuration::setBufferSize(int const bufferSize) const
{
    mUserSettings->setValue(Tags::BUFFER_SIZE, bufferSize);
}

//==============================================================================
void Configuration::setAttenuationDbIndex(int const attenuationIndex) const
{
    jassert(attenuationIndex >= 0 && attenuationIndex < ATTENUATION_DB_STRINGS.size());
    mUserSettings->setValue(Tags::ATTENUATION_DB, ATTENUATION_DB_STRINGS[attenuationIndex]);
}

//==============================================================================
void Configuration::setAttenuationFrequencyIndex(int const frequencyIndex) const
{
    jassert(frequencyIndex >= 0 && frequencyIndex < ATTENUATION_FREQUENCY_STRINGS.size());
    mUserSettings->setValue(Tags::ATTENUATION_HZ, ATTENUATION_FREQUENCY_STRINGS[frequencyIndex]);
}

//==============================================================================
void Configuration::setRecordingFormat(RecordingFormat const recordingFormat) const
{
    auto const recordingFormatIndex{ static_cast<int>(recordingFormat) };
    jassert(recordingFormatIndex >= 0 && recordingFormatIndex < RECORDING_FORMAT_STRINGS.size());
    auto const & recordingFormatString{ RECORDING_FORMAT_STRINGS.getReference(recordingFormatIndex) };
    mUserSettings->setValue(Tags::RECORDING_FORMAT, recordingFormatString);
}

//==============================================================================
void Configuration::setRecordingConfig(RecordingConfig const recordingConfig) const
{
    auto const recordingConfigIndex{ static_cast<int>(recordingConfig) };
    jassert(recordingConfigIndex >= 0 && recordingConfigIndex < RECORDING_CONFIG_STRINGS.size());
    auto const & recordingConfigString{ RECORDING_CONFIG_STRINGS.getReference(recordingConfigIndex) };
    mUserSettings->setValue(Tags::RECORDING_CONFIG, recordingConfigString);
}

//==============================================================================
void Configuration::setLastRecordingDirectory(juce::File const & lastRecordingDirectory) const
{
    mUserSettings->setValue(Tags::LAST_RECORDING_DIRECTORY, lastRecordingDirectory.getFullPathName());
}

//==============================================================================
void Configuration::setOscInputPort(int const oscInputPort) const
{
    mUserSettings->setValue(Tags::OSC_INPUT_PORT, oscInputPort);
}

//==============================================================================
void Configuration::setWindowX(int const windowX) const
{
    mUserSettings->setValue(Tags::WINDOW_X, windowX);
}

//==============================================================================
void Configuration::setWindowY(int const windowY) const
{
    mUserSettings->setValue(Tags::WINDOW_Y, windowY);
}

//==============================================================================
void Configuration::setWindowWidth(int const windowWidth) const
{
    mUserSettings->setValue(Tags::WINDOW_WIDTH, windowWidth);
}

//==============================================================================
void Configuration::setWindowHeight(int const windowHeight) const
{
    mUserSettings->setValue(Tags::WINDOW_HEIGHT, windowHeight);
}

//==============================================================================
void Configuration::setSashPosition(double const sashPosition) const
{
    mUserSettings->setValue(Tags::SASH_POSITION, sashPosition);
}

//==============================================================================
void Configuration::setLastVbapSpeakerSetup(juce::File const & lastVbapSpeakerSetup) const
{
    mUserSettings->setValue(Tags::LAST_VBAP_SPEAKER_SETUP, lastVbapSpeakerSetup.getFullPathName());
}

//==============================================================================
void Configuration::setLastOpenPreset(juce::File const & lastOpenPreset) const
{
    mUserSettings->setValue(Tags::LAST_OPEN_PRESET, lastOpenPreset.getFullPathName());
}

//==============================================================================
void Configuration::setLastOpenSpeakerSetup(juce::File const & lastOpenSpeakerSetup) const
{
    mUserSettings->setValue(Tags::LAST_OPEN_SPEAKER_SETUP, lastOpenSpeakerSetup.getFullPathName());
}

//==============================================================================
void Configuration::setLastPresetDirectory(juce::File const & directory) const
{
    mUserSettings->setValue(Tags::LAST_PRESET_DIRECTORY, directory.getFullPathName());
}

//==============================================================================
void Configuration::setLastSpeakerSetupDirectory(juce::File const & directory) const
{
    mUserSettings->setValue(Tags::LAST_SPEAKER_SETUP_DIRECTORY, directory.getFullPathName());
}

//==============================================================================
juce::File Configuration::getLastVbapSpeakerSetup() const
{
    juce::File lastVbap{ mUserSettings->getValue(Tags::LAST_VBAP_SPEAKER_SETUP) };
    if (!lastVbap.existsAsFile()) {
        return DEFAULT_SPEAKER_SETUP_FILE;
    }
    return lastVbap;
}

//==============================================================================
juce::File Configuration::getLastOpenPreset() const
{
    juce::File lastOpenPreset{ mUserSettings->getValue(Tags::LAST_OPEN_PRESET) };
    if (!lastOpenPreset.existsAsFile()) {
        return DEFAULT_PRESET_FILE;
    }
    return lastOpenPreset;
}

//==============================================================================
juce::File Configuration::getLastOpenSpeakerSetup() const
{
    juce::File lastOpenSpeakerSetup{ mUserSettings->getValue(Tags::LAST_OPEN_SPEAKER_SETUP) };
    if (!lastOpenSpeakerSetup.existsAsFile()) {
        return DEFAULT_SPEAKER_SETUP_FILE;
    }
    return lastOpenSpeakerSetup;
}

//==============================================================================
juce::String Configuration::getDeviceType() const
{
    return mUserSettings->getValue(Tags::DEVICE_TYPE);
}

//==============================================================================
juce::String Configuration::getInputDevice() const
{
    return mUserSettings->getValue(Tags::INPUT_DEVICE);
}

//==============================================================================
juce::String Configuration::getOutputDevice() const
{
    return mUserSettings->getValue(Tags::OUTPUT_DEVICE);
}

//==============================================================================
double Configuration::getSampleRate() const
{
    return mUserSettings->getDoubleValue(Tags::SAMPLE_RATE, 48000.0);
}

//==============================================================================
int Configuration::getBufferSize() const
{
    return mUserSettings->getIntValue(Tags::BUFFER_SIZE, 512);
}

//==============================================================================
int Configuration::getAttenuationDbIndex() const
{
    auto const string{ mUserSettings->getValue(Tags::ATTENUATION_DB) };
    auto const index{ ATTENUATION_DB_STRINGS.indexOf(string) };
    if (index < 0) {
        return 3; // -36 db
    }
    return index;
}

//==============================================================================
int Configuration::getAttenuationFrequencyIndex() const
{
    auto const string{ mUserSettings->getValue(Tags::ATTENUATION_HZ) };
    auto const index{ ATTENUATION_FREQUENCY_STRINGS.indexOf(string) };
    if (index < 0) {
        return 3; // 1000 Hz
    }
    return index;
}

//==============================================================================
std::optional<double> Configuration::getSashPosition() const
{
    if (!mUserSettings->containsKey(Tags::SASH_POSITION)) {
        return std::nullopt;
    }
    return mUserSettings->getDoubleValue(Tags::SASH_POSITION);
}

//==============================================================================
RecordingFormat Configuration::getRecordingFormat() const
{
    auto const recordingFormatString{ mUserSettings->getValue(Tags::RECORDING_FORMAT) };

    for (int index{}; index < RECORDING_FORMAT_STRINGS.size(); ++index) {
        if (RECORDING_FORMAT_STRINGS[index] == recordingFormatString) {
            return static_cast<RecordingFormat>(index);
        }
    }
    return {};
}

//==============================================================================
RecordingConfig Configuration::getRecordingConfig() const
{
    auto const recordingConfigString{ mUserSettings->getValue(Tags::RECORDING_CONFIG) };

    for (int index{}; index < RECORDING_CONFIG_STRINGS.size(); ++index) {
        if (RECORDING_CONFIG_STRINGS[index] == recordingConfigString) {
            return static_cast<RecordingConfig>(index);
        }
    }
    return {};
}

//==============================================================================
juce::File Configuration::getLastRecordingDirectory() const
{
    juce::File file{ mUserSettings->getValue(Tags::LAST_RECORDING_DIRECTORY) };
    if (!file.isDirectory()) {
        return juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    }
    return file;
}

//==============================================================================
int Configuration::getOscInputPort() const
{
    return mUserSettings->getIntValue(Tags::OSC_INPUT_PORT, DEFAULT_OSC_INPUT_PORT);
}

//==============================================================================
int Configuration::getWindowX() const
{
    return mUserSettings->getIntValue(Tags::WINDOW_X, -1);
}

//==============================================================================
int Configuration::getWindowY() const
{
    return mUserSettings->getIntValue(Tags::WINDOW_Y, -1);
}

//==============================================================================
int Configuration::getWindowWidth() const
{
    return mUserSettings->getIntValue(Tags::WINDOW_WIDTH, 0);
}

//==============================================================================
int Configuration::getWindowHeight() const
{
    return mUserSettings->getIntValue(Tags::WINDOW_HEIGHT, 0);
}

//==============================================================================
juce::File Configuration::getLastPresetDirectory() const
{
    juce::File file{ mUserSettings->getValue(Tags::LAST_PRESET_DIRECTORY) };
    if (!file.isDirectory()) {
        return juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    }
    return file;
}

//==============================================================================
juce::File Configuration::getLastSpeakerSetupDirectory() const
{
    juce::File file{ mUserSettings->getValue(Tags::LAST_SPEAKER_SETUP_DIRECTORY) };
    if (!file.isDirectory()) {
        return juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    }
    return file;
}
