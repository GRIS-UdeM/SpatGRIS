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

#include "sg_LogicStrucs.hpp"

#include "sg_LegacySpatFileFormat.hpp"

juce::String const ColdSourceData::XmlTags::STATE = "STATE";
juce::String const ColdSourceData::XmlTags::DIRECT_OUT = "DIRECT_OUT";
juce::String const ColdSourceData::XmlTags::COLOUR = "COLOR";
juce::String const ColdSourceData::XmlTags::HYBRID_SPAT_MODE = "HYBRID_SPAT_MODE";
juce::String const ColdSourceData::XmlTags::MAIN_TAG_PREFIX = "SOURCE_";

juce::String const ColdSpeakerHighpassData::XmlTags::MAIN_TAG = "HIGHPASS";
juce::String const ColdSpeakerHighpassData::XmlTags::FREQ = "FREQ";

juce::String const ColdSpeakerData::XmlTags::STATE = "STATE";
juce::String const ColdSpeakerData::XmlTags::GAIN = "GAIN";
juce::String const ColdSpeakerData::XmlTags::IS_DIRECT_OUT_ONLY = "DIRECT_OUT_ONLY";
juce::String const ColdSpeakerData::XmlTags::MAIN_TAG_PREFIX = "SPEAKER_";

juce::String const ColdLbapDistanceAttenuationData::XmlTags::MAIN_TAG = "LBAP_SETTINGS";
juce::String const ColdLbapDistanceAttenuationData::XmlTags::FREQ = "FREQ";
juce::String const ColdLbapDistanceAttenuationData::XmlTags::ATTENUATION = "ATTENUATION";

juce::String const ColdAudioSettings::XmlTags::MAIN_TAG = "AUDIO_SETTINGS";
juce::String const ColdAudioSettings::XmlTags::INTERFACE_TYPE = "INTERFACE_TYPE";
juce::String const ColdAudioSettings::XmlTags::INPUT_INTERFACE = "INPUT_INTERFACE";
juce::String const ColdAudioSettings::XmlTags::OUTPUT_INTERFACE = "OUTPUT_INTERFACE";
juce::String const ColdAudioSettings::XmlTags::SAMPLE_RATE = "SAMPLE_RATE";
juce::String const ColdAudioSettings::XmlTags::BUFFER_SIZE = "BUFFER_SIZE";

juce::String const ColdRecordingOptions::XmlTags::MAIN_TAG = "RECORDING_OPTIONS";
juce::String const ColdRecordingOptions::XmlTags::FORMAT = "FORMAT";
juce::String const ColdRecordingOptions::XmlTags::FILE_TYPE = "FILE_TYPE";

juce::String const ColdStereoRouting::XmlTags::MAIN_TAG = "STEREO_ROUTING";
juce::String const ColdStereoRouting::XmlTags::LEFT = "LEFT";
juce::String const ColdStereoRouting::XmlTags::RIGHT = "RIGHT";

juce::String const ColdViewSettings::XmlTags::MAIN_TAG = "VIEW_SETTINGS";
juce::String const ColdViewSettings::XmlTags::SHOW_SPEAKERS = "SHOW_SPEAKERS";
juce::String const ColdViewSettings::XmlTags::SHOW_SOURCE_NUMBERS = "SHOW_SOURCE_NUMBERS";
juce::String const ColdViewSettings::XmlTags::SHOW_SPEAKER_NUMBERS = "SHOW_SPEAKER_NUMBERS";
juce::String const ColdViewSettings::XmlTags::SHOW_SPEAKER_TRIPLETS = "SHOW_SPEAKER_TRIPLETS";
juce::String const ColdViewSettings::XmlTags::SHOW_SPEAKER_LEVELS = "SHOW_SPEAKER_LEVELS";
juce::String const ColdViewSettings::XmlTags::SHOW_SPHERE_OR_CUBE = "SHOW_SPHERE_OR_CUBE";
juce::String const ColdViewSettings::XmlTags::SHOW_SOURCE_ACTIVITY = "SHOW_SOURCE_ACTIVITY";

juce::String const ColdSpatGrisProjectData::XmlTags::MAIN_TAG = "SPAT_GRIS_PROJECT_DATA";
juce::String const ColdSpatGrisProjectData::XmlTags::VERSION = "VERSION";
juce::String const ColdSpatGrisProjectData::XmlTags::SOURCES = "SOURCES";
juce::String const ColdSpatGrisProjectData::XmlTags::MASTER_GAIN = "MASTER_GAIN";
juce::String const ColdSpatGrisProjectData::XmlTags::GAIN_INTERPOLATION = "GAIN_INTERPOLATION";
juce::String const ColdSpatGrisProjectData::XmlTags::OSC_PORT = "OSC_PORT";

