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

#pragma once

#include "AudioStructs.hpp"
#include "OwnedMap.hpp"
#include "Triplet.hpp"

static constexpr auto DEFAULT_OSC_INPUT_PORT = 18032;
static constexpr auto MAX_OSC_INPUT_PORT = 65535;

enum class PortState { normal, muted, solo };

[[nodiscard]] juce::String portStateToString(PortState state);
[[nodiscard]] tl::optional<PortState> stringToPortState(juce::String const & string);

//==============================================================================
struct ViewSettings {
    bool showSpeakers{ true };
    bool showSpeakerNumbers{ false };
    bool showSpeakerTriplets{ false };
    bool showSpeakerLevels{ false };
    bool showSphereOrCube{ false };
    bool showSourceActivity{ false };
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
    StaticMap<output_patch_t, ViewportSpeakerConfig, MAX_NUM_SPEAKERS> speakers{};
    ViewSettings viewSettings{};
    SpatMode spatMode{};
    juce::String title{};
};

struct ViewportState {
    StrongArray<source_index_t, AtomicExchanger<tl::optional<ViewportSourceData>>::Ticket *, MAX_NUM_SOURCES>
        mostRecentSourcesData{};
    StrongArray<output_patch_t, AtomicExchanger<float>::Ticket *, MAX_NUM_SPEAKERS> mostRectentSpeakersAlpha{};
    float cameraZoomVelocity{};
    PolarVector cameraPosition{};
    int lastRenderTimeMs{};
    juce::Point<float> rayClick{};
    bool shouldRayCast{};
    juce::Point<float> panMouseOrigin{};
    PolarVector panCameraOrigin{};
    float displayScaling{};
    juce::Array<Triplet> triplets{};
};

struct ViewportData {
    ViewportConfig config{};
    ViewportState state{};
    StaticMap<source_index_t, AtomicExchanger<tl::optional<ViewportSourceData>>, MAX_NUM_SOURCES> sources{};
    StrongArray<output_patch_t, AtomicExchanger<float>, MAX_NUM_SPEAKERS> speakersAlpha{};
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
    juce::Colour colour{ DEFAULT_SOURCE_COLOR };
    //==============================================================================
    [[nodiscard]] SourceAudioConfig toConfig(bool soloMode) const;
    [[nodiscard]] ViewportSourceData toViewportData(float alpha) const;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml(source_index_t index) const;
    [[nodiscard]] static tl::optional<SourceData> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(SourceData const & other) const noexcept;
    //==============================================================================
    struct XmlTags {
        static juce::String const STATE;
        static juce::String const DIRECT_OUT;
        static juce::String const COLOUR;
        static juce::String const MAIN_TAG_PREFIX;
    };
};

//==============================================================================
struct SpeakerHighpassData {
    hz_t freq{};
    //==============================================================================
    [[nodiscard]] SpeakerHighpassConfig toConfig(double sampleRate) const;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<SpeakerHighpassData> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(SpeakerHighpassData const & other) const noexcept;
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
    [[nodiscard]] bool operator==(SpeakerData const & other) const noexcept;
    //==============================================================================
    struct XmlTags {
        static juce::String const STATE;
        static juce::String const GAIN;
        static juce::String const IS_DIRECT_OUT_ONLY;
        static juce::String const MAIN_TAG_PREFIX;
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
    [[nodiscard]] bool operator==(LbapDistanceAttenuationData const & other) const noexcept;
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
    double sampleRate{ DEFAULT_SAMPLE_RATE };
    int bufferSize{ DEFAULT_BUFFER_SIZE };
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
constexpr auto DEFAULT_RECORDING_FORMAT{ RecordingFormat::wav };
enum class RecordingFileType { mono, interleaved };
constexpr auto DEFAULT_RECORDING_FILE_TYPE{ RecordingFileType::mono };

juce::String recordingFormatToString(RecordingFormat format);
tl::optional<RecordingFormat> stringToRecordingFormat(juce::String const & string);
juce::String recordingFileTypeToString(RecordingFileType fileType);
tl::optional<RecordingFileType> stringToRecordingFileType(juce::String const & string);

//==============================================================================
struct RecordingOptions {
    RecordingFormat format{ DEFAULT_RECORDING_FORMAT };
    RecordingFileType fileType{ DEFAULT_RECORDING_FILE_TYPE };
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
using SourcesData = OwnedMap<source_index_t, SourceData, MAX_NUM_SOURCES>;
struct SpatGrisProjectData {
    SourcesData sources{};
    LbapDistanceAttenuationData lbapDistanceAttenuationData{};
    int oscPort{ DEFAULT_OSC_INPUT_PORT };
    dbfs_t masterGain{};
    float spatGainsInterpolation{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<SpatGrisProjectData> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(SpatGrisProjectData const & other) const noexcept;
    [[nodiscard]] bool operator!=(SpatGrisProjectData const & other) const noexcept { return !(*this == other); }
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const SOURCES;
        static juce::String const MASTER_GAIN;
        static juce::String const GAIN_INTERPOLATION;
        static juce::String const OSC_PORT;
    };
};

//==============================================================================
struct SpatGrisAppData {
    AudioSettings audioSettings{};
    RecordingOptions recordingOptions{};
    ViewSettings viewSettings{};
    CartesianVector cameraPosition{ 0.0f, -2.0f, 1.0f };
    juce::String lastLbapOrVbapSpeakerSetup{ DEFAULT_SPEAKER_SETUP_FILE.getFullPathName() };
    juce::String lastProject{ DEFAULT_PROJECT_FILE.getFullPathName() };
    juce::String lastRecordingDirectory{
        juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory).getFullPathName()
    };
    SpatMode spatMode{};
    int windowX{ 100 };
    int windowY{ 100 };
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
        static juce::String const CAMERA;
    };
};

//==============================================================================
using SpeakersData = OwnedMap<output_patch_t, SpeakerData, MAX_NUM_SPEAKERS>;

struct SpeakerSetup {
    SpeakersData speakers{};
    juce::Array<output_patch_t> order{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml(SpatMode spatMode) const;
    [[nodiscard]] static tl::optional<std::pair<SpeakerSetup, SpatMode>> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(SpeakerSetup const & other) const noexcept;
    [[nodiscard]] bool operator!=(SpeakerSetup const & other) const noexcept { return !(*this == other); }
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const SPAT_MODE;
    };
};

struct SpatGrisData {
    SpeakerSetup speakerSetup{};
    SpatGrisProjectData project{};
    SpatGrisAppData appData{};
    tl::optional<dbfs_t> pinkNoiseLevel{};
    AtomicExchanger<SourcePeaks>::Ticket * mostRecentSourcePeaks{};
    AtomicExchanger<SpeakerPeaks>::Ticket * mostRecentSpeakerPeaks{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<AudioConfig> toAudioConfig() const;
    [[nodiscard]] ViewportConfig toViewportConfig() const noexcept;
};