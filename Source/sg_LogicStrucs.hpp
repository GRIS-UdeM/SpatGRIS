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

#include "sg_AudioStructs.hpp"
#include "sg_CartesianVector.hpp"
#include "sg_Dbfs.hpp"
#include "sg_Hz.hpp"
#include "sg_OwnedMap.hpp"
#include "sg_Position.hpp"
#include "sg_Radians.hpp"
#include "sg_SourceIndex.hpp"
#include "sg_StaticMap.hpp"
#include "sg_Triplet.hpp"

static constexpr auto DEFAULT_OSC_INPUT_PORT = 18032;
static constexpr auto MAX_OSC_INPUT_PORT = 65535;

/** Signifies that a piece of data will be updated by an external thread DURING PLAYBACK.
 *
 * These structures have to be updated using a lock-free mechanisms (see AtomicUpdater) in order to prevent audio or
 * visual glitches.
 *
 * This  doesn't do anything: it's just a reminder tag.
 */
struct HOT {
};

/** Signifies that a piece of data will be updated by an external thread BUT NEVER DURING PLAYBACK.
 *
 * This means that audio and visual glitches are ok and that a standard lock-based approach can be used.
 *
 * This  doesn't do anything: it's just a reminder tag.
 */
struct WARM {
};

/** Signifies that a piece of data that is NEVER updated by an external thread.
 *
 * This  doesn't do anything: it's just a reminder tag.
 */
struct COLD {
};

//==============================================================================
/** The classical states of a mixing slice (input or output). */
enum class SliceState { normal, muted, solo };
[[nodiscard]] juce::String sliceStateToString(SliceState state);
[[nodiscard]] tl::optional<SliceState> stringToSliceState(juce::String const & string);

//==============================================================================
/** The settings associated with the 3D viewport. */
struct ViewSettings : COLD {
    bool showSpeakers{ true };
    bool showSourceNumbers{ false };
    bool showSpeakerNumbers{ false };
    bool showSpeakerTriplets{ false };
    bool showSpeakerLevels{ false };
    bool showSphereOrCube{ false };
    bool showSourceActivity{ false };
    //==============================================================================
    /** Used for saving. */
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    /** Used for loading. */
    [[nodiscard]] static tl::optional<ViewSettings> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const SHOW_SPEAKERS;
        static juce::String const SHOW_SOURCE_NUMBERS;
        static juce::String const SHOW_SPEAKER_NUMBERS;
        static juce::String const SHOW_SPEAKER_TRIPLETS;
        static juce::String const SHOW_SPEAKER_LEVELS;
        static juce::String const SHOW_SPHERE_OR_CUBE;
        static juce::String const SHOW_SOURCE_ACTIVITY;
    };
};

//==============================================================================
/** The data needed in order to properly display a source in the 3D viewport. */
struct ViewportSourceData : HOT {
    Position position{};
    float azimuthSpan{};
    float zenithSpan{};
    juce::Colour colour{};
    SpatMode hybridSpatMode{};
};
/** A ViewportSourceData thread-safe updater. The writer is the message thread and the reader is the OpenGL thread. */
using ViewPortSourceDataUpdater = AtomicUpdater<tl::optional<ViewportSourceData>>;

//==============================================================================
/** The WARM data needed in order to properly display a speaker in the 3D viewport. */
struct ViewportSpeakerConfig : WARM {
    Position position{};
    bool isSelected{};
    bool isDirectOutOnly{};
};
/** An updater used by the 3D viewport to read the speakers' alpha levels. The writer is the message thread and the
 * reader is the OpenGL thread. */
using ViewPortSpeakerAlphaUpdater = AtomicUpdater<float>;

//==============================================================================
/** The WARM data needed by the 3D viewport that will be updated, but only once in a while with a mutex. */
struct ViewportConfig : WARM {
    StaticMap<output_patch_t, ViewportSpeakerConfig, MAX_NUM_SPEAKERS> speakers{};
    ViewSettings viewSettings{};
    SpatMode spatMode{};
    juce::String title{};
};

