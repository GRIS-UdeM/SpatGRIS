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

// Macros
#ifndef M_PI
    #define M_PI (3.14159265358979323846264338327950288)
#endif

#ifndef M2_PI
    #define M2_PI (6.28318530717958647692528676655900577)
#endif

#ifndef M_PI2
    #define M_PI2 (1.57079632679489661923132169163975143)
#endif

#ifndef M_PI4
    #define M_PI4 (0.785398163397448309615660845819875720)
#endif

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
#include "ServerGrisConstants.h"
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
public:
    MainContentComponent(MainWindow & parent);
    ~MainContentComponent() final;
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
    void handleOpenManual();
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

    // Menubar methods.
    juce::StringArray getMenuBarNames() final;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String & /*menuName*/) final;

    void menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/) final;

    // Speakers.
    juce::OwnedArray<Speaker> & getListSpeaker() { return this->listSpeaker; }
    juce::OwnedArray<Speaker> const & getListSpeaker() const { return this->listSpeaker; }

    std::mutex & getLockSpeakers() { return this->lockSpeakers; }

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
    juce::OwnedArray<Input> & getListSourceInput() { return this->listSourceInput; }
    juce::OwnedArray<Input> const & getListSourceInput() const { return this->listSourceInput; }

    std::mutex & getLockInputs() { return this->lockInputs; }

    void updateInputJack(int inInput, Input & inp);
    bool isRadiusNormalized() const;

    // Jack clients.
    JackClientGris * getJackClient() { return this->jackClient.get(); }
    JackClientGris const * getJackClient() const { return this->jackClient.get(); }

    std::mutex & getLockClients() { return this->jackClient->getClientsLock(); }

    std::vector<Client> & getListClientjack() { return this->jackClient->getClients(); }
    std::vector<Client> const & getListClientjack() const { return this->jackClient->getClients(); }

    void connectionClientJack(juce::String nameCli, bool conn = true);

    // VBAP triplets.
    void setListTripletFromVbap();
    void clearListTriplet() { this->listTriplet.clear(); }

    std::vector<Triplet> & getListTriplet() { return this->listTriplet; }
    std::vector<Triplet> const & getListTriplet() const { return this->listTriplet; }

    // Speaker selections.
    void selectSpeaker(unsigned int idS);
    void selectTripletSpeaker(int idS);
    bool tripletExist(Triplet tri, int & pos) const;

    // Mute - solo.
    void muteInput(int id, bool mute);
    void muteOutput(int id, bool mute);
    void soloInput(int id, bool solo);
    void soloOutput(int id, bool solo);

    // Input - output amplitude levels.
    float getLevelsOut(int indexLevel) const { return (20.0f * log10f(this->jackClient->getLevelsOut(indexLevel))); }
    float getLevelsIn(int indexLevel) const { return (20.0f * log10f(this->jackClient->getLevelsIn(indexLevel))); }
    float getLevelsAlpha(int indexLevel) const
    {
        float level = this->jackClient->getLevelsIn(indexLevel);
        if (level > 0.0001) { // -80 dB
            return 1.0;
        } else {
            return sqrtf(level * 10000.0f);
        }
    }
    float getSpeakerLevelsAlpha(int indexLevel) const
    {
        float level = this->jackClient->getLevelsOut(indexLevel);
        float alpha = 1.0;
        if (level > 0.001) { // -60 dB
            alpha = 1.0;
        } else {
            alpha = sqrtf(level * 1000.0f);
        }
        if (alpha < 0.6) {
            alpha = 0.6;
        }
        return alpha;
    }

    // Called when the speaker setup has changed.
    bool updateLevelComp();

    // Open - save.
    void openXmlFileSpeaker(juce::String path);
    void reloadXmlFileSpeaker();
    void openPreset(juce::String path);
    void getPresetData(juce::XmlElement * xml) const;
    void savePreset(juce::String path);
    void saveSpeakerSetup(juce::String path);
    void saveProperties(juce::String device,
                        int rate,
                        int buff,
                        int fileformat,
                        int fileconfig,
                        int attenuationDB,
                        int attenuationHz,
                        int oscPort);
    void chooseRecordingPath();
    void setNameConfig();
    void setTitle();

    // Screen refresh timer.
    void handleTimer(bool state)
    {
        if (state) {
            startTimerHz(24);
        } else {
            stopTimer();
        }
    }

    // Close windows other than the main one.
    void closeSpeakersConfigurationWindow()
    {
        this->winSpeakConfig.reset();
        this->jackClient->setProcessBlockOn(true);
    }
    void closePropertiesWindow() { this->windowProperties.reset(); }
    void closeFlatViewWindow() { this->flatViewWindowSource = nullptr; }
    void closeAboutWindow() { this->aboutWindow = nullptr; }
    void closeOscLogWindow() { this->oscLogWindow = nullptr; }

    // Widget listener handlers.
    void timerCallback() final;
    void paint(juce::Graphics & g) final;
    void resized() final;
    void buttonClicked(juce::Button * button) final;
    void sliderValueChanged(juce::Slider * slider) final;
    void textEditorFocusLost(juce::TextEditor & textEditor) final;
    void textEditorReturnKeyPressed(juce::TextEditor & textEditor) final;
    void comboBoxChanged(juce::ComboBox * comboBox) final;

    int getModeSelected() const { return this->comBoxModeSpat->getSelectedId() - 1; }

    void setOscLogging(const juce::OSCMessage & message);
    //==============================================================================
    // App user settings.
    int oscInputPort = 18032;
    unsigned int samplingRate = 48000;

    juce::ApplicationProperties applicationProperties;
    juce::Rectangle<int> flatViewWindowRect;

    // Visual flags.
    bool isSourceLevelShown;
    bool isSpeakerLevelShown;
    bool isTripletsShown;
    bool isSpanShown;

    // App states.
    bool needToSavePreset = false;
    bool needToSaveSpeakerSetup = false;
    bool needToComputeVbap = true;
    //==============================================================================
    // Widget creation helper.
    juce::TextEditor * addTextEditor(juce::String const & s,
                                     juce::String const & emptyS,
                                     juce::String const & stooltip,
                                     int x,
                                     int y,
                                     int w,
                                     int h,
                                     juce::Component * into,
                                     int wLab = 80);

