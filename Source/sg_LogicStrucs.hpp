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

//==============================================================================
/** The classical states of a mixing slice (input or output). */
enum class SliceState { normal, muted, solo };
[[nodiscard]] juce::String sliceStateToString(SliceState state);
[[nodiscard]] tl::optional<SliceState> stringToSliceState(juce::String const & string);

//==============================================================================
/** The settings associated with the 3D viewport. */
struct ColdViewSettings {
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
    [[nodiscard]] static tl::optional<ColdViewSettings> fromXml(juce::XmlElement const & xml);
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
struct HotViewportSourceData {
    Position position{};
    float azimuthSpan{};
    float zenithSpan{};
    juce::Colour colour{};
    SpatMode hybridSpatMode{};
};
using HotViewportSourceDataUpdater = AtomicUpdater<tl::optional<HotViewportSourceData>>;

//==============================================================================
/** The WARM data needed in order to properly display a speaker in the 3D viewport. */
struct WarmViewportSpeakerData {
    Position position{};
    bool isSelected{};
    bool isDirectOutOnly{};
};
/** An updater used by the 3D viewport to read the speakers' alpha levels. The writer is the message thread and the
 * reader is the OpenGL thread. */
using HotViewportSpeakerAlphaUpdater = AtomicUpdater<float>;

//==============================================================================
/** The WARM data needed by the 3D viewport that will be updated, but only once in a while with a mutex. */
struct WarmViewportData {
    StaticMap<output_patch_t, WarmViewportSpeakerData, MAX_NUM_SPEAKERS> speakers{};
    ColdViewSettings viewSettings{};
    SpatMode spatMode{};
    juce::String title{};
};

//==============================================================================
/** The COLD 3D viewport's data. */
struct ColdViewportData {
    StrongArray<source_index_t, HotViewportSourceDataUpdater::Token *, MAX_NUM_SOURCES> mostRecentSourcesData{};
    StrongArray<output_patch_t, HotViewportSpeakerAlphaUpdater::Token *, MAX_NUM_SPEAKERS> mostRecentSpeakersAlpha{};
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
    WarmViewportData warmData{};
    ColdViewportData coldData{};
    StaticMap<source_index_t, HotViewportSourceDataUpdater, MAX_NUM_SOURCES> hotSourcesDataUpdaters{};
    StrongArray<output_patch_t, HotViewportSpeakerAlphaUpdater, MAX_NUM_SPEAKERS> hotSpeakersAlphaUpdaters{};
};

//==============================================================================
/** TODO */
struct ColdSourceData {
    SliceState state{};
    tl::optional<Position> position{}; // tl::nullopt if source is inactive
    float azimuthSpan{};
    float zenithSpan{};
    tl::optional<output_patch_t> directOut{};
    juce::Colour colour{ DEFAULT_SOURCE_COLOR };
    SpatMode hybridSpatMode{};
    //==============================================================================
    [[nodiscard]] SourceAudioConfig toConfig(bool soloMode) const;
    [[nodiscard]] HotViewportSourceData toViewportData(float alpha) const;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml(source_index_t index) const;
    [[nodiscard]] static tl::optional<ColdSourceData> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(ColdSourceData const & other) const noexcept;
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
struct ColdSpeakerHighpassData {
    hz_t freq{};
    //==============================================================================
    [[nodiscard]] SpeakerHighpassConfig toConfig(double sampleRate) const;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<ColdSpeakerHighpassData> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(ColdSpeakerHighpassData const & other) const noexcept;
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const FREQ;
    };
};

//==============================================================================
struct ColdSpeakerData {
    SliceState state{};
    Position position{ PolarVector{ radians_t{ 0.0f }, radians_t{ 0.0f }, 1.0f } };
    dbfs_t gain{};
    tl::optional<ColdSpeakerHighpassData> highpassData{};
    float peak{};
    bool isSelected{};
    bool isDirectOutOnly{};
    //==============================================================================
    [[nodiscard]] SpeakerAudioConfig toConfig(bool soloMode, double sampleRate) const noexcept;
    [[nodiscard]] WarmViewportSpeakerData toViewportConfig() const noexcept;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml(output_patch_t outputPatch) const noexcept;
    [[nodiscard]] static tl::optional<ColdSpeakerData> fromXml(juce::XmlElement const & xml) noexcept;
    [[nodiscard]] bool operator==(ColdSpeakerData const & other) const noexcept;
    //==============================================================================
    struct XmlTags {
        static juce::String const STATE;
        static juce::String const GAIN;
        static juce::String const IS_DIRECT_OUT_ONLY;
        static juce::String const MAIN_TAG_PREFIX;
    };
};

//==============================================================================
struct ColdLbapDistanceAttenuationData {
    hz_t freq{ 1000.0f };
    dbfs_t attenuation{};
    //==============================================================================
    [[nodiscard]] LbapAttenuationConfig toConfig(double sampleRate) const;
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<ColdLbapDistanceAttenuationData> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(ColdLbapDistanceAttenuationData const & other) const noexcept;
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const FREQ;
        static juce::String const ATTENUATION;
    };
};

