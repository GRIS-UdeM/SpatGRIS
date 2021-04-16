#include "LogicStrucs.hpp"

juce::String const SourceData::XmlTags::STATE = "STATE";
juce::String const SourceData::XmlTags::DIRECT_OUT = "DIRECT_OUT";
juce::String const SourceData::XmlTags::COLOUR = "COLOR";

juce::String const SpeakerHighpassData::XmlTags::MAIN_TAG = "HIGHPASS";
juce::String const SpeakerHighpassData::XmlTags::FREQ = "FREQ";

juce::String const SpeakerData::XmlTags::STATE = "STATE";
juce::String const SpeakerData::XmlTags::GAIN = "GAIN";
juce::String const SpeakerData::XmlTags::IS_DIRECT_OUT_ONLY = "DIRECT_OUT_ONLY";

juce::String const LbapDistanceAttenuationData::XmlTags::MAIN_TAG = "LBAP_SETTINGS";
juce::String const LbapDistanceAttenuationData::XmlTags::FREQ = "FREQ";
juce::String const LbapDistanceAttenuationData::XmlTags::ATTENUATION = "ATTENUATION";

juce::String const AudioSettings::XmlTags::MAIN_TAG = "AUDIO_SETTINGS";
juce::String const AudioSettings::XmlTags::INTERFACE_TYPE = "INTERFACE_TYPE";
juce::String const AudioSettings::XmlTags::INPUT_INTERFACE = "INPUT_INTERFACE";
juce::String const AudioSettings::XmlTags::OUTPUT_INTERFACE = "OUTPUT_INTERFACE";
juce::String const AudioSettings::XmlTags::SAMPLE_RATE = "SAMPLE_RATE";
juce::String const AudioSettings::XmlTags::BUFFER_SIZE = "BUFFER_SIZE";

juce::String const RecordingOptions::XmlTags::MAIN_TAG = "RECORDING_OPTIONS";
juce::String const RecordingOptions::XmlTags::FORMAT = "FORMAT";
juce::String const RecordingOptions::XmlTags::FILE_TYPE = "FILE_TYPE";

juce::String const ViewSettings::XmlTags::MAIN_TAG = "VIEW_SETTINGS";
juce::String const ViewSettings::XmlTags::SHOW_SPEAKERS = "SHOW_SPEAKERS";
juce::String const ViewSettings::XmlTags::SHOW_SPEAKER_NUMBERS = "SHOW_SPEAKER_NUMBERS";
juce::String const ViewSettings::XmlTags::SHOW_SPEAKER_TRIPLETS = "SHOW_SPEAKER_TRIPLETS";
juce::String const ViewSettings::XmlTags::SHOW_SPEAKER_LEVELS = "SHOW_SPEAKER_LEVELS";
juce::String const ViewSettings::XmlTags::SHOW_SPHERE_OR_CUBE = "SHOW_SPHERE_OR_CUBE";
juce::String const ViewSettings::XmlTags::SHOW_SOURCE_ACTIVITY = "SHOW_SOURCE_ACTIVITY";

juce::String const SpatGrisProjectData::XmlTags::MAIN_TAG = "SPAT_GRIS_PROJECT_DATA";
juce::String const SpatGrisProjectData::XmlTags::SOURCES = "SOURCES";
juce::String const SpatGrisProjectData::XmlTags::MASTER_GAIN = "MASTER_GAIN";
juce::String const SpatGrisProjectData::XmlTags::GAIN_INTERPOLATION = "GAIN_INTERPOLATION";
juce::String const SpatGrisProjectData::XmlTags::OSC_PORT = "OSC_PORT";
juce::String const SpatGrisProjectData::XmlTags::CAMERA = "CAMERA";