juce::String const ColdSpatGrisAppData::XmlTags::MAIN_TAG = "SPAT_GRIS_APP_DATA";
juce::String const ColdSpatGrisAppData::XmlTags::LAST_SPEAKER_SETUP = "LAST_SPEAKER_SETUP";
juce::String const ColdSpatGrisAppData::XmlTags::LAST_PROJECT = "LAST_PROJECT";
juce::String const ColdSpatGrisAppData::XmlTags::LAST_RECORDING_DIRECTORY = "LAST_RECORDING_DIRECTORY";
juce::String const ColdSpatGrisAppData::XmlTags::LAST_STEREO_MODE = "LAST_STEREO_MODE";
juce::String const ColdSpatGrisAppData::XmlTags::WINDOW_X = "WINDOW_X";
juce::String const ColdSpatGrisAppData::XmlTags::WINDOW_Y = "WINDOW_Y";
juce::String const ColdSpatGrisAppData::XmlTags::WINDOW_WIDTH = "WINDOW_WIDTH";
juce::String const ColdSpatGrisAppData::XmlTags::WINDOW_HEIGHT = "WINDOW_HEIGHT";
juce::String const ColdSpatGrisAppData::XmlTags::SASH_POSITION = "SASH_POSITION";
juce::String const ColdSpatGrisAppData::XmlTags::CAMERA = "CAMERA";

juce::String const ColdSpeakerSetup::XmlTags::MAIN_TAG = "SPEAKER_SETUP";
juce::String const ColdSpeakerSetup::XmlTags::VERSION = "VERSION";
juce::String const ColdSpeakerSetup::XmlTags::SPAT_MODE = "SPAT_MODE";

//==============================================================================
juce::String sliceStateToString(SliceState const state)
{
    switch (state) {
    case SliceState::muted:
        return "muted";
    case SliceState::solo:
        return "solo";
    case SliceState::normal:
        return "normal";
    }
    jassertfalse;
    return "";
}

//==============================================================================
tl::optional<SliceState> stringToSliceState(juce::String const & string)
{
    if (string == "muted") {
        return SliceState::muted;
    }
    if (string == "solo") {
        return SliceState::solo;
    }
    if (string == "normal") {
        return SliceState::normal;
    }
    return tl::nullopt;
}

//==============================================================================
SourceAudioConfig ColdSourceData::toConfig(bool const soloMode) const
{
    SourceAudioConfig result;
    result.directOut = directOut;
    result.isMuted = soloMode ? state != SliceState::solo : state == SliceState::muted;
    return result;
}

//==============================================================================
HotViewportSourceData ColdSourceData::toViewportData(float const alpha) const
{
    jassert(position);

    HotViewportSourceData result;
    result.colour = colour.withAlpha(alpha);
    result.azimuthSpan = azimuthSpan;
    result.zenithSpan = zenithSpan;
    result.position = *position;
    result.hybridSpatMode = hybridSpatMode;

    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdSourceData::toXml(source_index_t const index) const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG_PREFIX + juce::String{ index.get() }) };

    result->setAttribute(XmlTags::STATE, sliceStateToString(state));
    if (directOut) {
        result->setAttribute(XmlTags::DIRECT_OUT, directOut->get());
    }
    result->setAttribute(XmlTags::COLOUR, juce::String{ colour.getARGB() });

    result->setAttribute(XmlTags::HYBRID_SPAT_MODE, spatModeToString(hybridSpatMode));

    return result;
}

//==============================================================================
tl::optional<ColdSourceData> ColdSourceData::fromXml(juce::XmlElement const & xml)
{
    if (!xml.getTagName().startsWith(XmlTags::MAIN_TAG_PREFIX)) {
        return tl::nullopt;
    }

    juce::StringArray const requiredTags{ XmlTags::STATE, XmlTags::COLOUR };

    if (!std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & tag) {
            return xml.hasAttribute(tag);
        })) {
        return tl::nullopt;
    }

    auto const state{ stringToSliceState(xml.getStringAttribute(XmlTags::STATE)) };

    if (!state) {
        return tl::nullopt;
    }

    ColdSourceData result{};
    result.state = *state;
    if (xml.hasAttribute(XmlTags::DIRECT_OUT)) {
        result.directOut = output_patch_t{ xml.getIntAttribute(XmlTags::DIRECT_OUT) };
    }

    if (auto const maybeHybridSpatMode{ stringToSpatMode(xml.getStringAttribute(XmlTags::HYBRID_SPAT_MODE)) }) {
        result.hybridSpatMode = *maybeHybridSpatMode;
    }

    result.colour = juce::Colour{ narrow<uint32_t>(xml.getStringAttribute(XmlTags::COLOUR).getLargeIntValue()) };

    return result;
}