//==============================================================================
struct ColdAudioSettings {
    juce::String deviceType{};
    juce::String inputDevice{};
    juce::String outputDevice{};
    double sampleRate{ DEFAULT_SAMPLE_RATE };
    int bufferSize{ DEFAULT_BUFFER_SIZE };
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<ColdAudioSettings> fromXml(juce::XmlElement const & xml);
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
struct ColdRecordingOptions {
    RecordingFormat format{ DEFAULT_RECORDING_FORMAT };
    RecordingFileType fileType{ DEFAULT_RECORDING_FILE_TYPE };
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<ColdRecordingOptions> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const FORMAT;
        static juce::String const FILE_TYPE;
    };
};

//==============================================================================
struct ColdStereoRouting {
    output_patch_t left{ 1 };
    output_patch_t right{ 2 };
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<ColdStereoRouting> fromXml(juce::XmlElement const & xml);
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const LEFT;
        static juce::String const RIGHT;
    };
};

//==============================================================================
using ColdSourcesData = OwnedMap<source_index_t, ColdSourceData, MAX_NUM_SOURCES>;
struct ColdSpatGrisProjectData {
    ColdSourcesData sources{};
    ColdLbapDistanceAttenuationData lbapDistanceAttenuationData{};
    int oscPort{ DEFAULT_OSC_INPUT_PORT };
    dbfs_t masterGain{};
    float spatGainsInterpolation{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<ColdSpatGrisProjectData> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(ColdSpatGrisProjectData const & other) const noexcept;
    [[nodiscard]] bool operator!=(ColdSpatGrisProjectData const & other) const noexcept { return !(*this == other); }
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
struct ColdSpatGrisAppData {
    ColdAudioSettings audioSettings{};
    ColdRecordingOptions recordingOptions{};
    ColdViewSettings viewSettings{};
    CartesianVector cameraPosition{ -0.5256794095039368f, -2.008379459381104f, 1.312143206596375f };
    juce::String lastSpeakerSetup{ DEFAULT_SPEAKER_SETUP_FILE.getFullPathName() };
    juce::String lastProject{ DEFAULT_PROJECT_FILE.getFullPathName() };
    juce::String lastRecordingDirectory{
        juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory).getFullPathName()
    };
    tl::optional<StereoMode> stereoMode{};
    ColdStereoRouting stereoRouting{};
    int windowX{ 100 };
    int windowY{ 100 };
    int windowWidth{ 1200 };
    int windowHeight{ 637 };
    double sashPosition{ -0.4694048616932104 };
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<ColdSpatGrisAppData> fromXml(juce::XmlElement const & xml);
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
using ColdSpeakersData = OwnedMap<output_patch_t, ColdSpeakerData, MAX_NUM_SPEAKERS>;
using ColdSpeakersOrdering = juce::Array<output_patch_t>;

struct ColdSpeakerSetup {
    ColdSpeakersData speakers{};
    ColdSpeakersOrdering ordering{};
    SpatMode spatMode{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const;
    [[nodiscard]] static tl::optional<ColdSpeakerSetup> fromXml(juce::XmlElement const & xml);
    [[nodiscard]] bool operator==(ColdSpeakerSetup const & other) const noexcept;
    [[nodiscard]] bool operator!=(ColdSpeakerSetup const & other) const noexcept { return !(*this == other); }
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

struct ColdSpatGrisData {
    ColdSpeakerSetup speakerSetup{};
    ColdSpatGrisProjectData project{};
    ColdSpatGrisAppData appData{};
    tl::optional<dbfs_t> pinkNoiseLevel{};
    AtomicUpdater<SourcePeaks>::Token * mostRecentSourcePeaks{};
    AtomicUpdater<SpeakerPeaks>::Token * mostRecentSpeakerPeaks{};
    AtomicUpdater<StereoPeaks>::Token * mostRecentStereoPeaks{};
    //==============================================================================
    [[nodiscard]] std::unique_ptr<AudioConfig> toAudioConfig() const;
    [[nodiscard]] WarmViewportData toViewportConfig() const noexcept;
};