juce::String const SpatGrisAppData::XmlTags::MAIN_TAG = "SPAT_GRIS_APP_DATA";
juce::String const SpatGrisAppData::XmlTags::LAST_SPEAKER_SETUP = "LAST_SPEAKER_SETUP";
juce::String const SpatGrisAppData::XmlTags::LAST_PROJECT = "LAST_PROJECT";
juce::String const SpatGrisAppData::XmlTags::LAST_RECORDING_DIRECTORY = "LAST_RECORDING_DIRECTORY";
juce::String const SpatGrisAppData::XmlTags::LAST_SPAT_MODE = "LAST_SPAT_MODE";
juce::String const SpatGrisAppData::XmlTags::WINDOW_X = "WINDOW_X";
juce::String const SpatGrisAppData::XmlTags::WINDOW_Y = "WINDOW_Y";
juce::String const SpatGrisAppData::XmlTags::WINDOW_WIDTH = "WINDOW_WIDTH";
juce::String const SpatGrisAppData::XmlTags::WINDOW_HEIGHT = "WINDOW_HEIGHT";
juce::String const SpatGrisAppData::XmlTags::SASH_POSITION = "SASH_POSITION";

juce::String const SpeakerSetup::XmlTags::MAIN_TAG = "SPEAKER_SETUP";
juce::String const SpeakerSetup::XmlTags::SPAT_MODE = "SPAT_MODE";

//==============================================================================
juce::String portStateToString(PortState const state)
{
    switch (state) {
    case PortState::muted:
        return "muted";
    case PortState::solo:
        return "solo";
    case PortState::normal:
        return "normal";
    }
    jassertfalse;
    return "";
}

//==============================================================================
tl::optional<PortState> stringToPortState(juce::String const & string)
{
    if (string == "muted") {
        return PortState::muted;
    }
    if (string == "solo") {
        return PortState::solo;
    }
    if (string == "normal") {
        return PortState::normal;
    }
    return tl::nullopt;
}

//==============================================================================
SourceAudioConfig SourceData::toConfig(bool const soloMode) const
{
    SourceAudioConfig result;
    result.directOut = directOut;
    result.isMuted = soloMode ? state == PortState::solo : state != PortState::muted;
    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> SourceData::toXml(source_index_t const index) const
{
    auto result{ std::make_unique<juce::XmlElement>(juce::String{ index.get() }) };

    result->setAttribute(XmlTags::STATE, portStateToString(state));
    if (directOut) {
        result->setAttribute(XmlTags::DIRECT_OUT, directOut->get());
    }
    result->setAttribute(XmlTags::COLOUR, juce::String{ colour.getARGB() });

    return result;
}

//==============================================================================
tl::optional<SourceData> SourceData::fromXml(juce::XmlElement const & xml)
{
    juce::StringArray const requiredTags{ XmlTags::STATE, XmlTags::COLOUR };

    auto const * positionElement{ xml.getChildByName(CartesianVector::XmlTags::MAIN_TAG) };

    if (positionElement == nullptr
        || !std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & tag) {
               return xml.hasAttribute(tag);
           })) {
        return tl::nullopt;
    }

    auto const state{ stringToPortState(xml.getStringAttribute(XmlTags::STATE)) };
    auto const position{ CartesianVector::fromXml(*positionElement) };

    if (!state || !position) {
        return tl::nullopt;
    }

    SourceData result{};
    result.state = *state;
    result.position = *position;
    result.vector = PolarVector::fromCartesian(*position);
    if (xml.hasAttribute(XmlTags::DIRECT_OUT)) {
        result.directOut = output_patch_t{ xml.getIntAttribute(XmlTags::DIRECT_OUT) };
    }
    result.colour = juce::Colour{ narrow<uint32_t>(xml.getStringAttribute(XmlTags::COLOUR).getLargeIntValue()) };

    return result;
}

//==============================================================================
SpeakerHighpassConfig SpeakerHighpassData::toConfig(double const sampleRate) const
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
std::unique_ptr<juce::XmlElement> SpeakerHighpassData::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };
    result->setAttribute(XmlTags::FREQ, freq.get());
    return result;
}

//==============================================================================
tl::optional<SpeakerHighpassData> SpeakerHighpassData::fromXml(juce::XmlElement const & xml)
{
    if (xml.getTagName() != XmlTags::MAIN_TAG || !xml.hasAttribute(XmlTags::FREQ)) {
        return tl::nullopt;
    }

    SpeakerHighpassData result;
    result.freq = hz_t{ static_cast<float>(xml.getDoubleAttribute(XmlTags::FREQ)) };

    return result;
}