//==============================================================================
bool ColdSourceData::operator==(ColdSourceData const & other) const noexcept
{
    // TODO : this is very misleading and should be replaced with something that does make it seem like we're really
    // comparing the two sources.
    return other.directOut == directOut && other.colour == colour && other.hybridSpatMode == hybridSpatMode;
}

//==============================================================================
SpeakerHighpassConfig ColdSpeakerHighpassData::toConfig(double const sampleRate) const
{
    auto const f{ narrow<double>(freq.get()) };
    auto const wc{ 2.0 * juce::MathConstants<double>::pi * f };
    auto const wc2{ wc * wc };
    auto const wc3{ wc2 * wc };
    auto const wc4{ wc2 * wc2 };
    auto const k{ wc / std::tan(juce::MathConstants<double>::pi * f / sampleRate) };
    auto const k2{ k * k };
    auto const k3{ k2 * k };
    auto const k4{ k2 * k2 };
    auto const sqTmp1{ juce::MathConstants<double>::sqrt2 * wc3 * k };
    auto const sqTmp2{ juce::MathConstants<double>::sqrt2 * wc * k3 };
    auto const aTmp{ 4.0 * wc2 * k2 + 2.0 * sqTmp1 + k4 + 2.0 * sqTmp2 + wc4 };
    auto const k4ATmp{ k4 / aTmp };

    /* common */
    auto const b1{ (4.0 * (wc4 + sqTmp1 - k4 - sqTmp2)) / aTmp };
    auto const b2{ (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / aTmp };
    auto const b3{ (4.0 * (wc4 - sqTmp1 + sqTmp2 - k4)) / aTmp };
    auto const b4{ (k4 - 2.0 * sqTmp1 + wc4 - 2.0 * sqTmp2 + 4.0 * wc2 * k2) / aTmp };

    /* highpass */
    auto const ha0{ k4ATmp };
    auto const ha1{ -4.0 * k4ATmp };
    auto const ha2{ 6.0 * k4ATmp };

    return SpeakerHighpassConfig{ b1, b2, b3, b4, ha0, ha1, ha2 };
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdSpeakerHighpassData::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };
    result->setAttribute(XmlTags::FREQ, freq.get());
    return result;
}

//==============================================================================
tl::optional<ColdSpeakerHighpassData> ColdSpeakerHighpassData::fromXml(juce::XmlElement const & xml)
{
    if (xml.getTagName() != XmlTags::MAIN_TAG || !xml.hasAttribute(XmlTags::FREQ)) {
        return tl::nullopt;
    }

    ColdSpeakerHighpassData result;
    result.freq = hz_t{ static_cast<float>(xml.getDoubleAttribute(XmlTags::FREQ)) };

    return result;
}

//==============================================================================
bool ColdSpeakerHighpassData::operator==(ColdSpeakerHighpassData const & other) const noexcept
{
    return other.freq == freq;
}

//==============================================================================
SpeakerAudioConfig ColdSpeakerData::toConfig(bool const soloMode, double const sampleRate) const noexcept
{
    auto const getHighpassConfig = [&](ColdSpeakerHighpassData const & data) { return data.toConfig(sampleRate); };

    SpeakerAudioConfig result;
    result.isMuted = soloMode ? state != SliceState::solo : state == SliceState::muted;
    result.gain = gain.toGain();
    result.highpassConfig = highpassData.map(getHighpassConfig);
    result.isDirectOutOnly = isDirectOutOnly;
    return result;
}

//==============================================================================
WarmViewportSpeakerData ColdSpeakerData::toViewportConfig() const noexcept
{
    return WarmViewportSpeakerData{ position, isSelected, isDirectOutOnly };
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdSpeakerData::toXml(output_patch_t const outputPatch) const noexcept
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG_PREFIX + juce::String{ outputPatch.get() }) };

    result->setAttribute(XmlTags::STATE, sliceStateToString(state));
    result->addChildElement(position.getCartesian().toXml());
    result->setAttribute(XmlTags::GAIN, gain.get());
    if (highpassData) {
        result->addChildElement(highpassData->toXml().release());
    }
    result->setAttribute(XmlTags::IS_DIRECT_OUT_ONLY, isDirectOutOnly);

    return result;
}

