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

#include <JuceHeader.h>

#include "lib/tl/optional.hpp"

#include "AboutWindow.h"
#include "AbstractSpatAlgorithm.hpp"
#include "AudioProcessor.h"
#include "Box.h"
#include "Configuration.h"
#include "EditSpeakersWindow.h"
#include "FlatViewWindow.h"
#include "LayoutComponent.h"
#include "LogicStrucs.hpp"
#include "OscInput.h"
#include "OscLogWindow.h"
#include "OwnedMap.hpp"
#include "SettingsWindow.h"
#include "SpatModeComponent.h"
#include "SpeakerViewComponent.h"
#include "StrongTypes.hpp"
#include "Triplet.hpp"
#include "VuMeterComponent.h"
#include "constants.hpp"

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
    , public juce::Button::Listener
    , public juce::TextEditor::Listener
    , public juce::Slider::Listener
    , public juce::ComboBox::Listener
    , public SourceVuMeterComponent::Owner
    , public SpeakerVuMeterComponent::Owner
    , private AudioDeviceManagerListener
    , private juce::Timer
    , private SpatModeComponent::Listener
{
    enum class LoadProjectOption { removeInvalidDirectOuts, dontRemoveInvalidDirectOuts };

    juce::ReadWriteLock mLock{};

    std::unique_ptr<AudioProcessor> mAudioProcessor{};

    // Speakers.
    juce::Array<Triplet> mTriplets{};

    OwnedMap<source_index_t, SourceVuMeterComponent, MAX_NUM_SOURCES> mSourceVuMeterComponents{};
    OwnedMap<output_patch_t, SpeakerVuMeterComponent, MAX_NUM_SPEAKERS> mSpeakerVuMeters{};

    // Open Sound Control.
    std::unique_ptr<OscInput> mOscReceiver{};

    // Windows.
    std::unique_ptr<EditSpeakersWindow> mEditSpeakersWindow{};
    std::unique_ptr<SettingsWindow> mPropertiesWindow{};
    std::unique_ptr<FlatViewWindow> mFlatViewWindow{};
    std::unique_ptr<AboutWindow> mAboutWindow{};
    std::unique_ptr<OscLogWindow> mOscLogWindow{};

    // Sources Section
    std::unique_ptr<MainUiSection> mSourcesSection{};
    std::unique_ptr<LayoutComponent> mSourcesLayout{};

    // Speakers Section
    std::unique_ptr<MainUiSection> mSpeakersSection{};
    std::unique_ptr<LayoutComponent> mSpeakersLayout{};

    // Controls section
    std::unique_ptr<Box> mControlUiBox{};

    // Main ui
    std::unique_ptr<LayoutComponent> mMainLayout{};

    // Component in Box 3.
    std::unique_ptr<juce::Label> mCpuUsageLabel{};
    std::unique_ptr<juce::Label> mCpuUsageValue{};
    std::unique_ptr<juce::Label> mSampleRateLabel{};
    std::unique_ptr<juce::Label> mBufferSizeLabel{};
    std::unique_ptr<juce::Label> mChannelCountLabel{};

    SpatModeComponent mSpatModeComponent{ *this };

    std::unique_ptr<juce::Slider> mMasterGainOutSlider{};
    std::unique_ptr<juce::Slider> mInterpolationSlider{};

    std::unique_ptr<juce::TextEditor> mNumSourcesTextEditor{};

    std::unique_ptr<juce::TextButton> mStartRecordButton{};
    std::unique_ptr<juce::TextEditor> mMinRecordTextEditor{};
    std::unique_ptr<juce::Label> mTimeRecordedLabel{};
    std::unique_ptr<juce::TextButton> mInitRecordButton{};

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
    std::unique_ptr<AbstractSpatAlgorithm> mSpatAlgorithm{};

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

    void setLbapAttenuationDb(dbfs_t attenuation);
    void setLbapAttenuationFreq(hz_t freq);

    void refreshSourceVuMeterComponents();
    void refreshSpeakerVuMeterComponents();

    void removeInvalidDirectOuts();

    auto const & getLock() const { return mLock; }

    void handleSourcePositionChanged(source_index_t const sourceIndex,
                                     radians_t azimuth,
                                     radians_t elevation,
                                     float length,
                                     float newAzimuthSpan,
                                     float newZenithSpan);
    void resetSourcePosition(source_index_t sourceIndex);

    void loadDefaultSpeakerSetup(SpatMode spatMode);

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

    void handleSpatModeChanged(SpatMode spatMode) override;
    void handleMasterGainChanged(dbfs_t gain);
    void handleGainInterpolationChanged(float interpolation);
    void handleNewSpeakerPosition(output_patch_t outputPatch, CartesianVector const & position);
    void handleNewSpeakerPosition(output_patch_t outputPatch, PolarVector const & position);

    void updateAudioProcessor() const;
    void updateViewportConfig() const;

    void handleSetShowTriplets(bool state);

    void handleNumSourcesChanged(int numSources);

    // Speakers.
    [[nodiscard]] auto const & getSpeakersDisplayOrder() const { return mData.speakerSetup.order; }

    output_patch_t addSpeaker();
    output_patch_t insertSpeaker(int position);
    void removeSpeaker(output_patch_t outputPatch);
    void reorderSpeakers(juce::Array<output_patch_t> newOrder);

    [[nodiscard]] bool isRadiusNormalized() const;

    [[nodiscard]] AudioProcessor & getAudioProcessor() { return *mAudioProcessor; }
    [[nodiscard]] AudioProcessor const & getAudioProcessor() const { return *mAudioProcessor; }

    // Mute - solo.
    void setSourceState(source_index_t sourceIndex, PortState state);
    void setSpeakerState(output_patch_t outputPatch, PortState state);

    // Called when the speaker setup has changed.
    bool refreshSpeakers();

    // Open - save.
    void reloadXmlFileSpeaker();

    // Screen refresh timer.
    void handleTimer(bool state);
    void handleSaveAsSpeakerSetup();

    // Close windows other than the main one.
    void closeSpeakersConfigurationWindow();
    void closePropertiesWindow() { mPropertiesWindow.reset(); }
    void closeFlatViewWindow() { mFlatViewWindow.reset(); }
    void closeAboutWindow() { mAboutWindow.reset(); }
    void closeOscLogWindow() { mOscLogWindow.reset(); }
    //==============================================================================
    void timerCallback() override;
    void paint(juce::Graphics & g) override;
    void resized() override;
    void buttonClicked(juce::Button * button) override;
    void sliderValueChanged(juce::Slider * slider) override;
    void textEditorFocusLost(juce::TextEditor & textEditor) override;
    void textEditorReturnKeyPressed(juce::TextEditor & textEditor) override;
    void comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged) override;
    void menuItemSelected(int menuItemId, int /*topLevelMenuIndex*/) override;
    [[nodiscard]] juce::StringArray getMenuBarNames() override;
    [[nodiscard]] juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String & /*menuName*/) override;

    [[nodiscard]] bool isProjectModified() const;
    [[nodiscard]] bool isSpeakerSetupModified() const;

