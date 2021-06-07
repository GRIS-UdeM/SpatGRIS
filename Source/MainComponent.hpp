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

#include "AboutWindow.hpp"
#include "AudioProcessor.hpp"
#include "Configuration.hpp"
#include "Constants.hpp"
#include "ControlPanel.hpp"
#include "EditSpeakersWindow.hpp"
#include "FlatViewWindow.hpp"
#include "InfoPanel.hpp"
#include "LayoutComponent.hpp"
#include "LogicStrucs.hpp"
#include "OscInput.hpp"
#include "OscLogWindow.hpp"
#include "OwnedMap.hpp"
#include "PrepareToRecordWindow.hpp"
#include "SettingsWindow.hpp"
#include "SpeakerViewComponent.hpp"
#include "StrongTypes.hpp"
#include "TitledComponent.hpp"
#include "Triplet.hpp"
#include "VuMeterComponent.hpp"

class MainWindow;

//==============================================================================
class AudioDeviceManagerListener : public juce::ChangeListener
{
protected:
    virtual void audioParametersChanged() = 0;

private:
    void changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster * source) override
    {
        jassert(dynamic_cast<juce::AudioDeviceManager *>(source));
        audioParametersChanged();
    }
};

//==============================================================================
class MainContentComponent final
    : public juce::Component
    , public juce::MenuBarModel
    , public juce::ApplicationCommandTarget
    , public SourceVuMeterComponent::Owner
    , public SpeakerVuMeterComponent::Owner
    , public ControlPanel::Listener
    , private AudioDeviceManagerListener
    , private juce::Timer
{
    enum class LoadProjectOption { removeInvalidDirectOuts, dontRemoveInvalidDirectOuts };
    enum class LoadSpeakerSetupOption { allowDiscardingUnsavedChanges, disallowDiscardingUnsavedChanges };

    juce::ReadWriteLock mLock{};

    std::unique_ptr<AudioProcessor> mAudioProcessor{};

    // Speakers.
    juce::Array<Triplet> mTriplets{};

    OwnedMap<source_index_t, SourceVuMeterComponent, MAX_NUM_SOURCES> mSourceVuMeterComponents{};
    OwnedMap<output_patch_t, SpeakerVuMeterComponent, MAX_NUM_SPEAKERS> mSpeakerVuMeterComponents{};

    // Open Sound Control.
    std::unique_ptr<OscInput> mOscReceiver{};

    // Windows.
    std::unique_ptr<EditSpeakersWindow> mEditSpeakersWindow{};
    std::unique_ptr<SettingsWindow> mPropertiesWindow{};
    std::unique_ptr<FlatViewWindow> mFlatViewWindow{};
    std::unique_ptr<AboutWindow> mAboutWindow{};
    std::unique_ptr<OscLogWindow> mOscLogWindow{};
    std::unique_ptr<PrepareToRecordWindow> mPrepareToRecordWindow{};

    // info section
    std::unique_ptr<InfoPanel> mInfoPanel{};

    // Sources Section
    std::unique_ptr<TitledComponent> mSourcesSection{};
    std::unique_ptr<LayoutComponent> mSourcesLayout{};

    // Speakers Section
    std::unique_ptr<TitledComponent> mSpeakersSection{};
    std::unique_ptr<LayoutComponent> mSpeakersLayout{};

    // Controls section
    std::unique_ptr<TitledComponent> mControlsSection{};
    std::unique_ptr<ControlPanel> mControlPanel{};

    // Main ui
    std::unique_ptr<LayoutComponent> mMainLayout{};

    // UI Components.
    std::unique_ptr<SpeakerViewComponent> mSpeakerViewComponent{};
    juce::StretchableLayoutManager mVerticalLayout{};
    std::unique_ptr<juce::StretchableLayoutResizerBar> mVerticalDividerBar{};

    // App splash screen.
    std::unique_ptr<juce::SplashScreen> mSplashScreen{};

    // Flags.
    bool mIsProcessForeground{ true };
    //==============================================================================
    // Look-and-feel.
    GrisLookAndFeel & mLookAndFeel;
    SmallGrisLookAndFeel & mSmallLookAndFeel;

    MainWindow & mMainWindow;

    std::unique_ptr<juce::MenuBarComponent> mMenuBar{};
    //==============================================================================
    // App user settings.

    Configuration mConfiguration;
    juce::Rectangle<int> mFlatViewWindowRect{};

    SpatGrisData mData{};

public:
    //==============================================================================
    MainContentComponent(MainWindow & mainWindow,
                         GrisLookAndFeel & grisLookAndFeel,
                         SmallGrisLookAndFeel & smallGrisLookAndFeel);
    //==============================================================================
    MainContentComponent() = delete;
    ~MainContentComponent() override;

    MainContentComponent(MainContentComponent const &) = delete;
    MainContentComponent(MainContentComponent &&) = delete;

    MainContentComponent & operator=(MainContentComponent const &) = delete;
    MainContentComponent & operator=(MainContentComponent &&) = delete;
    //==============================================================================
    // Exit application.
    [[nodiscard]] bool exitApp();

    void updatePeaks();

    auto const & getData() const noexcept { return mData; }

    void refreshSourceVuMeterComponents();
    void refreshSpeakerVuMeterComponents();

    void warnIfDirectOutMismatch();

    auto const & getLock() const { return mLock; }

    void handleSourcePositionChanged(source_index_t const sourceIndex,
                                     radians_t azimuth,
                                     radians_t elevation,
                                     float length,
                                     float newAzimuthSpan,
                                     float newZenithSpan);
    void resetSourcePosition(source_index_t sourceIndex);

    void setRecordingFormat(RecordingFormat format);
    void setRecordingFileType(RecordingFileType fileType);

    void handleSpeakerOnlyDirectOutChanged(output_patch_t outputPatch, bool state);
    void handleSpeakerOutputPatchChanged(output_patch_t oldOutputPatch, output_patch_t newOutputPatch);
    void handleSetSpeakerGain(output_patch_t outputPatch, dbfs_t gain);
    void handleSetSpeakerHighPassFreq(output_patch_t outputPatch, hz_t freq);

    void handlePinkNoiseGainChanged(tl::optional<dbfs_t> gain);

    void handleSourceColorChanged(source_index_t sourceIndex, juce::Colour colour) override;
    void handleSourceStateChanged(source_index_t sourceIndex, PortState state) override;
    void handleSpeakerSelected(juce::Array<output_patch_t> selection) override;
    void handleSpeakerStateChanged(output_patch_t outputPatch, PortState state) override;
    void handleSourceDirectOutChanged(source_index_t sourceIndex, tl::optional<output_patch_t> outputPatch) override;
    [[nodiscard]] SpeakersData const & getSpeakersData() const override { return mData.speakerSetup.speakers; }

    void handleNewSpeakerPosition(output_patch_t outputPatch, CartesianVector const & position);
    void handleNewSpeakerPosition(output_patch_t outputPatch, PolarVector const & position);

    void updateAudioProcessor() const;
    void updateSpatAlgorithm() const;
    void updateViewportConfig() const;

    void handleSetShowTriplets(bool state);

    [[nodiscard]] bool setSpatMode(SpatMode spatMode) override;
    void setStereoMode(tl::optional<StereoMode> stereoMode) override;
    [[nodiscard]] bool spatModeChanged(SpatMode spatMode, LoadSpeakerSetupOption option);
    void cubeAttenuationDbChanged(dbfs_t value) override;
    void cubeAttenuationHzChanged(hz_t value) override;

    // Speakers.
    [[nodiscard]] auto const & getSpeakersDisplayOrder() const { return mData.speakerSetup.order; }

    output_patch_t addSpeaker(tl::optional<output_patch_t> speakerToCopy, tl::optional<int> index);
    void removeSpeaker(output_patch_t outputPatch);
    void reorderSpeakers(juce::Array<output_patch_t> newOrder);

    [[nodiscard]] AudioProcessor & getAudioProcessor() { return *mAudioProcessor; }
    [[nodiscard]] AudioProcessor const & getAudioProcessor() const { return *mAudioProcessor; }

    // Mute - solo.
    void setSourceState(source_index_t sourceIndex, PortState state);
    void setSpeakerState(output_patch_t outputPatch, PortState state);

    // Called when the speaker setup has changed.
    bool refreshSpeakers();

    // Screen refresh timer.
    void handleTimer(bool state);
    void handleSaveSpeakerSetup();
    void handleSaveSpeakerSetupAs();
    void handleShowPreferences();

    // Close windows other than the main one.
    void closeSpeakersConfigurationWindow();
    void closePropertiesWindow() { mPropertiesWindow.reset(); }
    void closeFlatViewWindow() { mFlatViewWindow.reset(); }
    void closeAboutWindow() { mAboutWindow.reset(); }
    void closeOscLogWindow() { mOscLogWindow.reset(); }
    void closePrepareToRecordWindow() { mPrepareToRecordWindow.reset(); }
    //==============================================================================
    void timerCallback() override;
    void paint(juce::Graphics & g) override;
    void resized() override;
    void menuItemSelected(int menuItemId, int /*topLevelMenuIndex*/) override;
    [[nodiscard]] juce::StringArray getMenuBarNames() override;
    [[nodiscard]] juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String & /*menuName*/) override;

    [[nodiscard]] bool isProjectModified() const;
    [[nodiscard]] bool isSpeakerSetupModified() const;

    void prepareAndStartRecording(juce::File const & fileOrDirectory, RecordingOptions const & recordingOptions);

    void masterGainChanged(dbfs_t gain) override;
    void interpolationChanged(float interpolation) override;
    void numSourcesChanged(int numSources) override;
    void recordButtonPressed() override;