private:
    //==============================================================================
    // Look-and-feel.
    GrisLookAndFeel mGrisFeel;
    SmallGrisLookAndFeel mSmallTextGrisFeel;

    MainWindow & parent;

    std::unique_ptr<juce::MenuBarComponent> menuBar;

    // Widget creation helpers.
    juce::Label *
        addLabel(const juce::String & s, const juce::String & stooltip, int x, int y, int w, int h, Component * into);
    juce::TextButton *
        addButton(const juce::String & s, const juce::String & stooltip, int x, int y, int w, int h, Component * into);
    juce::ToggleButton * addToggleButton(const juce::String & s,
                                         const juce::String & stooltip,
                                         int x,
                                         int y,
                                         int w,
                                         int h,
                                         Component * into,
                                         bool toggle = false);
    juce::Slider *
        addSlider(const juce::String & s, const juce::String & stooltip, int x, int y, int w, int h, Component * into);
    juce::ComboBox * addComboBox(const juce::String & s,
                                 const juce::String & stooltip,
                                 int x,
                                 int y,
                                 int w,
                                 int h,
                                 Component * into);

    // Jack server - client.
    std::unique_ptr<JackServerGris> jackServer;
    std::unique_ptr<JackClientGris> jackClient;

    // Speakers.
    std::vector<Triplet> listTriplet{};
    juce::OwnedArray<Speaker> listSpeaker{};

    std::mutex lockSpeakers{};

    // Sources.
    juce::OwnedArray<Input> listSourceInput{};

    std::mutex lockInputs{};

    // Open Sound Control.
    std::unique_ptr<OscInput> oscReceiver{};

    // Paths.
    juce::String nameConfig;
    juce::String pathLastVbapSpeakerSetup;
    juce::String pathCurrentFileSpeaker;
    juce::String pathCurrentPreset;

    // Alsa output device
    juce::String alsaOutputDevice;
    juce::Array<juce::String> alsaAvailableOutputDevices;

    // Windows.
    std::unique_ptr<EditSpeakersWindow> winSpeakConfig;
    std::unique_ptr<PropertiesWindow> windowProperties;
    std::unique_ptr<FlatViewWindow> flatViewWindowSource;
    std::unique_ptr<AboutWindow> aboutWindow;
    std::unique_ptr<OscLogWindow> oscLogWindow;

    // 3 Main Boxes.
    std::unique_ptr<Box> boxMainUI;
    std::unique_ptr<Box> boxInputsUI;
    std::unique_ptr<Box> boxOutputsUI;
    std::unique_ptr<Box> boxControlUI;

    // Component in Box 3.
    std::unique_ptr<juce::Label> labelJackStatus;
    std::unique_ptr<juce::Label> labelJackLoad;
    std::unique_ptr<juce::Label> labelJackRate;
    std::unique_ptr<juce::Label> labelJackBuffer;
    std::unique_ptr<juce::Label> labelJackInfo;

    std::unique_ptr<juce::ComboBox> comBoxModeSpat;

    std::unique_ptr<juce::Slider> sliderMasterGainOut;
    std::unique_ptr<juce::Slider> sliderInterpolation;

    std::unique_ptr<juce::TextEditor> tedAddInputs;

    std::unique_ptr<juce::Label> labelAllClients;
    std::unique_ptr<JackClientListComponent> jackClientListComponentJack;

    std::unique_ptr<juce::TextButton> butStartRecord;
    std::unique_ptr<juce::TextEditor> tedMinRecord;
    std::unique_ptr<juce::Label> labelTimeRecorded;
    std::unique_ptr<juce::TextButton> butInitRecord;

    // UI Components.
    std::unique_ptr<SpeakerViewComponent> speakerView;
    juce::StretchableLayoutManager verticalLayout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> verticalDividerBar;

    // App splash screen.
    std::unique_ptr<juce::SplashScreen> splash;

    // Flags.
    bool isProcessForeground;
    bool isNumbersShown;
    bool isSpeakersShown;
    bool isSphereShown;
    bool isRecording;

    //==============================================================================
    // The following methods implement the ApplicationCommandTarget interface, allowing
    // this window to publish a set of actions it can perform, and which can be mapped
    // onto menus, keypresses, etc.
    juce::ApplicationCommandTarget * getNextCommandTarget() final { return findFirstTargetParentComponent(); }

    void getAllCommands(juce::Array<juce::CommandID> & commands) final;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo & result) final;
    bool perform(juce::ApplicationCommandTarget::InvocationInfo const & info) final;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};
