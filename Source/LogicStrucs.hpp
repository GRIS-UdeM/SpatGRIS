#pragma once

#include "AudioStructs.hpp"

enum class PortState { normal, muted, solo };

[[nodiscard]] juce::String portStateToString(PortState const state)
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
[[nodiscard]] tl::optional<PortState> stringToPortState(juce::String const & string)
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

struct SourceData {
    struct XmlTags {
        static juce::String const STATE;
        static juce::String const AZIMUTH_SPAN;
        static juce::String const ZENITH_SPAN;
        static juce::String const DIRECT_OUT;
        static juce::String const COLOUR;
    };

    PortState state{};
    PolarVector vector{};
    CartesianVector position{};
    float azimuthSpan{};
    float zenithSpan{};
    tl::optional<output_patch_t> directOut{};
    float peak{};
    bool isSelected{};
    juce::Colour colour{};

    [[nodiscard]] SourceAudioConfig toConfig(bool const soloMode) const
    {
        SourceAudioConfig result;
        result.directOut = directOut;
        result.isMuted = soloMode ? state == PortState::solo : state != PortState::muted;
        return result;
    }

    [[nodiscard]] juce::XmlElement * toXml(source_index_t const index) const
    {
        auto result{ std::make_unique<juce::XmlElement>(juce::String{ index.get() }) };

        result->setAttribute(XmlTags::STATE, portStateToString(state));
        result->addChildElement(position.toXml());
        result->setAttribute(XmlTags::AZIMUTH_SPAN, azimuthSpan);
        result->setAttribute(XmlTags::ZENITH_SPAN, zenithSpan);
        if (directOut) {
            result->setAttribute(XmlTags::DIRECT_OUT, directOut->get());
        }
        result->setAttribute(XmlTags::COLOUR, juce::String{ colour.getARGB() });

        return result.release();
    }

    [[nodiscard]] static tl::optional<SourceData> fromXml(juce::XmlElement const & xml)
    {
        juce::StringArray const requiredTags{ XmlTags::STATE,
                                              XmlTags::AZIMUTH_SPAN,
                                              XmlTags::ZENITH_SPAN,
                                              XmlTags::COLOUR };

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
        result.azimuthSpan = xml.getDoubleAttribute(XmlTags::AZIMUTH_SPAN);
        result.zenithSpan = xml.getDoubleAttribute(XmlTags::ZENITH_SPAN);
        if (xml.hasAttribute(XmlTags::DIRECT_OUT)) {
            result.directOut = output_patch_t{ xml.getIntAttribute(XmlTags::DIRECT_OUT) };
        }
        result.colour = juce::Colour{ narrow<uint32_t>(xml.getStringAttribute(XmlTags::COLOUR).getLargeIntValue()) };

        return result;
    }
};

struct SpeakerHighpassData {
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const FREQ;
    };

    hz_t freq{};

    [[nodiscard]] SpeakerHighpassConfig toConfig(double const sampleRate) const
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

    [[nodiscard]] juce::XmlElement * toXml() const
    {
        auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };
        result->setAttribute(XmlTags::FREQ, freq.get());
        return result.release();
    }

    [[nodiscard]] static tl::optional<SpeakerHighpassData> fromXml(juce::XmlElement const & xml)
    {
        if (xml.getTagName() != XmlTags::MAIN_TAG || !xml.hasAttribute(XmlTags::FREQ)) {
            return tl::nullopt;
        }

        SpeakerHighpassData result;
        result.freq = hz_t{ static_cast<float>(xml.getDoubleAttribute(XmlTags::FREQ)) };

        return result;
    }
};

struct SpeakerData {
    struct XmlTags {
        static juce::String const STATE;
        static juce::String const GAIN;
        static juce::String const IS_DIRECT_OUT_ONLY;
    };

    PortState state{};
    PolarVector vector{};
    CartesianVector position{};
    float gain{};
    tl::optional<SpeakerHighpassData> crossoverData{};
    float peak{};
    bool isSelected{};
    bool isDirectOutOnly{};

    [[nodiscard]] SpeakerAudioConfig toConfig(bool const soloMode, double const sampleRate) const
    {
        auto const getHighpassConfig = [&](SpeakerHighpassData const & data) { return data.toConfig(sampleRate); };

        SpeakerAudioConfig result;
        result.isMuted = soloMode ? state == PortState::solo : state != PortState::muted;
        result.gain = gain;
        result.highpassConfig = crossoverData.map(getHighpassConfig);
        result.isDirectOutOnly = isDirectOutOnly;
        return result;
    }

    [[nodiscard]] juce::XmlElement * toXml(output_patch_t const outputPatch) const
    {
        auto result{ std::make_unique<juce::XmlElement>(juce::String{ outputPatch.get() }) };

        result->setAttribute(XmlTags::STATE, portStateToString(state));
        result->addChildElement(position.toXml());
        result->setAttribute(XmlTags::GAIN, gain);
        if (crossoverData) {
            result->addChildElement(crossoverData->toXml());
        }
        result->setAttribute(XmlTags::IS_DIRECT_OUT_ONLY, isDirectOutOnly);

        return result.release();
    }