//==============================================================================
/** The COLD 3D viewport's data. */
struct ViewportState : COLD {
    StrongArray<source_index_t, ViewPortSourceDataUpdater::Token *, MAX_NUM_SOURCES> mostRecentSourcesData{};
    StrongArray<output_patch_t, ViewPortSpeakerAlphaUpdater::Token *, MAX_NUM_SPEAKERS> mostRecentSpeakersAlpha{};
    float cameraZoomVelocity{};
    Position cameraPosition{};
    juce::int64 lastRenderTimeMs{ juce::Time::currentTimeMillis() };
    juce::Point<float> rayClick{};
    bool shouldRayCast{};
    juce::Point<float> panMouseOrigin{};
    PolarVector panCameraOrigin{};
    float displayScaling{};
    juce::Array<Triplet> triplets{};
};

//==============================================================================
/** All of the 3D viewport's data. */
struct ViewportData {
    ViewportConfig config{}; // Modified once in a while by the message thread
    ViewportState state{};   // Never touched by the message thread
    StaticMap<source_index_t, ViewPortSourceDataUpdater, MAX_NUM_SOURCES>
        sourcesDataQueues{}; // Constantly modified by the message thread
    StrongArray<output_patch_t, ViewPortSpeakerAlphaUpdater, MAX_NUM_SPEAKERS>
        speakersAlphaQueues{}; // Constantly modified by the message thread
};

//==============================================================================
/**  */
struct SourceData {
    SliceState state{};
    tl::optional<Position> position{}; // tl::nullopt if source is inactive
    float azimuthSpan{};
    float zenithSpan{};
    tl::optional<output_patch_t> directOut{};
    juce::Colour colour{ DEFAULT_SOURCE_COLOR };
    SpatMode hybridSpatMode{};
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
        static juce::String const HYBRID_SPAT_MODE;
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
    SliceState state{};
    Position position{ PolarVector{ radians_t{ 0.0f }, radians_t{ 0.0f }, 1.0f } };
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
enum class RecordingFormat {
    wav,
    aiff
#ifdef USE_CAF
    ,
    caf
#endif
};
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
struct StereoRouting {
    output_patch_t left{ 1 };
    output_patch_t right{ 2 };
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<StereoRouting> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const LEFT;
        static juce::String const RIGHT;
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
        static juce::String const VERSION;
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
    CartesianVector cameraPosition{ -0.5256794095039368f, -2.008379459381104f, 1.312143206596375f };
    juce::String lastSpeakerSetup{ DEFAULT_SPEAKER_SETUP_FILE.getFullPathName() };
    juce::String lastProject{ DEFAULT_PROJECT_FILE.getFullPathName() };
    juce::String lastRecordingDirectory{
        juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory).getFullPathName()
    };
    tl::optional<StereoMode> stereoMode{};
    StereoRouting stereoRouting{};
    int windowX{ 100 };
    int windowY{ 100 };
    int windowWidth{ 1200 };
    int windowHeight{ 637 };
    double sashPosition{ -0.4694048616932104 };
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<SpatGrisAppData> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const LAST_SPEAKER_SETUP;
        static juce::String const LAST_PROJECT;
        static juce::String const LAST_RECORDING_DIRECTORY;
        static juce::String const LAST_STEREO_MODE;
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
using SpeakersOrdering = juce::Array<output_patch_t>;

struct SpeakerSetup {
    SpeakersData speakers{};
    SpeakersOrdering ordering{};
    SpatMode spatMode{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<SpeakerSetup> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(SpeakerSetup const & other) const noexcept;
    [[nodiscard]] bool operator!=(SpeakerSetup const & other) const noexcept { return !(*this == other); }
    [[nodiscard]] bool isDomeLike() const noexcept;
    [[nodiscard]] SpeakersAudioConfig toAudioConfig(double sampleRate) const noexcept;
    [[nodiscard]] int numOfSpatializedSpeakers() const noexcept;
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const VERSION;
        static juce::String const SPAT_MODE;
    };
};

struct SpatGrisData {
    SpeakerSetup speakerSetup{};
    SpatGrisProjectData project{};
    SpatGrisAppData appData{};
    tl::optional<dbfs_t> pinkNoiseLevel{};
    AtomicUpdater<SourcePeaks>::Token * mostRecentSourcePeaks{};
    AtomicUpdater<SpeakerPeaks>::Token * mostRecentSpeakerPeaks{};
    AtomicUpdater<StereoPeaks>::Token * mostRecentStereoPeaks{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<AudioConfig> toAudioConfig() const;
    [[nodiscard]] ViewportConfig toViewportConfig() const noexcept;
};