//==============================================================================
tl::optional<ColdSpeakerData> ColdSpeakerData::fromXml(juce::XmlElement const & xml) noexcept
{
    juce::StringArray const requiredTags{ XmlTags::GAIN, XmlTags::IS_DIRECT_OUT_ONLY, XmlTags::STATE };

    auto const * positionElement{ xml.getChildByName(CartesianVector::XmlTags::MAIN_TAG) };

    if (positionElement == nullptr
        || !std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & tag) {
               return xml.hasAttribute(tag);
           })) {
        return tl::nullopt;
    }

    auto const position{ CartesianVector::fromXml(*positionElement) };
    auto const state{ stringToSliceState(xml.getStringAttribute(XmlTags::STATE)) };

    if (!position || !state) {
        return tl::nullopt;
    }

    auto const * crossoverElement{ xml.getChildByName(ColdSpeakerHighpassData::XmlTags::MAIN_TAG) };

    ColdSpeakerData result{};
    result.state = *state;
    result.position = *position;
    result.gain = dbfs_t{ static_cast<float>(xml.getDoubleAttribute(XmlTags::GAIN)) };
    if (crossoverElement) {
        auto const crossover{ ColdSpeakerHighpassData::fromXml(*crossoverElement) };
        if (!crossover) {
            return tl::nullopt;
        }
        result.highpassData = crossover;
    }
    result.isDirectOutOnly = xml.getBoolAttribute(XmlTags::IS_DIRECT_OUT_ONLY);

    return result;
}

//==============================================================================
bool ColdSpeakerData::operator==(ColdSpeakerData const & other) const noexcept
{
    return other.isDirectOutOnly == isDirectOutOnly && other.highpassData == highpassData && other.position == position
           && other.gain == gain;
}

//==============================================================================
LbapAttenuationConfig ColdLbapDistanceAttenuationData::toConfig(double const sampleRate) const
{
    auto const coefficient{ std::exp(-juce::MathConstants<float>::twoPi * freq.get() / narrow<float>(sampleRate)) };
    auto const gain{ attenuation.toGain() };
    LbapAttenuationConfig const result{ gain, coefficient };
    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdLbapDistanceAttenuationData::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::FREQ, freq.get());
    result->setAttribute(XmlTags::ATTENUATION, attenuation.get());

    return result;
}

//==============================================================================
tl::optional<ColdLbapDistanceAttenuationData> ColdLbapDistanceAttenuationData::fromXml(juce::XmlElement const & xml)
{
    if (xml.getTagName() != XmlTags::MAIN_TAG || !xml.hasAttribute(XmlTags::FREQ)
        || !xml.hasAttribute(XmlTags::ATTENUATION)) {
        return tl::nullopt;
    }

    ColdLbapDistanceAttenuationData result{};
    result.freq = hz_t{ static_cast<float>(xml.getDoubleAttribute(XmlTags::FREQ)) };
    result.attenuation = dbfs_t{ static_cast<float>(xml.getDoubleAttribute(XmlTags::ATTENUATION)) };

    return result;
}

//==============================================================================
bool ColdLbapDistanceAttenuationData::operator==(ColdLbapDistanceAttenuationData const & other) const noexcept
{
    return other.attenuation == attenuation && other.freq == freq;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdAudioSettings::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::INTERFACE_TYPE, deviceType);
    result->setAttribute(XmlTags::INPUT_INTERFACE, inputDevice);
    result->setAttribute(XmlTags::OUTPUT_INTERFACE, outputDevice);
    result->setAttribute(XmlTags::SAMPLE_RATE, sampleRate);
    result->setAttribute(XmlTags::BUFFER_SIZE, bufferSize);

    return result;
}

//==============================================================================
tl::optional<ColdAudioSettings> ColdAudioSettings::fromXml(juce::XmlElement const & xml)
{
    juce::StringArray requiredAttributes{ XmlTags::INTERFACE_TYPE,
                                          XmlTags::INPUT_INTERFACE,
                                          XmlTags::OUTPUT_INTERFACE,
                                          XmlTags::SAMPLE_RATE,
                                          XmlTags::BUFFER_SIZE };

    if (xml.getTagName() != XmlTags::MAIN_TAG
        || !std::all_of(requiredAttributes.begin(), requiredAttributes.end(), [&](juce::String const & string) {
               return xml.hasAttribute(string);
           })) {
        return tl::nullopt;
    }

    tl::optional<ColdAudioSettings> result{ ColdAudioSettings{} };
    result->deviceType = xml.getStringAttribute(XmlTags::INTERFACE_TYPE);
    result->inputDevice = xml.getStringAttribute(XmlTags::INPUT_INTERFACE);
    result->outputDevice = xml.getStringAttribute(XmlTags::OUTPUT_INTERFACE);
    result->sampleRate = xml.getDoubleAttribute(XmlTags::SAMPLE_RATE);
    result->bufferSize = xml.getIntAttribute(XmlTags::BUFFER_SIZE);

    return result;
}

//==============================================================================
juce::String recordingFormatToString(RecordingFormat const format)
{
    return RECORDING_FORMAT_STRINGS[static_cast<int>(format)];
}