    [[nodiscard]] static tl::optional<SpeakerData> fromXml(juce::XmlElement const & xml)
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
        result.gain = static_cast<float>(xml.getDoubleAttribute(XmlTags::GAIN));
        if (crossoverElement) {
            auto const crossover{ SpeakerHighpassData::fromXml(*crossoverElement) };
            if (!crossover) {
                return tl::nullopt;
            }
            result.crossoverData = crossover;
        }
        result.isDirectOutOnly = xml.getBoolAttribute(XmlTags::IS_DIRECT_OUT_ONLY);

        return result;
    }
};

struct LbapDistanceAttenuationData {
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const FREQ;
        static juce::String const ATTENUATION;
    };

    hz_t freq{};
    dbfs_t attenuation{};

    [[nodiscard]] LbapAttenuationConfig toConfig(double const sampleRate) const
    {
        auto const coefficient{ std::exp(-juce::MathConstants<float>::twoPi * freq.get() / narrow<float>(sampleRate)) };
        auto const gain{ attenuation.toGain() };
        LbapAttenuationConfig const result{ gain, coefficient };
        return result;
    }

    [[nodiscard]] juce::XmlElement * toXml() const
    {
        auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

        result->setAttribute(XmlTags::FREQ, freq.get());
        result->setAttribute(XmlTags::ATTENUATION, attenuation.get());

        return result.release();
    }

    [[nodiscard]] static tl::optional<LbapDistanceAttenuationData> fromXml(juce::XmlElement const & xml)
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
};

struct AudioSettings {
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const INTERFACE_TYPE;
        static juce::String const INPUT_INTERFACE;
        static juce::String const OUTPUT_INTERFACE;
        static juce::String const SAMPLE_RATE;
        static juce::String const BUFFER_SIZE;
    };

    juce::String interfaceType;
    juce::String inputInterface;
    juce::String outputInterface;
    double sampleRate;
    int bufferSize;

    [[nodiscard]] juce::XmlElement * toXml() const
    {
        auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

        result->setAttribute(XmlTags::INTERFACE_TYPE, interfaceType);
        result->setAttribute(XmlTags::INPUT_INTERFACE, inputInterface);
        result->setAttribute(XmlTags::OUTPUT_INTERFACE, outputInterface);
        result->setAttribute(XmlTags::SAMPLE_RATE, sampleRate);
        result->setAttribute(XmlTags::BUFFER_SIZE, bufferSize);

        return result.release();
    }

    [[nodiscard]] static tl::optional<AudioSettings> fromXml(juce::XmlElement const & xml)
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
        result->interfaceType = xml.getStringAttribute(XmlTags::INTERFACE_TYPE);
        result->inputInterface = xml.getStringAttribute(XmlTags::INPUT_INTERFACE);
        result->outputInterface = xml.getStringAttribute(XmlTags::OUTPUT_INTERFACE);
        result->sampleRate = xml.getDoubleAttribute(XmlTags::SAMPLE_RATE);
        result->bufferSize = xml.getIntAttribute(XmlTags::BUFFER_SIZE);

        return result;
    }
};

enum class RecordingFormat { wav, aiff };
enum class RecordingFileType { mono, interleaved };

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

tl::optional<RecordingFileType> stringToRecrdingFileType(juce::String const & string)
{
    if (string == "interleaved") {
        return RecordingFileType::interleaved;
    }
    if (string == "mono") {
        return RecordingFileType::mono;
    }
    return tl::nullopt;
}

struct RecordingOptions {
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const FORMAT;
        static juce::String const FILE_TYPE;
    };

    RecordingFormat format{};
    RecordingFileType fileType{};

    [[nodiscard]] juce::XmlElement * toXml() const
    {
        auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

        result->setAttribute(XmlTags::FORMAT, recordingFormatToString(format));
        result->setAttribute(XmlTags::FILE_TYPE, recordingFileTypeToString(fileType));

        return result.release();
    }

    [[nodiscard]] static tl::optional<RecordingOptions> fromXml(juce::XmlElement const & xml)
    {
        auto const format{ stringToRecordingFormat(xml.getStringAttribute(XmlTags::FORMAT)) };
        auto const fileType{ stringToRecrdingFileType(xml.getStringAttribute(XmlTags::FILE_TYPE)) };

        if (!format || !fileType) {
            return tl::nullopt;
        }

        RecordingOptions result;
        result.format = *format;
        result.fileType = *fileType;

        return result;
    }
};

