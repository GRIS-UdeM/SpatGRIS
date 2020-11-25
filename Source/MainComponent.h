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

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#define STRING2(x) #x
#define STRING(x) STRING2(x)

#include "AboutWindow.h"
#include "Box.h"
#include "EditSpeakersWindow.h"
#include "FlatViewWindow.h"
#include "Input.h"
#include "JackClientGRIS.h"
#include "JackClientListComponent.h"
#include "JackServerGRIS.h"
#include "OscInput.h"
#include "OscLogWindow.h"
#include "PropertiesWindow.h"
#include "Speaker.h"
#include "SpeakerViewComponent.h"

class MainWindow;

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
    , private juce::Timer
{
    // Jack server - client.
    std::unique_ptr<JackServerGris> mJackServer{};
    JackClientGris mJackClient{};

    // Speakers.
    std::vector<Triplet> mTriplets{};
    juce::OwnedArray<Speaker> mSpeakers{};

    std::mutex mSpeakerLocks{};

    // Sources.
    juce::OwnedArray<Input> mSourceInputs{};

    std::mutex mInputLocks{};

    // Open Sound Control.
    std::unique_ptr<OscInput> mOscReceiver{};

    // Paths.
    juce::String mNameConfig{};
    juce::String mPathLastVbapSpeakerSetup{};
    juce::String mPathCurrentFileSpeaker{};
    juce::String mPathCurrentPreset{};

    // Alsa output device
    juce::String mAlsaOutputDevice{};
    juce::Array<juce::String> mAlsaAvailableOutputDevices{};

    // Windows.
    std::unique_ptr<EditSpeakersWindow> mEditSpeakersWindow{};
    std::unique_ptr<PropertiesWindow> mPropertiesWindow{};
    std::unique_ptr<FlatViewWindow> mFlatViewWindow{};
    std::unique_ptr<AboutWindow> mAboutWindow{};
    std::unique_ptr<OscLogWindow> mOscLogWindow{};

    // 3 Main Boxes.
    std::unique_ptr<Box> mMainUiBox{};
    std::unique_ptr<Box> mInputsUiBox{};
    std::unique_ptr<Box> mOutputsUiBox{};
    std::unique_ptr<Box> mControlUiBox{};

    // Component in Box 3.
    std::unique_ptr<juce::Label> mJackStatusLabel{};
    std::unique_ptr<juce::Label> mJackLoadLabel{};
    std::unique_ptr<juce::Label> mJackRateLabel{};
    std::unique_ptr<juce::Label> mJackBufferLabel{};
    std::unique_ptr<juce::Label> mJackInfoLabel{};

    std::unique_ptr<juce::ComboBox> mModeSpatCombo{};

    std::unique_ptr<juce::Slider> mMasterGainOutSlider{};
    std::unique_ptr<juce::Slider> mInterpolationSlider{};

    std::unique_ptr<juce::TextEditor> mAddInputsTextEditor{};

    std::unique_ptr<juce::Label> mAllClientsLabel{};
    std::unique_ptr<JackClientListComponent> mJackClientListComponent{};

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
    bool mIsNumbersShown{ false };
    bool mIsSpeakersShown{ true };
    bool mIsSphereShown{ false };
    bool mIsRecording{};
    //==============================================================================
    // Look-and-feel.
    GrisLookAndFeel & mLookAndFeel;
    SmallGrisLookAndFeel mSmallLookAndFeel{};

    MainWindow & mMainWindow;

    std::unique_ptr<juce::MenuBarComponent> mMenuBar{};
    //==============================================================================
    // App user settings.
    int mOscInputPort = 18032;
    unsigned int mSamplingRate = 48000;

    juce::ApplicationProperties mApplicationProperties{};
    juce::Rectangle<int> mFlatViewWindowRect{};

    // Visual flags.
    bool mIsSourceLevelShown{ false };
    bool mIsSpeakerLevelShown{};
    bool mIsTripletsShown{ false };
    bool mIsSpanShown{ true };

    // App states.
    bool mNeedToSavePreset{ false };
    bool mNeedToSaveSpeakerSetup{ false };
    bool mNeedToComputeVbap{ true };

public:
    //==============================================================================
    explicit MainContentComponent(MainWindow & mainWindow, GrisLookAndFeel & newLookAndFeel);
    //==============================================================================
    MainContentComponent() = delete;
    ~MainContentComponent() override;

    MainContentComponent(MainContentComponent const &) = delete;
    MainContentComponent(MainContentComponent &&) = delete;

    MainContentComponent & operator=(MainContentComponent const &) = delete;
    MainContentComponent & operator=(MainContentComponent &&) = delete;
    //==============================================================================
    // Exit application.
    bool isPresetModified() const;
    bool exitApp();

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
    bool validateShowTriplets() const;
    void handleShowSourceLevel();
    void handleShowSpeakerLevel();
    void handleShowSphere();
    void handleResetInputPositions();
    void handleResetMeterClipping();
    void handleShowOscLogView();
    void handleInputColours();

    // other
    bool isTripletsShown() const { return mIsTripletsShown; }
    bool needToSaveSpeakerSetup() const { return mNeedToSaveSpeakerSetup; }
    bool isSpanShown() const { return mIsSpanShown; }
    bool isSourceLevelShown() const { return mIsSourceLevelShown; }
    auto & getApplicationProperties() { return mApplicationProperties; }
    bool isSpeakerLevelShown() const { return mIsSpeakerLevelShown; }

    void setNeedToComputeVbap(bool const state) { mNeedToComputeVbap = state; }
    void setNeedToSaveSpeakerSetup(bool const state) { mNeedToSaveSpeakerSetup = state; }

    // Menubar methods.
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String & /*menuName*/) override;

    void menuItemSelected(int menuItemId, int /*topLevelMenuIndex*/) override;

    // Speakers.
    juce::OwnedArray<Speaker> & getSpeakers() { return this->mSpeakers; }
    juce::OwnedArray<Speaker> const & getSpeakers() const { return this->mSpeakers; }

    std::mutex & getSpeakersLock() { return this->mSpeakerLocks; }

    Speaker * getSpeakerFromOutputPatch(int out);
    Speaker const * getSpeakerFromOutputPatch(int out) const;

    void addSpeaker(int sortColumnId = 1, bool isSortedForwards = true);
    void insertSpeaker(int position, int sortColumnId, bool isSortedForwards);
    void removeSpeaker(int idSpeaker);
    void setDirectOut(int id, int chn);
    void reorderSpeakers(std::vector<int> const & newOrder);
    void resetSpeakerIds();
    int getMaxSpeakerId() const;
    int getMaxSpeakerOutputPatch() const;

    // Sources.
    juce::OwnedArray<Input> & getSourceInputs() { return this->mSourceInputs; }
    juce::OwnedArray<Input> const & getSourceInputs() const { return this->mSourceInputs; }

    std::mutex & getInputsLock() { return this->mInputLocks; }

    void updateInputJack(int inInput, Input & inp);
    bool isRadiusNormalized() const;

    // Jack clients.
    JackClientGris * getJackClient() { return &this->mJackClient; }
    JackClientGris const * getJackClient() const { return &mJackClient; }

    std::vector<Client> & getClients() { return this->mJackClient.getClients(); }
    std::vector<Client> const & getClients() const { return this->mJackClient.getClients(); }

    std::mutex & getClientsLock() { return mJackClient.getClientsLock(); }

    void connectionClientJack(juce::String const & clientName, bool conn = true);

    // VBAP triplets.
    void setTripletsFromVbap();
    void clearTriplets() { this->mTriplets.clear(); }

    std::vector<Triplet> & getTriplets() { return this->mTriplets; }
    std::vector<Triplet> const & getTriplets() const { return this->mTriplets; }

    // Speaker selections.
    void selectSpeaker(int const idS) const;
    void selectTripletSpeaker(int idS);
    bool tripletExists(Triplet const & tri, int & pos) const;

    // Mute - solo.
    void muteInput(int id, bool mute);
    void muteOutput(int id, bool mute);
    void soloInput(int id, bool solo);
    void soloOutput(int id, bool solo);

    // Input - output amplitude levels.
    float getLevelsOut(int const indexLevel) const
    {
        return (20.0f * log10f(this->mJackClient.getLevelsOut(indexLevel)));
    }
    float getLevelsIn(int const indexLevel) const
    {
        return (20.0f * log10f(this->mJackClient.getLevelsIn(indexLevel)));
    }
    float getLevelsAlpha(int indexLevel) const;

    float getSpeakerLevelsAlpha(int indexLevel) const;

    // Called when the speaker setup has changed.
    bool updateLevelComp();

    // Open - save.
    void openXmlFileSpeaker(juce::String const & path);
    void reloadXmlFileSpeaker();
    void openPreset(juce::String const & path);
    void getPresetData(juce::XmlElement * xml) const;
    void savePreset(juce::String const & path);
    void saveSpeakerSetup(juce::String const & path);
    void saveProperties(juce::String const & device,
                        int rate,
                        int buff,
                        int fileFormat,
                        int fileConfig,
                        int attenuationDb,
                        int attenuationHz,
                        int oscPort);
    void chooseRecordingPath();
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
    void comboBoxChanged(juce::ComboBox const * comboBox) override;

    ModeSpatEnum getModeSelected() const { return static_cast<ModeSpatEnum>(mModeSpatCombo->getSelectedId() - 1); }

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
    juce::ApplicationCommandTarget * getNextCommandTarget() override { return findFirstTargetParentComponent(); }

    void getAllCommands(juce::Array<juce::CommandID> & commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo & result) override;
    bool perform(juce::ApplicationCommandTarget::InvocationInfo const & info) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(MainContentComponent)
};