//==============================================================================
tl::optional<RecordingFormat> stringToRecordingFormat(juce::String const & string)
{
    for (int i{}; i < RECORDING_FORMAT_STRINGS.size(); ++i) {
        if (string == RECORDING_FORMAT_STRINGS[i]) {
            return static_cast<RecordingFormat>(i);
        }
    }

    return tl::nullopt;
}

//==============================================================================
juce::String recordingFileTypeToString(RecordingFileType const fileType)
{
    return RECORDING_FILE_TYPE_STRINGS[static_cast<int>(fileType)];
}

//==============================================================================
tl::optional<RecordingFileType> stringToRecordingFileType(juce::String const & string)
{
    for (int i{}; i < RECORDING_FILE_TYPE_STRINGS.size(); ++i) {
        if (string == RECORDING_FILE_TYPE_STRINGS[i]) {
            return static_cast<RecordingFileType>(i);
        }
    }
    return tl::nullopt;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdRecordingOptions::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::FORMAT, recordingFormatToString(format));
    result->setAttribute(XmlTags::FILE_TYPE, recordingFileTypeToString(fileType));

    return result;
}

//==============================================================================
tl::optional<ColdRecordingOptions> ColdRecordingOptions::fromXml(juce::XmlElement const & xml)
{
    auto const format{ stringToRecordingFormat(xml.getStringAttribute(XmlTags::FORMAT)) };
    auto const fileType{ stringToRecordingFileType(xml.getStringAttribute(XmlTags::FILE_TYPE)) };

    if (!format || !fileType) {
        return tl::nullopt;
    }

    ColdRecordingOptions result;
    result.format = *format;
    result.fileType = *fileType;

    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdStereoRouting::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::LEFT, left.get());
    result->setAttribute(XmlTags::RIGHT, right.get());

    return result;
}

//==============================================================================
tl::optional<ColdStereoRouting> ColdStereoRouting::fromXml(juce::XmlElement const & xml)
{
    if (xml.getTagName() != XmlTags::MAIN_TAG || !xml.hasAttribute(XmlTags::LEFT)
        || !xml.hasAttribute(XmlTags::RIGHT)) {
        return tl::nullopt;
    }

    ColdStereoRouting result;
    result.left = output_patch_t{ xml.getIntAttribute(XmlTags::LEFT, 1) };
    result.right = output_patch_t{ xml.getIntAttribute(XmlTags::RIGHT, 2) };
    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdViewSettings::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::SHOW_SPEAKERS, showSpeakers);
    result->setAttribute(XmlTags::SHOW_SOURCE_NUMBERS, showSourceNumbers);
    result->setAttribute(XmlTags::SHOW_SPEAKER_NUMBERS, showSpeakerNumbers);
    result->setAttribute(XmlTags::SHOW_SPEAKER_TRIPLETS, showSpeakerTriplets);
    result->setAttribute(XmlTags::SHOW_SPEAKER_LEVELS, showSpeakerLevels);
    result->setAttribute(XmlTags::SHOW_SPHERE_OR_CUBE, showSphereOrCube);
    result->setAttribute(XmlTags::SHOW_SOURCE_ACTIVITY, showSourceActivity);

    return result;
}

//==============================================================================
tl::optional<ColdViewSettings> ColdViewSettings::fromXml(juce::XmlElement const & xml)
{
    juce::StringArray const requiredTags{ XmlTags::SHOW_SPEAKERS,        XmlTags::SHOW_SOURCE_NUMBERS,
                                          XmlTags::SHOW_SPEAKER_NUMBERS, XmlTags::SHOW_SPEAKER_TRIPLETS,
                                          XmlTags::SHOW_SPEAKER_LEVELS,  XmlTags::SHOW_SPHERE_OR_CUBE,
                                          XmlTags::SHOW_SOURCE_ACTIVITY };

    if (xml.getTagName() != XmlTags::MAIN_TAG
        || !std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & tag) {
               return xml.hasAttribute(tag);
           })) {
        return tl::nullopt;
    }

    ColdViewSettings result;
    result.showSpeakers = xml.getBoolAttribute(XmlTags::SHOW_SPEAKERS);
    result.showSourceNumbers = xml.getBoolAttribute(XmlTags::SHOW_SOURCE_NUMBERS);
    result.showSpeakerNumbers = xml.getBoolAttribute(XmlTags::SHOW_SPEAKER_NUMBERS);
    result.showSpeakerTriplets = xml.getBoolAttribute(XmlTags::SHOW_SPEAKER_TRIPLETS);
    result.showSpeakerLevels = xml.getBoolAttribute(XmlTags::SHOW_SPEAKER_LEVELS);
    result.showSphereOrCube = xml.getBoolAttribute(XmlTags::SHOW_SPHERE_OR_CUBE);
    result.showSourceActivity = xml.getBoolAttribute(XmlTags::SHOW_SOURCE_ACTIVITY);

    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdSpatGrisProjectData::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    auto sourcesElement{ std::make_unique<juce::XmlElement>(XmlTags::SOURCES) };
    for (auto const sourceData : sources) {
        sourcesElement->addChildElement(sourceData.value->toXml(sourceData.key).release());
    }

    result->addChildElement(sourcesElement.release());
    result->addChildElement(lbapDistanceAttenuationData.toXml().release());

    result->setAttribute(XmlTags::OSC_PORT, oscPort);
    result->setAttribute(XmlTags::MASTER_GAIN, masterGain.get());
    result->setAttribute(XmlTags::GAIN_INTERPOLATION, spatGainsInterpolation);
    result->setAttribute(XmlTags::VERSION, SPAT_GRIS_VERSION.toString());

    return result;
}