private:
    //==============================================================================
    // MenuBar handlers.
    void handleNewProject();
    void handleOpenProject();
    void handleSaveProject();
    void handleSaveProjectAs();
    void handleOpenSpeakerSetup();
    void handleShowSpeakerEditWindow();
    void handleShowAbout();
    void handleShow2DView();
    void handleShowNumbers();
    void handleShowSpeakers();
    void handleShowTriplets();
    void handleShowSourceLevel();
    void handleShowSpeakerLevel();
    void handleShowSphere();
    void handleResetInputPositions();
    void handleResetMeterClipping();
    void handleColorizeInputs();

    [[nodiscard]] output_patch_t getMaxSpeakerOutputPatch() const;
    //==============================================================================
    // Open - save.
    [[nodiscard]] static tl::optional<SpeakerSetup> extractSpeakerSetup(juce::File const & file);
    [[nodiscard]] bool loadSpeakerSetup(juce::File file, LoadSpeakerSetupOption option);
    [[nodiscard]] bool
        loadProject(juce::File const & file, LoadProjectOption loadProjectOption, bool discardCurrentProject);
    [[nodiscard]] bool saveProject(tl::optional<juce::File> maybeFile);
    [[nodiscard]] bool saveSpeakerSetup(tl::optional<juce::File> maybeFile);
    [[nodiscard]] bool makeSureProjectIsSavedToDisk() noexcept;
    [[nodiscard]] bool makeSureSpeakerSetupIsSavedToDisk() noexcept;
    void setTitle() const;
    [[nodiscard]] static bool isDefaultFile(juce::File const & file) noexcept;
    [[nodiscard]] bool performSafeSave(juce::XmlElement const & content, juce::File const & destination) noexcept;

    //==============================================================================
    // OVERRIDES
    void audioParametersChanged() override;

    void getAllCommands(juce::Array<juce::CommandID> & commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo & result) override;
    [[nodiscard]] juce::ApplicationCommandTarget * getNextCommandTarget() override;
    [[nodiscard]] bool perform(juce::ApplicationCommandTarget::InvocationInfo const & info) override;
    //==============================================================================
    static void handleOpenManual();
    //==============================================================================
    JUCE_LEAK_DETECTOR(MainContentComponent)
};
