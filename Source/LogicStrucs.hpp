#pragma once

#include "AudioStructs.hpp"
#include "OwnedMap.hpp"

static constexpr auto DEFAULT_OSC_INPUT_PORT = 18032;
static constexpr auto MAX_OSC_INPUT_PORT = 65535;

enum class PortState { normal, muted, solo };

[[nodiscard]] juce::String portStateToString(PortState const state);
[[nodiscard]] tl::optional<PortState> stringToPortState(juce::String const & string);

//==============================================================================
struct SourceData {
    PortState state{};
    tl::optional<PolarVector> vector{};
    tl::optional<CartesianVector> position{};
    float azimuthSpan{};
    float zenithSpan{};
    tl::optional<output_patch_t> directOut{};
    float peak{};
    bool isSelected{};
    juce::Colour colour{};
    //==============================================================================
    [[nodiscard]] SourceAudioConfig toConfig(bool soloMode) const;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml(source_index_t index) const;
    [[nodiscard]] static tl::optional<SourceData> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const STATE;
        static juce::String const DIRECT_OUT;
        static juce::String const COLOUR;
    };
};

struct Triplet {
    output_patch_t id1{};
    output_patch_t id2{};
    output_patch_t id3{};

    [[nodiscard]] constexpr bool contains(output_patch_t const outputPatch) const noexcept
    {
        return id1 == outputPatch || id2 == outputPatch || id3 == outputPatch;
    }
};

//==============================================================================
struct SpeakerHighpassData {
    hz_t freq{};
    //==============================================================================
    [[nodiscard]] SpeakerHighpassConfig toConfig(double sampleRate) const;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<SpeakerHighpassData> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const FREQ;
    };
};

//==============================================================================
struct SpeakerData {
    PortState state{};
    PolarVector vector{};
    CartesianVector position{};
    float gain{};
    tl::optional<SpeakerHighpassData> crossoverData{};
    float peak{};
    bool isSelected{};
    bool isDirectOutOnly{};
    //==============================================================================
    [[nodiscard]] SpeakerAudioConfig toConfig(bool soloMode, double sampleRate) const;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml(output_patch_t outputPatch) const;
    [[nodiscard]] static tl::optional<SpeakerData> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const STATE;
        static juce::String const GAIN;
        static juce::String const IS_DIRECT_OUT_ONLY;
    };
};

//==============================================================================
struct LbapDistanceAttenuationData {
    hz_t freq{};
    dbfs_t attenuation{};
    //==============================================================================
    [[nodiscard]] LbapAttenuationConfig toConfig(double sampleRate) const;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<LbapDistanceAttenuationData> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const FREQ;
        static juce::String const ATTENUATION;
    };
};

//==============================================================================
struct AudioSettings {
    juce::String deviceType{};
    juce::String inputDevice{};
    juce::String outputDevice{};
    double sampleRate{};
    int bufferSize{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<AudioSettings> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const INTERFACE_TYPE;
        static juce::String const INPUT_INTERFACE;
        static juce::String const OUTPUT_INTERFACE;
        static juce::String const SAMPLE_RATE;
        static juce::String const BUFFER_SIZE;
    };
};

//==============================================================================
enum class RecordingFormat { wav, aiff };
enum class RecordingFileType { mono, interleaved };

juce::String recordingFormatToString(RecordingFormat format);
tl::optional<RecordingFormat> stringToRecordingFormat(juce::String const & string);
juce::String recordingFileTypeToString(RecordingFileType fileType);
tl::optional<RecordingFileType> stringToRecordingFileType(juce::String const & string);

//==============================================================================
struct RecordingOptions {
    RecordingFormat format{};
    RecordingFileType fileType{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<RecordingOptions> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const FORMAT;
        static juce::String const FILE_TYPE;
    };
};

//==============================================================================
struct SpatGrisViewSettings {
    bool showSpeakers{};
    bool showSpeakerNumbers{};
    bool showSpeakerTriplets{};
    bool showSpeakerLevels{};
    bool showSphereOrCube{};
    bool showSourceActivity{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<SpatGrisViewSettings> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const SHOW_SPEAKERS;
        static juce::String const SHOW_SPEAKER_NUMBERS;
        static juce::String const SHOW_SPEAKER_TRIPLETS;
        static juce::String const SHOW_SPEAKER_LEVELS;
        static juce::String const SHOW_SPHERE_OR_CUBE;
        static juce::String const SHOW_SOURCE_ACTIVITY;
    };
};

//==============================================================================
using SourcesData = OwnedMap<source_index_t, SourceData>;
struct SpatGrisProjectData {
    SourcesData sources{};
    LbapDistanceAttenuationData lbapDistanceAttenuationData{};
    SpatGrisViewSettings viewSettings{};
    CartesianVector cameraPosition{};
    int oscPort{ DEFAULT_OSC_INPUT_PORT };
    dbfs_t masterGain{};
    float spatGainsInterpolation{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<SpatGrisProjectData> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const SOURCES;
        static juce::String const MASTER_GAIN;
        static juce::String const GAIN_INTERPOLATION;
        static juce::String const OSC_PORT;
        static juce::String const CAMERA;
    };
};

//==============================================================================
struct SpatGrisAppData {
    AudioSettings audioSettings{};
    RecordingOptions recordingOptions{};
    juce::String lastSpeakerSetup{};
    juce::String lastProject{};
    juce::String lastRecordingDirectory{};
    SpatMode spatMode{};
    int windowX{};
    int windowY{};
    int windowWidth{};
    int windowHeight{};
    double sashPosition{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<SpatGrisAppData> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const LAST_SPEAKER_SETUP;
        static juce::String const LAST_PROJECT;
        static juce::String const LAST_RECORDING_DIRECTORY;
        static juce::String const LAST_SPAT_MODE;
        static juce::String const WINDOW_X;
        static juce::String const WINDOW_Y;
        static juce::String const WINDOW_WIDTH;
        static juce::String const WINDOW_HEIGHT;
        static juce::String const SASH_POSITION;
    };
};

//==============================================================================
using SpeakersData = OwnedMap<output_patch_t, SpeakerData>;

struct SpeakerSetup {
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const SPAT_MODE;
    };

    SpeakersData speakers{};
    juce::Array<output_patch_t> order{};

    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml(SpatMode mode) const;
    [[nodiscard]] static tl::optional<std::pair<SpeakerSetup, SpatMode>> fromXml(juce::XmlElement const & xml);
};

struct SpatGrisData {
    SpeakerSetup speakerSetup{};
    SpatGrisProjectData project{};
    SpatGrisAppData appData{};
    tl::optional<dbfs_t> pinkNoiseLevel{};
    //==============================================================================
    [[nodiscard]] AudioConfig toAudioConfig() const;
};