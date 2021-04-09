/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Configuration.h"

juce::String const Configuration::XmlTags::MAIN_TAG = "SpatGRIS app data";

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
    mUserSettings->saveIfNeeded();
    mApplicationProperties.saveIfNeeded();
    mApplicationProperties.closeFiles();
}

//==============================================================================
void Configuration::save(SpatGrisAppData const & appData) const
{
    mUserSettings->clear();
    mUserSettings->setValue(XmlTags::MAIN_TAG, appData.toXml());
}

//==============================================================================
SpatGrisAppData Configuration::load() const
{
    auto const appDataElement{ mUserSettings->getXmlValue(XmlTags::MAIN_TAG) };

    if (appDataElement) {
        auto const appData{ SpatGrisAppData::fromXml(*appDataElement) };
        if (appData) {
            return *appData;
        }
    }
    return SpatGrisAppData{};
}