//==============================================================================
SpeakerAudioConfig SpeakerData::toConfig(bool const soloMode, double const sampleRate) const
{
    auto const getHighpassConfig = [&](SpeakerHighpassData const & data) { return data.toConfig(sampleRate); };

    SpeakerAudioConfig result;
    result.isMuted = soloMode ? state == PortState::solo : state != PortState::muted;
    result.gain = gain.toGain();
    result.highpassConfig = highpassData.map(getHighpassConfig);
    result.isDirectOutOnly = isDirectOutOnly;
    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> SpeakerData::toXml(output_patch_t const outputPatch) const
{
    auto result{ std::make_unique<juce::XmlElement>(juce::String{ outputPatch.get() }) };

    result->setAttribute(XmlTags::STATE, portStateToString(state));
    result->addChildElement(position.toXml());
    result->setAttribute(XmlTags::GAIN, gain.get());
    if (highpassData) {
        result->addChildElement(highpassData->toXml().release());
    }
    result->setAttribute(XmlTags::IS_DIRECT_OUT_ONLY, isDirectOutOnly);

    return result;
}

//==============================================================================
tl::optional<SpeakerData> SpeakerData::fromXml(juce::XmlElement const & xml)
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
    auto const state{ stringToPortState(xml.getStringAttribute(XmlTags::STATE)) };

    if (!position || !state) {
        return tl::nullopt;
    }

    auto const * crossoverElement{ xml.getChildByName(SpeakerHighpassData::XmlTags::MAIN_TAG) };

    SpeakerData result{};
    result.state = *state;
    result.position = *position;
    result.vector = PolarVector::fromCartesian(*position);
    result.gain = dbfs_t{ static_cast<float>(xml.getDoubleAttribute(XmlTags::GAIN)) };
    if (crossoverElement) {
        auto const crossover{ SpeakerHighpassData::fromXml(*crossoverElement) };
        if (!crossover) {
            return tl::nullopt;
        }
        result.highpassData = crossover;
    }
    result.isDirectOutOnly = xml.getBoolAttribute(XmlTags::IS_DIRECT_OUT_ONLY);

    return result;
}

//==============================================================================
LbapAttenuationConfig LbapDistanceAttenuationData::toConfig(double const sampleRate) const
{
    auto const coefficient{ std::exp(-juce::MathConstants<float>::twoPi * freq.get() / narrow<float>(sampleRate)) };
    auto const gain{ attenuation.toGain() };
    LbapAttenuationConfig const result{ gain, coefficient };
    return result;
}

std::unique_ptr<juce::XmlElement> LbapDistanceAttenuationData::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::FREQ, freq.get());
    result->setAttribute(XmlTags::ATTENUATION, attenuation.get());

    return result;
}

//==============================================================================
tl::optional<LbapDistanceAttenuationData> LbapDistanceAttenuationData::fromXml(juce::XmlElement const & xml)
{
    if (xml.getTagName() != XmlTags::MAIN_TAG || !xml.hasAttribute(XmlTags::FREQ)
        || !xml.hasAttribute(XmlTags::ATTENUATION)) {
        return tl::nullopt;
    }

    LbapDistanceAttenuationData result{};
    result.freq = hz_t{ static_cast<float>(xml.getDoubleAttribute(XmlTags::FREQ)) };
    result.attenuation = dbfs_t{ static_cast<float>(xml.getDoubleAttribute(XmlTags::ATTENUATION)) };

    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> AudioSettings::toXml() const
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
tl::optional<AudioSettings> AudioSettings::fromXml(juce::XmlElement const & xml)
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

    tl::optional<AudioSettings> result{ AudioSettings{} };
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
    switch (format) {
    case RecordingFormat::aiff:
        return "aiff";
    case RecordingFormat::wav:
        return "wav";
    }
    jassertfalse;
    return "";
}

//==============================================================================
tl::optional<RecordingFormat> stringToRecordingFormat(juce::String const & string)
{
    if (string == "aiff") {
        return RecordingFormat::aiff;
    }
    if (string == "wav") {
        return RecordingFormat::wav;
    }
    return tl::nullopt;
}

//==============================================================================
juce::String recordingFileTypeToString(RecordingFileType const fileType)
{
    switch (fileType) {
    case RecordingFileType::interleaved:
        return "interleaved";
    case RecordingFileType::mono:
        return "mono";
    }
    jassertfalse;
    return "";
}

