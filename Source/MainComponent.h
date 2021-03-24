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

#include <optional>

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

//#define STRING2(x) #x
//#define STRING(x) STRING2(x)

#include "AboutWindow.h"
#include "AudioProcessor.h"
#include "Box.h"
#include "Configuration.h"
#include "EditSpeakersWindow.h"
#include "FlatViewWindow.h"
#include "Input.h"
#include "Manager.hpp"
#include "OscInput.h"
#include "OscLogWindow.h"
#include "SettingsWindow.h"
#include "Speaker.h"
#include "SpeakerViewComponent.h"
#include "StrongTypes.hpp"

class MainWindow;

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
// This component lives inside our window, and this is where you should put all your controls and content.
class MainContentComponent final
    : public juce::Component
    , public juce::MenuBarModel
    , public juce::ApplicationCommandTarget
    , public juce::Button::Listener
    , public juce::TextEditor::Listener
    , public juce::Slider::Listener
    , public juce::ComboBox::Listener
    , private AudioDeviceManagerListener
    , private juce::Timer
{
    std::unique_ptr<AudioProcessor> mAudioProcessor{};

    // Speakers.
    juce::Array<Triplet> mTriplets{};
    Manager<Speaker, speaker_id_t> mSpeakers{};
    juce::Array<speaker_id_t> mSpeakersDisplayOrder{};

    // Sources.
    juce::OwnedArray<Input> mInputs{};
    std::mutex mInputsLock{};

    // Open Sound Control.
    std::unique_ptr<OscInput> mOscReceiver{};

    juce::String mConfigurationName{};

    juce::File mCurrentSpeakerSetup{};

    // Windows.
    std::unique_ptr<EditSpeakersWindow> mEditSpeakersWindow{};
    std::unique_ptr<SettingsWindow> mPropertiesWindow{};
    std::unique_ptr<FlatViewWindow> mFlatViewWindow{};
    std::unique_ptr<AboutWindow> mAboutWindow{};
    std::unique_ptr<OscLogWindow> mOscLogWindow{};

    // 3 Main Boxes.
    std::unique_ptr<Box> mMainUiBox{};
    std::unique_ptr<Box> mInputsUiBox{};
    std::unique_ptr<Box> mOutputsUiBox{};
    std::unique_ptr<Box> mControlUiBox{};

    // Component in Box 3.
    std::unique_ptr<juce::Label> mCpuUsageLabel{};
    std::unique_ptr<juce::Label> mCpuUsageValue{};
    std::unique_ptr<juce::Label> mSampleRateLabel{};
    std::unique_ptr<juce::Label> mBufferSizeLabel{};
    std::unique_ptr<juce::Label> mChannelCountLabel{};

    std::unique_ptr<juce::ComboBox> mSpatModeCombo{};

    std::unique_ptr<juce::Slider> mMasterGainOutSlider{};
    std::unique_ptr<juce::Slider> mInterpolationSlider{};

    std::unique_ptr<juce::TextEditor> mAddInputsTextEditor{};

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
    int mOscInputPort{ 18032 };
    unsigned mSamplingRate{ 48000u };

    Configuration mConfiguration;
    juce::Rectangle<int> mFlatViewWindowRect{};

    // Visual flags.
    bool mIsNumbersShown{ false };
    bool mIsSpeakersShown{ true };
    bool mIsSphereShown{ false };
    bool mIsSourceLevelShown{ false };
    bool mIsSpeakerLevelShown{ false };
    bool mIsTripletsShown{ false };
    bool mIsSpanShown{ true };

    // App states.
    bool mNeedToSavePreset{ false };
    bool mNeedToSaveSpeakerSetup{ false };
    bool mNeedToComputeVbap{ true };

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
    [[nodiscard]] bool exitApp() const;

    void setShowTriplets(bool state);

    // other
    [[nodiscard]] bool isTripletsShown() const { return mIsTripletsShown; }
    [[nodiscard]] bool needToSaveSpeakerSetup() const { return mNeedToSaveSpeakerSetup; }
    [[nodiscard]] bool isSpanShown() const { return mIsSpanShown; }
    [[nodiscard]] bool isSourceLevelShown() const { return mIsSourceLevelShown; }
    [[nodiscard]] bool isSpeakerLevelShown() const { return mIsSpeakerLevelShown; }

    void setNeedToComputeVbap(bool const state) { mNeedToComputeVbap = state; }
    void setNeedToSaveSpeakerSetup(bool const state) { mNeedToSaveSpeakerSetup = state; }

    // Speakers.
    [[nodiscard]] auto & getSpeakers() { return mSpeakers; }
    [[nodiscard]] auto const & getSpeakers() const { return mSpeakers; }

    [[nodiscard]] Speaker * getSpeakerFromOutputPatch(output_patch_t out);
    [[nodiscard]] Speaker const * getSpeakerFromOutputPatch(output_patch_t out) const;

    Speaker & addSpeaker();
    void insertSpeaker(int position);
    void removeSpeaker(speaker_id_t const id);
    void setDirectOut(int id, output_patch_t chn) const;
    void reorderSpeakers(juce::Array<speaker_id_t> newOrder);

    // Sources.
    [[nodiscard]] juce::OwnedArray<Input> & getSourceInputs() { return mInputs; }
    [[nodiscard]] juce::OwnedArray<Input> const & getSourceInputs() const { return mInputs; }

    [[nodiscard]] std::mutex & getInputsLock() { return mInputsLock; }

    [[nodiscard]] bool isRadiusNormalized() const;

    [[nodiscard]] AudioProcessor & getAudioProcessor() { return *mAudioProcessor; }
    [[nodiscard]] AudioProcessor const & getAudioProcessor() const { return *mAudioProcessor; }

    // VBAP triplets.
    [[nodiscard]] juce::Array<Triplet> & getTriplets() { return mTriplets; }
    [[nodiscard]] juce::Array<Triplet> const & getTriplets() const { return mTriplets; }

    // Speaker selections.
    void selectSpeaker(output_patch_t outputPatch);
    void selectTripletSpeaker(speaker_id_t idS);

    // Mute - solo.
    void muteInput(int id, bool mute) const;
    void muteOutput(output_patch_t id, bool mute) const;
    void soloInput(output_patch_t id, bool solo) const;
    void soloOutput(output_patch_t id, bool solo) const;

    // Input - output amplitude levels.
    [[nodiscard]] float getLevelsOut(int indexLevel) const;
    [[nodiscard]] float getLevelsIn(int indexLevel) const;
    [[nodiscard]] float getLevelsAlpha(int indexLevel) const;
    [[nodiscard]] float getSpeakerLevelsAlpha(int indexLevel) const;

    // Called when the speaker setup has changed.
    bool refreshSpeakers();

    // Open - save.
    void reloadXmlFileSpeaker();
    void saveProperties(juce::String const & audioDeviceType,
                        juce::String const & inputDevice,
                        juce::String const & outputDevice,
                        double sampleRate,
                        int bufferSize,
                        RecordingFormat recordingFormat,
                        RecordingConfig recordingConfig,
                        int attenuationDbIndex,
                        int attenuationFrequencyIndex,
                        int oscPort);

    // Screen refresh timer.
    void handleTimer(bool state);
    void handleSaveAsSpeakerSetup();

    // Close windows other than the main one.
    void closeSpeakersConfigurationWindow();
    void closePropertiesWindow() { mPropertiesWindow.reset(); }
    void closeFlatViewWindow() { mFlatViewWindow.reset(); }
    void closeAboutWindow() { mAboutWindow.reset(); }
    void closeOscLogWindow() { mOscLogWindow.reset(); }

    [[nodiscard]] auto const & getConfiguration() const { return mConfiguration; }
    [[nodiscard]] SpatMode getModeSelected() const;
    void setOscLogging(const juce::OSCMessage & message) const;
    void updateSourceData(int sourceDataIndex, Input & input) const;
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
    void handleNew();
    void handleOpenProject();
    void handleSaveProject() const;
    void handleSaveAsProject() const;
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
    void handleShowOscLogView();
    void handleInputColours();

    void setShowNumbers(bool state);
    void setShowSpeakers(bool state);
    void setTripletsFromVbap();

    [[nodiscard]] speaker_id_t getMaxSpeakerId() const;
    [[nodiscard]] output_patch_t getMaxSpeakerOutputPatch() const;
    [[nodiscard]] bool tripletExists(Triplet const & tri, int & pos) const;
    [[nodiscard]] bool validateShowTriplets() const;
    [[nodiscard]] bool isProjectModified() const;
    //==============================================================================
    // Open - save.
    void openXmlFileSpeaker(juce::File const & file, std::optional<SpatMode> forceSpatMode = std::nullopt);
    void openProject(juce::File const & file);
    void getProjectData(juce::XmlElement * xml) const;
    void saveProject(juce::String const & path) const;
    void saveSpeakerSetup(juce::String const & path);
    void setCurrentSpeakerSetup(juce::File const & file);
    void setTitle() const;

    [[nodiscard]] bool initRecording() const;
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