struct SpatGrisProjectData {
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const SOURCES;
        static juce::String const MASTER_GAIN;
        static juce::String const GAIN_INTERPOLATION;
        static juce::String const OSC_PORT;
    };

    using SourcesData = OwnedMap<source_index_t, SourceData>;
    SourcesData sourcesData{};

    AudioSettings audioSettings{};
    RecordingOptions recordingOptions{};
    int oscPort{};
    float masterGain{};
    float spatGainsInterpolation{};
    LbapDistanceAttenuationData lbapDistanceAttenuationData{};

    [[nodiscard]] juce::XmlElement * toXml() const
    {
        auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };

        auto sources{ std::make_unique<juce::XmlElement>(XmlTags::SOURCES) };
        for (auto const sourceData : sourcesData) {
            sources->addChildElement(sourceData.value->toXml(sourceData.key));
        }
        result->addChildElement(sources.release());
        result->addChildElement(audioSettings.toXml());
        result->addChildElement(recordingOptions.toXml());
        result->setAttribute(XmlTags::OSC_PORT, oscPort);
        result->setAttribute(XmlTags::MASTER_GAIN, masterGain);
        result->setAttribute(XmlTags::GAIN_INTERPOLATION, spatGainsInterpolation);
        result->addChildElement(lbapDistanceAttenuationData.toXml());

        return result.release();
    }

    [[nodiscard]] static bool fromXml(juce::XmlElement const & xml, SpatGrisProjectData & destination)
    {
        juce::StringArray const requiredTags{ XmlTags::MASTER_GAIN, XmlTags::GAIN_INTERPOLATION, XmlTags::OSC_PORT };
        if (!std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & string) {
                return xml.hasAttribute(string);
            })) {
            return false;
        }

        auto const * sourcesElement{ xml.getChildByName(XmlTags::SOURCES) };
        auto const * audioSettingsElement{ xml.getChildByName(AudioSettings::XmlTags::MAIN_TAG) };
        auto const * recordingOptionsElement{ xml.getChildByName(RecordingOptions::XmlTags::MAIN_TAG) };
        auto const * lbapAttenuationElement{ xml.getChildByName(LbapDistanceAttenuationData::XmlTags::MAIN_TAG) };

        if (!sourcesElement || !audioSettingsElement || !recordingOptionsElement || !lbapAttenuationElement) {
            return false;
        }

        destination.sourcesData.clear();
        forEachXmlChildElement(*sourcesElement, sourceElement)
        {
            jassert(sourceElement);
            auto const sourceData{ SourceData::fromXml(*sourceElement) };
            if (!sourceData) {
                return false;
            }
            auto const sourceIndex{ source_index_t{ sourceElement->getTagName().getIntValue() } };
            destination.sourcesData.add(sourceIndex, std::make_unique<SourceData>(*sourceData));
        }

        auto const audioSettings{ AudioSettings::fromXml(*audioSettingsElement) };
        auto const recordingOptions{ RecordingOptions::fromXml(*recordingOptionsElement) };
        auto const lbapAttenuation{ LbapDistanceAttenuationData::fromXml(*lbapAttenuationElement) };

        if (!audioSettings || !recordingOptions || !lbapAttenuation) {
            return false;
        }

        destination.audioSettings = *audioSettings;
        destination.recordingOptions = *recordingOptions;
        destination.masterGain = static_cast<float>(xml.getDoubleAttribute(XmlTags::MASTER_GAIN));
        destination.spatGainsInterpolation = static_cast<float>(xml.getDoubleAttribute(XmlTags::GAIN_INTERPOLATION));
        destination.oscPort = xml.getIntAttribute(XmlTags::OSC_PORT);
        destination.lbapDistanceAttenuationData = *lbapAttenuation;

        return true;
    }
};

struct SpatGrisData {
    using SpeakersData = OwnedMap<output_patch_t, SpeakerData>;
    SpeakersData speakersData{};

    SpatGrisProjectData projectData{};

    tl::optional<float> pinkNoiseGain{};

    [[nodiscard]] std::unique_ptr<AudioConfig> toAudioConfig() const
    {
        auto result{ std::make_unique<AudioConfig>() };

        for (auto source : projectData.sourcesData) {
            if (source.value->directOut) {
                result->directOutPairs.add(std::make_pair(source.key, *source.value->directOut));
            }
        }

        auto const isAtLeastOneSourceSolo{ std::any_of(
            projectData.sourcesData.cbegin(),
            projectData.sourcesData.cend(),
            [](auto const node) { return node.value->state == PortState::solo; }) };
        auto const isAtLeastOnSpeakerSolo{ std::any_of(speakersData.cbegin(), speakersData.cend(), [](auto const node) {
            return node.value->state == PortState::solo;
        }) };

        result->lbapAttenuationConfig
            = projectData.lbapDistanceAttenuationData.toConfig(projectData.audioSettings.sampleRate);
        result->masterGain = projectData.masterGain;
        result->pinkNoiseGain = pinkNoiseGain;
        for (auto const source : projectData.sourcesData) {
            result->sourcesAudioConfig.add(source.key, source.value->toConfig(isAtLeastOneSourceSolo));
        }
        result->spatGainsInterpolation = projectData.spatGainsInterpolation;
        for (auto const speaker : speakersData) {
            result->speakersAudioConfig.add(
                speaker.key,
                speaker.value->toConfig(isAtLeastOnSpeakerSolo, projectData.audioSettings.sampleRate));
        }

        return result;
    }
};