//==============================================================================
tl::optional<RecordingFileType> stringToRecordingFileType(juce::String const & string)
{
    if (string == "interleaved") {
        return RecordingFileType::interleaved;
    }
    if (string == "mono") {
        return RecordingFileType::mono;
    }
    return tl::nullopt;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> RecordingOptions::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::FORMAT, recordingFormatToString(format));
    result->setAttribute(XmlTags::FILE_TYPE, recordingFileTypeToString(fileType));

    return result;
}

//==============================================================================
tl::optional<RecordingOptions> RecordingOptions::fromXml(juce::XmlElement const & xml)
{
    auto const format{ stringToRecordingFormat(xml.getStringAttribute(XmlTags::FORMAT)) };
    auto const fileType{ stringToRecordingFileType(xml.getStringAttribute(XmlTags::FILE_TYPE)) };

    if (!format || !fileType) {
        return tl::nullopt;
    }

    RecordingOptions result;
    result.format = *format;
    result.fileType = *fileType;

    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ViewSettings::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::SHOW_SPEAKERS, showSpeakers);
    result->setAttribute(XmlTags::SHOW_SPEAKER_NUMBERS, showSpeakerNumbers);
    result->setAttribute(XmlTags::SHOW_SPEAKER_TRIPLETS, showSpeakerTriplets);
    result->setAttribute(XmlTags::SHOW_SPEAKER_LEVELS, showSpeakerLevels);
    result->setAttribute(XmlTags::SHOW_SPHERE_OR_CUBE, showSphereOrCube);
    result->setAttribute(XmlTags::SHOW_SOURCE_ACTIVITY, showSourceActivity);

    return result;
}

//==============================================================================
tl::optional<ViewSettings> ViewSettings::fromXml(juce::XmlElement const & xml)
{
    juce::StringArray const requiredTags{ XmlTags::MAIN_TAG,
                                          XmlTags::SHOW_SPEAKERS,
                                          XmlTags::SHOW_SPEAKER_NUMBERS,
                                          XmlTags::SHOW_SPEAKER_TRIPLETS,
                                          XmlTags::SHOW_SPEAKER_LEVELS,
                                          XmlTags::SHOW_SPHERE_OR_CUBE,
                                          XmlTags::SHOW_SOURCE_ACTIVITY };

    if (!std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & tag) {
            return xml.hasAttribute(tag);
        })) {
        return tl::nullopt;
    }

    ViewSettings result;
    result.showSpeakers = xml.getBoolAttribute(XmlTags::SHOW_SPEAKERS);
    result.showSpeakerNumbers = xml.getBoolAttribute(XmlTags::SHOW_SPEAKER_NUMBERS);
    result.showSpeakerTriplets = xml.getBoolAttribute(XmlTags::SHOW_SPEAKER_TRIPLETS);
    result.showSpeakerLevels = xml.getBoolAttribute(XmlTags::SHOW_SPEAKER_LEVELS);
    result.showSphereOrCube = xml.getBoolAttribute(XmlTags::SHOW_SPHERE_OR_CUBE);
    result.showSourceActivity = xml.getBoolAttribute(XmlTags::SHOW_SOURCE_ACTIVITY);

    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> SpatGrisProjectData::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    auto sourcesElement{ std::make_unique<juce::XmlElement>(XmlTags::SOURCES) };
    for (auto const sourceData : sources) {
        sourcesElement->addChildElement(sourceData.value->toXml(sourceData.key).release());
    }
    auto cameraElement{ std::make_unique<juce::XmlElement>(XmlTags::CAMERA) };
    cameraElement->addChildElement(cameraPosition.toXml());

    result->addChildElement(sourcesElement.release());
    result->addChildElement(cameraElement.release());
    result->addChildElement(lbapDistanceAttenuationData.toXml().release());
    result->addChildElement(viewSettings.toXml().release());

    result->setAttribute(XmlTags::OSC_PORT, oscPort);
    result->setAttribute(XmlTags::MASTER_GAIN, masterGain.get());
    result->setAttribute(XmlTags::GAIN_INTERPOLATION, spatGainsInterpolation);

    return result;
}

