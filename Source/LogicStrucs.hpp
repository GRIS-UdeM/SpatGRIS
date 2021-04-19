#pragma once

#include "AudioStructs.hpp"
#include "OwnedMap.hpp"

static constexpr auto DEFAULT_OSC_INPUT_PORT = 18032;
static constexpr auto MAX_OSC_INPUT_PORT = 65535;

enum class PortState { normal, muted, solo };

[[nodiscard]] juce::String portStateToString(PortState state);
[[nodiscard]] tl::optional<PortState> stringToPortState(juce::String const & string);

//==============================================================================
struct ViewSettings {
    bool showSpeakers{};
    bool showSpeakerNumbers{};
    bool showSpeakerTriplets{};
    bool showSpeakerLevels{};
    bool showSphereOrCube{};
    bool showSourceActivity{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<ViewSettings> fromXml(juce::XmlElement const & xml);
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
struct ViewportSourceData {
    CartesianVector position{};
    float azimuthSpan{};
    float zenithSpan{};
    juce::Colour colour{};
};

struct ViewportSpeakerConfig {
    CartesianVector position{};
    bool isSelected{};
    bool isDirectOutOnly{};
};

struct ViewportConfig {
    StaticMap<output_patch_t, ViewportSpeakerConfig, MAX_OUTPUTS> speakers{};
    ViewSettings viewSettings{};
    SpatMode spatMode{};
    juce::String title{};
};

struct ViewportState {
    float cameraZoomVelocity{};
    PolarVector cameraPosition{};
    int lastRenderTimeMs{};
    juce::Point<float> deltaClick{};
    juce::Point<float> rayClick{};
    bool shouldRayCast{};
    float displayScaling{};
};

struct ViewportData {
    ViewportConfig config{};
    ViewportState state{};
    StaticMap<source_index_t, ThreadsafePtr<tl::optional<ViewportSourceData>>, MAX_INPUTS> sources{};
    StrongArray<output_patch_t, ThreadsafePtr<float>, MAX_OUTPUTS> speakersAlpha{};
};

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
    [[nodiscard]] ViewportSourceData toViewportData(float alpha) const;
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

    [[nodiscard]] constexpr bool isSameAs(Triplet const & other) const noexcept
    {
        return contains(other.id1) && contains(other.id2) && contains(other.id3);
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
    dbfs_t gain{};
    tl::optional<SpeakerHighpassData> highpassData{};
    float peak{};
    bool isSelected{};
    bool isDirectOutOnly{};
    //==============================================================================
    [[nodiscard]] SpeakerAudioConfig toConfig(bool soloMode, double sampleRate) const noexcept;
    [[nodiscard]] ViewportSpeakerConfig toViewportConfig() const noexcept;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml(output_patch_t outputPatch) const noexcept;
    [[nodiscard]] static tl::optional<SpeakerData> fromXml(juce::XmlElement const & xml) noexcept;
    //==============================================================================
    struct XmlTags {
        static juce::String const STATE;
        static juce::String const GAIN;
        static juce::String const IS_DIRECT_OUT_ONLY;
    };
};

//==============================================================================
struct LbapDistanceAttenuationData {
    hz_t freq{ 1000.0f };
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
using SourcesData = OwnedMap<source_index_t, SourceData, MAX_INPUTS>;
struct SpatGrisProjectData {
    SourcesData sources{};
    LbapDistanceAttenuationData lbapDistanceAttenuationData{};
    ViewSettings viewSettings{};
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
    juce::String lastSpeakerSetup{ DEFAULT_SPEAKER_SETUP_FILE.getFullPathName() };
    juce::String lastProject{ DEFAULT_PROJECT_FILE.getFullPathName() };
    juce::String lastRecordingDirectory{};
    SpatMode spatMode{};
    int windowX{};
    int windowY{};
    int windowWidth{ 1285 };
    int windowHeight{ 610 };
    double sashPosition{ 0.5 };
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
using SpeakersData = OwnedMap<output_patch_t, SpeakerData, MAX_OUTPUTS>;

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
    [[nodiscard]] ViewportConfig toViewportConfig() const noexcept;
};