private:
    //==============================================================================
    // Widget creation helpers.
    juce::TextEditor * addTextEditor(juce::String const & s,
                                     juce::String const & emptyS,
                                     juce::String const & tooltip,
                                     int x,
                                     int y,
                                     int w,
                                     int h,
                                     juce::Component * into,
                                     int wLab = 80);
    juce::Label * addLabel(const juce::String & s,
                           const juce::String & tooltip,
                           int x,
                           int y,
                           int w,
                           int h,
                           Component * into) const;
    juce::TextButton *
        addButton(const juce::String & s, const juce::String & tooltip, int x, int y, int w, int h, Component * into);
    juce::ToggleButton * addToggleButton(const juce::String & s,
                                         const juce::String & tooltip,
                                         int x,
                                         int y,
                                         int w,
                                         int h,
                                         Component * into,
                                         bool toggle = false);
    juce::Slider *
        addSlider(const juce::String & s, const juce::String & tooltip, int x, int y, int w, int h, Component * into);
    juce::ComboBox *
        addComboBox(const juce::String & s, const juce::String & tooltip, int x, int y, int w, int h, Component * into);
    //==============================================================================
    // MenuBar handlers.
    void handleNewProject();
    void handleOpenProject();
    void handleSaveProject();
    void handleSaveAsProject();
    void handleOpenSpeakerSetup();
    void handleShowSpeakerEditWindow();
    void handleShowPreferences();
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
    void loadSpeakerSetup(juce::File const & file, tl::optional<SpatMode> forceSpatMode = tl::nullopt);
    void loadProject(juce::File const & file, LoadProjectOption loadProjectOption);
    void saveProject(juce::String const & path);
    void saveSpeakerSetup(juce::String const & path);
    void setTitle() const;

    [[nodiscard]] bool initRecording();
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