//==============================================================================
tl::optional<SpatGrisProjectData> SpatGrisProjectData::fromXml(juce::XmlElement const & xml)
{
    juce::StringArray const requiredTags{ XmlTags::MASTER_GAIN, XmlTags::GAIN_INTERPOLATION, XmlTags::OSC_PORT };
    if (!std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & string) {
            return xml.hasAttribute(string);
        })) {
        return tl::nullopt;
    }

    auto const * sourcesElement{ xml.getChildByName(XmlTags::SOURCES) };
    auto const * lbapAttenuationElement{ xml.getChildByName(LbapDistanceAttenuationData::XmlTags::MAIN_TAG) };
    auto const * viewSettingsElement{ xml.getChildByName(ViewSettings::XmlTags::MAIN_TAG) };

    if (!sourcesElement || !lbapAttenuationElement || !viewSettingsElement) {
        return tl::nullopt;
    }

    SpatGrisProjectData result{};
    forEachXmlChildElement(*sourcesElement, sourceElement)
    {
        jassert(sourceElement);
        auto const sourceData{ SourceData::fromXml(*sourceElement) };
        if (!sourceData) {
            return tl::nullopt;
        }
        auto const sourceIndex{ source_index_t{ sourceElement->getTagName().getIntValue() } };
        result.sources.add(sourceIndex, std::make_unique<SourceData>(*sourceData));
    }

    auto const lbapAttenuation{ LbapDistanceAttenuationData::fromXml(*lbapAttenuationElement) };
    auto const viewSettings{ ViewSettings::fromXml(*viewSettingsElement) };

    if (!lbapAttenuation || !viewSettings) {
        return tl::nullopt;
    }

    result.masterGain = dbfs_t{ static_cast<dbfs_t::type>(xml.getDoubleAttribute(XmlTags::MASTER_GAIN)) };
    result.spatGainsInterpolation = static_cast<float>(xml.getDoubleAttribute(XmlTags::GAIN_INTERPOLATION));
    result.oscPort = xml.getIntAttribute(XmlTags::OSC_PORT);
    result.viewSettings = *viewSettings;
    result.lbapDistanceAttenuationData = *lbapAttenuation;

    return tl::make_optional(std::move(result));
}

//==============================================================================
std::unique_ptr<juce::XmlElement> SpatGrisAppData::toXml() const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->addChildElement(audioSettings.toXml().release());
    result->addChildElement(recordingOptions.toXml().release());

    result->setAttribute(XmlTags::LAST_SPEAKER_SETUP, lastSpeakerSetup);
    result->setAttribute(XmlTags::LAST_PROJECT, lastProject);
    result->setAttribute(XmlTags::LAST_RECORDING_DIRECTORY, lastRecordingDirectory);
    result->setAttribute(XmlTags::LAST_SPAT_MODE, spatModeToString(spatMode));
    result->setAttribute(XmlTags::WINDOW_X, windowX);
    result->setAttribute(XmlTags::WINDOW_Y, windowY);
    result->setAttribute(XmlTags::WINDOW_WIDTH, windowWidth);
    result->setAttribute(XmlTags::WINDOW_HEIGHT, windowHeight);
    result->setAttribute(XmlTags::SASH_POSITION, sashPosition);

    return result;
}