//==============================================================================
tl::optional<ColdSpatGrisProjectData> ColdSpatGrisProjectData::fromXml(juce::XmlElement const & xml)
{
    juce::StringArray const requiredTags{ XmlTags::MASTER_GAIN, XmlTags::GAIN_INTERPOLATION, XmlTags::OSC_PORT };
    if (xml.getTagName() != XmlTags::MAIN_TAG
        || !std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & string) {
               return xml.hasAttribute(string);
           })) {
        return readLegacyProjectFile(xml);
    }

    auto const * sourcesElement{ xml.getChildByName(XmlTags::SOURCES) };
    auto const * lbapAttenuationElement{ xml.getChildByName(ColdLbapDistanceAttenuationData::XmlTags::MAIN_TAG) };

    if (!sourcesElement || !lbapAttenuationElement) {
        return tl::nullopt;
    }

    ColdSpatGrisProjectData result{};

    auto const lbapAttenuation{ ColdLbapDistanceAttenuationData::fromXml(*lbapAttenuationElement) };

    if (!lbapAttenuation) {
        return tl::nullopt;
    }

    result.masterGain = LEGAL_MASTER_GAIN_RANGE.clipValue(
        dbfs_t{ static_cast<dbfs_t::type>(xml.getDoubleAttribute(XmlTags::MASTER_GAIN)) });
    result.spatGainsInterpolation = LEGAL_GAIN_INTERPOLATION_RANGE.clipValue(
        static_cast<float>(xml.getDoubleAttribute(XmlTags::GAIN_INTERPOLATION)));
    result.oscPort = xml.getIntAttribute(XmlTags::OSC_PORT); // TODO : validate value
    result.lbapDistanceAttenuationData = *lbapAttenuation;

    for (auto const * sourceElement : sourcesElement->getChildIterator()) {
        jassert(sourceElement);
        auto const sourceData{ ColdSourceData::fromXml(*sourceElement) };
        if (!sourceData) {
            return tl::nullopt;
        }

        auto const sourceIndex{ source_index_t{
            sourceElement->getTagName().substring(ColdSourceData::XmlTags::MAIN_TAG_PREFIX.length()).getIntValue() } };
        jassert(LEGAL_SOURCE_INDEX_RANGE.contains(sourceIndex));
        if (!LEGAL_SOURCE_INDEX_RANGE.contains(sourceIndex)) {
            return tl::nullopt;
        }
        result.sources.add(sourceIndex, std::make_unique<ColdSourceData>(*sourceData));
    }

    return tl::make_optional(std::move(result));
}

//==============================================================================
bool ColdSpatGrisProjectData::operator==(ColdSpatGrisProjectData const & other) const noexcept
{
    return other.spatGainsInterpolation == spatGainsInterpolation && other.oscPort == oscPort
           && other.masterGain == masterGain && other.lbapDistanceAttenuationData == lbapDistanceAttenuationData
           && other.sources == sources;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdSpatGrisAppData::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    auto cameraElement{ std::make_unique<juce::XmlElement>(XmlTags::CAMERA) };
    cameraElement->addChildElement(cameraPosition.toXml());

    result->addChildElement(audioSettings.toXml().release());
    result->addChildElement(recordingOptions.toXml().release());
    result->addChildElement(cameraElement.release());
    result->addChildElement(viewSettings.toXml().release());
    result->addChildElement(stereoRouting.toXml().release());

    result->setAttribute(XmlTags::LAST_SPEAKER_SETUP, lastSpeakerSetup);
    result->setAttribute(XmlTags::LAST_PROJECT, lastProject);
    result->setAttribute(XmlTags::LAST_RECORDING_DIRECTORY, lastRecordingDirectory);
    result->setAttribute(XmlTags::WINDOW_X, windowX);
    result->setAttribute(XmlTags::WINDOW_Y, windowY);
    result->setAttribute(XmlTags::WINDOW_WIDTH, windowWidth);
    result->setAttribute(XmlTags::WINDOW_HEIGHT, windowHeight);
    result->setAttribute(XmlTags::SASH_POSITION, sashPosition);

    if (stereoMode) {
        result->setAttribute(XmlTags::LAST_STEREO_MODE, stereoModeToString(*stereoMode));
    }

    return result;
}

