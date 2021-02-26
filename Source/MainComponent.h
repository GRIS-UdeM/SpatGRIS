/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <optional>

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#define STRING2(x) #x
#define STRING(x) STRING2(x)

#include "AboutWindow.h"
#include "AudioProcessor.h"
#include "Box.h"
#include "Configuration.h"
#include "EditSpeakersWindow.h"
#include "FlatViewWindow.h"
#include "Input.h"
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
    // Jack client.
    std::unique_ptr<AudioProcessor> mAudioProcessor{};

    // Speakers.
    juce::Array<Triplet> mTriplets{};
    juce::OwnedArray<Speaker> mSpeakers{};
    juce::CriticalSection mSpeakersLock{};

    // Sources.
    juce::OwnedArray<Input> mInputs{};
    std::mutex mInputsLock{};

    // Open Sound Control.
    std::unique_ptr<OscInput> mOscReceiver{};

    // Paths.
    juce::String mConfigurationName{};
    juce::String mCurrentSpeakerSetupPath{};
    juce::String mCurrentPresetPath{};

    // Alsa output device
    juce::String mAlsaOutputDevice{};

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
    [[nodiscard]] bool isPresetModified() const;
    [[nodiscard]] bool exitApp();

    // Menubar handlers.
    void handleNew();
    void handleOpenPreset();
    void handleSavePreset();
    void handleSaveAsPreset();
    void handleOpenSpeakerSetup();
    void handleSaveAsSpeakerSetup(); // Called when closing the Speaker Setup Edition window.
    void handleShowSpeakerEditWindow();
    void handleShowPreferences();
    void handleShowAbout();
    static void handleOpenManual();
    void handleShow2DView();
    void handleShowNumbers();
    void setShowNumbers(bool state);
    void handleShowSpeakers();
    void setShowSpeakers(bool state);
    void handleShowTriplets();
    void setShowTriplets(bool state);
    [[nodiscard]] bool validateShowTriplets() const;
    void handleShowSourceLevel();
    void handleShowSpeakerLevel();
    void handleShowSphere();
    void handleResetInputPositions();
    void handleResetMeterClipping();
    void handleShowOscLogView();
    void handleInputColours();

    // other
    [[nodiscard]] bool isTripletsShown() const { return mIsTripletsShown; }
    [[nodiscard]] bool needToSaveSpeakerSetup() const { return mNeedToSaveSpeakerSetup; }
    [[nodiscard]] bool isSpanShown() const { return mIsSpanShown; }
    [[nodiscard]] bool isSourceLevelShown() const { return mIsSourceLevelShown; }
    [[nodiscard]] bool isSpeakerLevelShown() const { return mIsSpeakerLevelShown; }

    void setNeedToComputeVbap(bool const state) { mNeedToComputeVbap = state; }
    void setNeedToSaveSpeakerSetup(bool const state) { mNeedToSaveSpeakerSetup = state; }

    // Menubar methods.
    [[nodiscard]] juce::StringArray getMenuBarNames() override;
    [[nodiscard]] juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String & /*menuName*/) override;

    void menuItemSelected(int menuItemId, int /*topLevelMenuIndex*/) override;

    // Speakers.
    [[nodiscard]] juce::OwnedArray<Speaker> & getSpeakers() { return this->mSpeakers; }
    [[nodiscard]] juce::OwnedArray<Speaker> const & getSpeakers() const { return this->mSpeakers; }

    [[nodiscard]] juce::CriticalSection const & getSpeakersLock() const { return mSpeakersLock; }

    [[nodiscard]] Speaker * getSpeakerFromOutputPatch(output_patch_t out);
    [[nodiscard]] Speaker const * getSpeakerFromOutputPatch(output_patch_t out) const;

    void addSpeaker(int sortColumnId = 1, bool isSortedForwards = true);
    void insertSpeaker(int position, int sortColumnId, bool isSortedForwards);
    void removeSpeaker(int idSpeaker);
    void setDirectOut(int id, output_patch_t chn) const;
    void reorderSpeakers(std::vector<speaker_id_t> const & newOrder);
    void resetSpeakerIds();
    [[nodiscard]] speaker_id_t getMaxSpeakerId() const;
    [[nodiscard]] output_patch_t getMaxSpeakerOutputPatch() const;

    // Sources.
    [[nodiscard]] juce::OwnedArray<Input> & getSourceInputs() { return this->mInputs; }
    [[nodiscard]] juce::OwnedArray<Input> const & getSourceInputs() const { return this->mInputs; }

    [[nodiscard]] std::mutex & getInputsLock() { return this->mInputsLock; }

    void updateInputJack(int inInput, Input & inp) const;
    [[nodiscard]] bool isRadiusNormalized() const;

    // Jack clients.
    [[nodiscard]] AudioProcessor * getJackClient() { return mAudioProcessor.get(); }
    [[nodiscard]] AudioProcessor const * getJackClient() const { return mAudioProcessor.get(); }

    [[nodiscard]] std::vector<ClientData> & getClients() { return mAudioProcessor->getClients(); }
    [[nodiscard]] std::vector<ClientData> const & getClients() const { return mAudioProcessor->getClients(); }

    [[nodiscard]] std::mutex & getClientsLock() const { return mAudioProcessor->getClientsLock(); }

    void connectionClientJack(juce::String const & clientName, bool conn = true);

    // VBAP triplets.
    void setTripletsFromVbap();
    void clearTriplets() { this->mTriplets.clear(); }

    [[nodiscard]] juce::Array<Triplet> & getTriplets() { return this->mTriplets; }
    [[nodiscard]] juce::Array<Triplet> const & getTriplets() const { return this->mTriplets; }

    // Speaker selections.
    void selectSpeaker(speaker_id_t const idS) const;
    void selectTripletSpeaker(speaker_id_t const idS);
    [[nodiscard]] bool tripletExists(Triplet const & tri, int & pos) const;

    // Mute - solo.
    void muteInput(int id, bool mute) const;
    void muteOutput(output_patch_t id, bool mute) const;
    void soloInput(output_patch_t id, bool solo) const;
    void soloOutput(output_patch_t id, bool solo) const;

    // Input - output amplitude levels.
    [[nodiscard]] float getLevelsOut(int const indexLevel) const
    {
        return (20.0f * std::log10(this->mAudioProcessor->getLevelsOut(indexLevel)));
    }
    [[nodiscard]] float getLevelsIn(int const indexLevel) const
    {
        return (20.0f * std::log10(this->mAudioProcessor->getLevelsIn(indexLevel)));
    }
    [[nodiscard]] float getLevelsAlpha(int indexLevel) const;

    [[nodiscard]] float getSpeakerLevelsAlpha(int indexLevel) const;

    [[nodiscard]] auto const & getConfiguration() const { return mConfiguration; }

    // Called when the speaker setup has changed.
    bool updateLevelComp(); // TODO : what does the return value means ?

    // Open - save.
    void openXmlFileSpeaker(juce::File const & file);
    void reloadXmlFileSpeaker();
    void openPreset(juce::File const & file);
    void getPresetData(juce::XmlElement * xml) const;
    void savePreset(juce::String const & path);
    void saveSpeakerSetup(juce::String const & path);
    void saveProperties(juce::String const & audioDeviceType,
                        juce::String const & inputDevice,
                        juce::String const & outputDevice,
                        double const sampleRate,
                        int bufferSize,
                        RecordingFormat const recordingFormat,
                        RecordingConfig const recordingConfig,
                        int attenuationDbIndex,
                        int attenuationFrequencyIndex,
                        int oscPort);
    bool initRecording() const;
    void setNameConfig();
    void setTitle() const;

    // Screen refresh timer.
    void handleTimer(bool state);

    // Close windows other than the main one.
    void closeSpeakersConfigurationWindow();
    void closePropertiesWindow() { this->mPropertiesWindow.reset(); }
    void closeFlatViewWindow() { this->mFlatViewWindow = nullptr; }
    void closeAboutWindow() { this->mAboutWindow = nullptr; }
    void closeOscLogWindow() { this->mOscLogWindow = nullptr; }

    // Widget listener handlers.
    void timerCallback() override;
    void paint(juce::Graphics & g) override;
    void resized() override;
    void buttonClicked(juce::Button * button) override;
    void sliderValueChanged(juce::Slider * slider) override;
    void textEditorFocusLost(juce::TextEditor & textEditor) override;
    void textEditorReturnKeyPressed(juce::TextEditor & textEditor) override;
    void comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged) override;

    [[nodiscard]] SpatModes getModeSelected() const
    {
        return static_cast<SpatModes>(mSpatModeCombo->getSelectedId() - 1);
    }

    void setOscLogging(const juce::OSCMessage & message) const;

    //==============================================================================
    // Widget creation helper.
    juce::TextEditor * addTextEditor(juce::String const & s,
                                     juce::String const & emptyS,
                                     juce::String const & tooltip,
                                     int x,
                                     int y,
                                     int w,
                                     int h,
                                     juce::Component * into,
                                     int wLab = 80);

private:
    // Widget creation helpers.
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
    // The following methods implement the ApplicationCommandTarget interface, allowing
    // this window to publish a set of actions it can perform, and which can be mapped
    // onto menus, keypresses, etc.
    [[nodiscard]] juce::ApplicationCommandTarget * getNextCommandTarget() override
    {
        return findFirstTargetParentComponent();
    }

    void getAllCommands(juce::Array<juce::CommandID> & commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo & result) override;
    [[nodiscard]] bool perform(juce::ApplicationCommandTarget::InvocationInfo const & info) override;

protected:
    void audioParametersChanged() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(MainContentComponent)
};