//==============================================================================
tl::optional<SpatGrisAppData> SpatGrisAppData::fromXml(juce::XmlElement const & xml)
{
    juce::StringArray const requiredTags{
        XmlTags::LAST_SPEAKER_SETUP, XmlTags::LAST_PROJECT,  XmlTags::LAST_RECORDING_DIRECTORY,
        XmlTags::LAST_SPAT_MODE,     XmlTags::WINDOW_X,      XmlTags::WINDOW_Y,
        XmlTags::WINDOW_WIDTH,       XmlTags::WINDOW_HEIGHT, XmlTags::SASH_POSITION
    };

    auto const * audioSettingsElement{ xml.getChildByName(AudioSettings::XmlTags::MAIN_TAG) };
    auto const * recordingOptionsElement{ xml.getChildByName(RecordingOptions::XmlTags::MAIN_TAG) };

    if (xml.getTagName() != XmlTags::MAIN_TAG || !audioSettingsElement || !recordingOptionsElement
        || !std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & tag) {
               return xml.hasAttribute(tag);
           })) {
        return tl::nullopt;
    }

    auto const audioSettings{ AudioSettings::fromXml(*audioSettingsElement) };
    auto const recordingOptions{ RecordingOptions::fromXml(*recordingOptionsElement) };
    auto const lastSpatMode{ stringToSpatMode(xml.getStringAttribute(XmlTags::LAST_SPAT_MODE)) };

    if (!audioSettings || !recordingOptions || !lastSpatMode) {
        return tl::nullopt;
    }

    SpatGrisAppData result;

    result.audioSettings = *audioSettings;
    result.recordingOptions = *recordingOptions;
    result.spatMode = *lastSpatMode;

    result.lastSpeakerSetup = xml.getStringAttribute(XmlTags::LAST_SPEAKER_SETUP);
    result.lastProject = xml.getStringAttribute(XmlTags::LAST_PROJECT);
    result.lastRecordingDirectory = xml.getStringAttribute(XmlTags::LAST_RECORDING_DIRECTORY);
    result.windowX = xml.getIntAttribute(XmlTags::WINDOW_X);
    result.windowY = xml.getIntAttribute(XmlTags::WINDOW_Y);
    result.windowWidth = xml.getIntAttribute(XmlTags::WINDOW_WIDTH);
    result.windowHeight = xml.getIntAttribute(XmlTags::WINDOW_HEIGHT);
    result.sashPosition = xml.getDoubleAttribute(XmlTags::SASH_POSITION);

    return result;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> SpeakerSetup::toXml(SpatMode const spatMode) const
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

    result->setAttribute(XmlTags::SPAT_MODE, spatModeToString(spatMode));

    jassert(order.size() == speakers.size());
    for (auto const outputPatch : order) {
        result->addChildElement(speakers[outputPatch].toXml(outputPatch).release());
    }

    return result;
}

//==============================================================================
tl::optional<std::pair<SpeakerSetup, SpatMode>> SpeakerSetup::fromXml(juce::XmlElement const & xml)
{
    auto const spatMode{ stringToSpatMode(xml.getStringAttribute(XmlTags::SPAT_MODE)) };

    if (xml.getTagName() != XmlTags::MAIN_TAG || !spatMode) {
        return tl::nullopt;
    }

    std::pair<SpeakerSetup, SpatMode> result{};
    result.second = *spatMode;

    forEachXmlChildElement(xml, speaker)
    {
        output_patch_t const outputPatch{ speaker->getTagName().getIntValue() };
        result.first.order.add(outputPatch);
        auto const speakerData{ SpeakerData::fromXml(*speaker) };
        if (!speakerData) {
            return tl::nullopt;
        }

        result.first.speakers.add(outputPatch, std::make_unique<SpeakerData>(*speakerData));
    }

    return tl::make_optional(std::move(result));
}

//==============================================================================
AudioConfig SpatGrisData::toAudioConfig() const
{
    AudioConfig result{};

    for (auto source : project.sources) {
        if (source.value->directOut) {
            result.directOutPairs.add(std::make_pair(source.key, *source.value->directOut));
        }
    }

    auto const isAtLeastOneSourceSolo{ std::any_of(
        project.sources.cbegin(),
        project.sources.cend(),
        [](auto const node) { return node.value->state == PortState::solo; }) };
    auto const isAtLeastOnSpeakerSolo{ std::any_of(
        speakerSetup.speakers.cbegin(),
        speakerSetup.speakers.cend(),
        [](auto const node) { return node.value->state == PortState::solo; }) };

    result.lbapAttenuationConfig = project.lbapDistanceAttenuationData.toConfig(appData.audioSettings.sampleRate);
    result.masterGain = project.masterGain.toGain();
    result.pinkNoiseGain = pinkNoiseLevel.map([](auto const & level) { return level.toGain(); });
    for (auto const source : project.sources) {
        result.sourcesAudioConfig.add(source.key, source.value->toConfig(isAtLeastOneSourceSolo));
    }
    result.spatGainsInterpolation = project.spatGainsInterpolation;
    for (auto const speaker : speakerSetup.speakers) {
        result.speakersAudioConfig.add(
            speaker.key,
            speaker.value->toConfig(isAtLeastOnSpeakerSolo, appData.audioSettings.sampleRate));
    }

    return result;
}