//==============================================================================
tl::optional<ColdSpatGrisAppData> ColdSpatGrisAppData::fromXml(juce::XmlElement const & xml)
{
    juce::StringArray const requiredTags{
        XmlTags::LAST_SPEAKER_SETUP, XmlTags::LAST_PROJECT, XmlTags::LAST_RECORDING_DIRECTORY,
        XmlTags::WINDOW_X,           XmlTags::WINDOW_Y,     XmlTags::WINDOW_WIDTH,
        XmlTags::WINDOW_HEIGHT,      XmlTags::SASH_POSITION
    };

    auto const * audioSettingsElement{ xml.getChildByName(ColdAudioSettings::XmlTags::MAIN_TAG) };
    auto const * recordingOptionsElement{ xml.getChildByName(ColdRecordingOptions::XmlTags::MAIN_TAG) };
    auto const * cameraElement{ xml.getChildByName(XmlTags::CAMERA) };
    auto const * viewSettingsElement{ xml.getChildByName(ColdViewSettings::XmlTags::MAIN_TAG) };
    auto const * stereoRoutingElement{ xml.getChildByName(ColdStereoRouting::XmlTags::MAIN_TAG) };

    if (xml.getTagName() != XmlTags::MAIN_TAG || !audioSettingsElement || !recordingOptionsElement || !cameraElement
        || !viewSettingsElement || !stereoRoutingElement
        || !std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & tag) {
               return xml.hasAttribute(tag);
           })) {
        return tl::nullopt;
    }

    auto const * cameraPositionElement{ cameraElement->getChildByName(CartesianVector::XmlTags::MAIN_TAG) };
    if (!cameraPositionElement) {
        return tl::nullopt;
    }

    auto const audioSettings{ ColdAudioSettings::fromXml(*audioSettingsElement) };
    auto const recordingOptions{ ColdRecordingOptions::fromXml(*recordingOptionsElement) };
    auto const cameraPosition{ CartesianVector::fromXml(*cameraPositionElement) };
    auto const viewSettings{ ColdViewSettings::fromXml(*viewSettingsElement) };
    auto const lastStereoMode{ stringToStereoMode(xml.getStringAttribute(XmlTags::LAST_STEREO_MODE)) };
    auto const stereoRouting{ ColdStereoRouting::fromXml(*stereoRoutingElement) };

    if (!audioSettings || !recordingOptions || !cameraPosition || !viewSettings || !stereoRouting) {
        return tl::nullopt;
    }

    ColdSpatGrisAppData result;

    result.audioSettings = *audioSettings;
    result.recordingOptions = *recordingOptions;
    result.stereoMode = lastStereoMode;
    result.stereoRouting = *stereoRouting;

    result.lastSpeakerSetup = xml.getStringAttribute(XmlTags::LAST_SPEAKER_SETUP);
    result.lastProject = xml.getStringAttribute(XmlTags::LAST_PROJECT);
    result.lastRecordingDirectory = xml.getStringAttribute(XmlTags::LAST_RECORDING_DIRECTORY);
    result.windowX = xml.getIntAttribute(XmlTags::WINDOW_X);
    result.windowY = xml.getIntAttribute(XmlTags::WINDOW_Y);
    result.windowWidth = xml.getIntAttribute(XmlTags::WINDOW_WIDTH);
    result.windowHeight = xml.getIntAttribute(XmlTags::WINDOW_HEIGHT);
    result.sashPosition = xml.getDoubleAttribute(XmlTags::SASH_POSITION);
    result.viewSettings = *viewSettings;
    result.cameraPosition = *cameraPosition;

    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ColdSpeakerSetup::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::VERSION, SPAT_GRIS_VERSION.toString());
    result->setAttribute(XmlTags::SPAT_MODE, spatModeToString(spatMode));

    jassert(ordering.size() == speakers.size());
    for (auto const outputPatch : ordering) {
        result->addChildElement(speakers[outputPatch].toXml(outputPatch).release());
    }

    return result;
}

//==============================================================================
tl::optional<ColdSpeakerSetup> ColdSpeakerSetup::fromXml(juce::XmlElement const & xml)
{
    auto const spatMode{ stringToSpatMode(xml.getStringAttribute(XmlTags::SPAT_MODE)) };

    if (xml.getTagName() != XmlTags::MAIN_TAG || !spatMode) {
        return readLegacySpeakerSetup(xml);
    }

    tl::optional<ColdSpeakerSetup> result{ ColdSpeakerSetup{} };
    result->spatMode = *spatMode;

    for (auto const * speaker : xml.getChildIterator()) {
        auto const tagName{ speaker->getTagName() };
        if (!tagName.startsWith(ColdSpeakerData::XmlTags::MAIN_TAG_PREFIX)) {
            return tl::nullopt;
        }
        output_patch_t const outputPatch{
            tagName.substring(ColdSpeakerData::XmlTags::MAIN_TAG_PREFIX.length()).getIntValue()
        };
        result->ordering.add(outputPatch);
        auto const speakerData{ ColdSpeakerData::fromXml(*speaker) };
        if (!speakerData) {
            return tl::nullopt;
        }

        result->speakers.add(outputPatch, std::make_unique<ColdSpeakerData>(*speakerData));
    }

    return result;
}

//==============================================================================
bool ColdSpeakerSetup::operator==(ColdSpeakerSetup const & other) const noexcept
{
    return other.ordering == ordering && other.speakers == speakers && other.spatMode == spatMode;
}

//==============================================================================
bool ColdSpeakerSetup::isDomeLike() const noexcept
{
    return std::all_of(speakers.cbegin(), speakers.cend(), [](ColdSpeakersData::ConstNode const & node) {
        return node.value->isDirectOutOnly || juce::isWithin(node.value->position.getPolar().length, 1.0f, 0.02f);
    });
}

//==============================================================================
SpeakersAudioConfig ColdSpeakerSetup::toAudioConfig(double const sampleRate) const noexcept
{
    SpeakersAudioConfig result{};

    auto const isAtLeastOnSpeakerSolo{ std::any_of(speakers.cbegin(), speakers.cend(), [](auto const node) {
        return node.value->state == SliceState::solo;
    }) };

    for (auto const speaker : speakers) {
        result.add(speaker.key, speaker.value->toConfig(isAtLeastOnSpeakerSolo, sampleRate));
    }

    return result;
}

//==============================================================================
int ColdSpeakerSetup::numOfSpatializedSpeakers() const noexcept
{
    auto const result{ std::count_if(
        speakers.cbegin(),
        speakers.cend(),
        [](ColdSpeakersData::ConstNode const & speaker) { return !speaker.value->isDirectOutOnly; }) };

    return narrow<int>(result);
}

//==============================================================================
std::unique_ptr<AudioConfig> ColdSpatGrisData::toAudioConfig() const
{
    auto result{ std::make_unique<AudioConfig>() };

    result->speakersAudioConfig = speakerSetup.toAudioConfig(appData.audioSettings.sampleRate);

    auto const isAtLeastOneSourceSolo{ std::any_of(
        project.sources.cbegin(),
        project.sources.cend(),
        [](auto const node) { return node.value->state == SliceState::solo; }) };

    auto const isValidDirectOut = [&](ColdSourceData const & source) {
        if (!source.directOut) {
            return false;
        }

        if (source.state == SliceState::muted) {
            return false;
        }

        if (isAtLeastOneSourceSolo && source.state != SliceState::solo) {
            return false;
        }

        if (!speakerSetup.speakers.contains(*source.directOut)) {
            return false;
        }

        return true;
    };

    for (auto const & source : project.sources) {
        if (isValidDirectOut(*source.value)) {
            result->directOutPairs.add(std::make_pair(source.key, *source.value->directOut));
        }
    }

    result->lbapAttenuationConfig = project.lbapDistanceAttenuationData.toConfig(appData.audioSettings.sampleRate);
    result->masterGain = project.masterGain.toGain();
    result->pinkNoiseGain = pinkNoiseLevel.map([](auto const & level) { return level.toGain(); });
    for (auto const source : project.sources) {
        result->sourcesAudioConfig.add(source.key, source.value->toConfig(isAtLeastOneSourceSolo));
    }
    result->spatMode = speakerSetup.spatMode;
    result->spatGainsInterpolation = project.spatGainsInterpolation;

    if (appData.stereoMode) {
        result->isStereo = appData.stereoMode.has_value();

        // direct outs are ignored in stereo mode
        result->directOutPairs.clearQuick();

        for (auto & source : result->sourcesAudioConfig) {
            source.value.directOut.reset();
        }
    }

    return result;
}

//==============================================================================
WarmViewportData ColdSpatGrisData::toViewportConfig() const noexcept
{
    WarmViewportData result{};
    for (auto const & speaker : speakerSetup.speakers) {
        result.speakers.add(speaker.key, speaker.value->toViewportConfig());
    }
    result.spatMode = speakerSetup.spatMode;
    result.viewSettings = appData.viewSettings;
    result.title = juce::File{ appData.lastSpeakerSetup }.getFileNameWithoutExtension();

    return result;
}
