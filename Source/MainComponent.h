/*
 This file is part of ServerGris.
 
 Developers: Olivier Belanger, Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

//Macro-----------------------
#ifndef M_PI
#define M_PI    (3.14159265358979323846264338327950288)
#endif
//--
#ifndef M2_PI
#define M2_PI   (6.28318530717958647692528676655900577)
#endif
//--
#ifndef M_PI2
#define M_PI2   (1.57079632679489661923132169163975143)
#endif
//--
#ifndef M_PI4
#define M_PI4   (0.785398163397448309615660845819875720)
#endif

#define STRING2(x) #x
#define STRING(x) STRING2(x)

//============================


#include "../JuceLibraryCode/JuceHeader.h"

#include "GrisLookAndFeel.h"
#include "SmallTextGrisLookAndFeel.h"

#include "jackClientGRIS.h"
#include "jackServerGRIS.h"
#include "Speaker.h"
#include "SpeakerViewComponent.h"
#include "UiComponent.h"
#include "LevelComponent.h"
#include "OscInput.h"
#include "Input.h"
#include "WinControl.h"
#include "MainWindow.h"

using namespace std;

static const unsigned int SizeWidthLevelComp   = 22;
static const unsigned int HertzRefreshNormal   = 24;
static const unsigned int HertzRefreshLowCpu   = 6;
static const unsigned int HertzRefresh2DLowCpu = 10;

//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
 your controls and content.
 */
class MainContentComponent   :  public Component,
                                public MenuBarModel,
                                public ApplicationCommandTarget,
                                public Button::Listener,
                                public TextEditor::Listener,
                                public Slider::Listener,
                                public ComboBox::Listener,
                                private Timer
{
public:
    //==============================================================================
    MainContentComponent(DocumentWindow *parent);
    ~MainContentComponent();
    bool exitApp();

    //==============================================================================
    void handleNew();
    void handleOpenPreset();
    void handleSavePreset();
    void handleSaveAsPreset();
    void handleOpenSpeakerSetup();
    void handleSaveSpeakerSetup();
    void handleSaveAsSpeakerSetup();
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
    bool validateShowTriplets();
    void handleShowSourceLevel();
    void handleShowSpeakerLevel();
    void handleShowSphere();
    void handleHighPerformance();
    void setHighPerformance(bool state);
    void handleResetInputPositions();
    void handleInputColours();

    //==============================================================================
    StringArray getMenuBarNames() override {
        const char* const names[] = { "File", "View", "Help", nullptr };
        return StringArray (names);
    }

    PopupMenu getMenuForIndex (int menuIndex, const String& /*menuName*/) override;
    void menuItemSelected (int menuItemID, int /*topLevelMenuIndex*/) override;

    Speaker * getSpeakerFromOutputPatch(int out);
    vector<Speaker *> getListSpeaker() { return this->listSpeaker; }
    mutex* getLockSpeakers(){ return this->lockSpeakers; }
        
    vector<Input *> getListSourceInput(){ return this->listSourceInput; }
    mutex* getLockInputs(){ return this->lockInputs; }
    void updateInputJack(int inInput, Input &inp);

    jackClientGris * getJackClient() { return this->jackClient; }
    
    mutex* getLockClients(){ return &this->jackClient->lockListClient; }
    vector<Client> *getListClientjack(){ return &this->jackClient->listClient; }
    void connectionClientJack(String nameCli, bool conn = true);

    void setListTripletFromVbap();
    vector<Triplet> getListTriplet(){ return this->listTriplet; }
    void clearListTriplet(){ this->listTriplet.clear(); }
    void selectSpeaker(unsigned int idS);
    void selectTripletSpeaker(int idS);
    void setNameConfig();
    void addSpeaker();
    void saveSpeakerSetup(String path);
    void removeSpeaker(int idSpeaker);
    bool updateLevelComp();
    void muteInput(int id, bool mute);
    void muteOutput(int id, bool mute);
    
    void soloInput(int id, bool solo);
    void soloOutput(int id, bool solo);
    void setDirectOut(int id, int chn);

    void saveProperties(int rate, int buff, int fileformat, int oscPort);

    void handleTimer(bool state) {
        if (state) {
            startTimerHz(HertzRefreshNormal);
        } else {
            stopTimer();
        }
    }

    //=======================================================================
    float getLevelsOut(int indexLevel) { return (20.0f * log10f(this->jackClient->getLevelsOut(indexLevel))); }
    float getLevelsIn(int indexLevel) { return (20.0f * log10f(this->jackClient->getLevelsIn(indexLevel))); }
    float getLevelsAlpha(int indexLevel) {
        float level = this->jackClient->getLevelsIn(indexLevel);
        if (level > 0.0001) { // -80 dB
            return 1.0;
        } else {
            return sqrtf(level * 10000.0f);
        }
    }

    float getSpeakerLevelsAlpha(int indexLevel) {
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

    void chooseRecordingPath();

    void destroyWinSpeakConf() { this->winSpeakConfig = nullptr; this->jackClient->processBlockOn = true; }
    void destroyWindowProperties() { this->windowProperties = nullptr; }
    void destroyWinControl() { this->winControlSource = nullptr; }
    void destroyAboutWindow() { this->aboutWindow = nullptr; }

    //=======================================================================
    void timerCallback() override;
    void paint (Graphics& g) override;
    
    void resized() override;
    void buttonClicked (Button *button) override;
    void sliderValueChanged (Slider *slider) override;
    void textEditorFocusLost (TextEditor &textEditor) override;
    void textEditorReturnKeyPressed (TextEditor &textEditor) override;
    void comboBoxChanged (ComboBox *comboBox) override;

    void setTitle();

    void loadVbapHrtfSpeakerSetup();

    String getCurrentFileSpeakerPath();

    ApplicationProperties applicationProperties;

    bool isSourceLevelShown = false;
    bool isSpeakerLevelShown = false;
    bool isTripletsShown = false;
    bool isSpanShown;

    bool needToSaveSpeakerSetup = false;
    bool needToComputeVbap = true;

    int oscInputPort = 18032;
    unsigned int samplingRate = 48000;

    TextEditor* addTextEditor(const String &s, const String &emptyS, const String &stooltip, int x, int y, int w, int h, Component *into, int wLab = 80);

    Rectangle<int> winControlRect;

private:

    DocumentWindow *parent;

    ScopedPointer<MenuBarComponent> menuBar;

    Label*          addLabel(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    TextButton*     addButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    ToggleButton*   addToggleButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, bool toggle = false);
    Slider*         addSlider(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    ComboBox*       addComboBox(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    

    void onOpenPreset();
    void openXmlFileSpeaker(String path);
    void openPreset(String path);
    void savePreset(String path);
    
    void updateSkeapersConf();
    bool tripletExist(Triplet tri, int &pos);
    
    //==============================================================================
    jackClientGris *    jackClient;
    jackServerGRIS *    jackServer;

    vector<Triplet>     listTriplet;
    vector<Speaker *>   listSpeaker;
    mutex *             lockSpeakers;
    
    vector<Input *>     listSourceInput;
    mutex *             lockInputs;
    OscInput *          oscReceiver;
    
    String nameConfig;
    String pathLastVbapSpeakerSetup;
    String pathCurrentFileSpeaker;
    String pathCurrentPreset;
    
    //UI Components---------------------------
    TooltipWindow tooltipWindow;
    StretchableLayoutManager verticalLayout;
    ScopedPointer<StretchableLayoutResizerBar> verticalDividerBar;
    GrisLookAndFeel mGrisFeel;
    SmallTextGrisLookAndFeel mSmallTextGrisFeel;
    SpeakerViewComponent *speakerView;
    
    WindowEditSpeaker * winSpeakConfig;
    WindowProperties * windowProperties;
    WinControl * winControlSource;
    AboutWindow * aboutWindow;

    //3 Main Box---------------------
    Box * boxMainUI;
    Box * boxInputsUI;
    Box * boxOutputsUI;
    Box * boxControlUI;
    
    //Component in Box 3 -----------
    //Jack
    Label *         labelJackStatus;
    Label *         labelJackLoad;
    Label *         labelJackRate;
    Label *         labelJackBuffer;
    Label *         labelJackInfo;
    Label *         labelModeInfo;
    
    ComboBox *      comBoxModeSpat;
    
    TextButton *    butAutoConnectJack;
    TextButton *    butDisconnectAllJack;

    Slider *        sliderMasterGainOut;
    Slider *        sliderInterpolation;
    
    TextEditor *    tedAddInputs;
        
    Label *         labelAllClients;
    BoxClient *     boxClientJack;
    
    //Record
    TextButton *    butStartRecord;
    TextEditor *    tedMinRecord;
    Label *         labelTimeRecorded;
    TextButton *    butInitRecord;

    SplashScreen *  splash;

    bool isProcessForeground;

    bool isNumbersShown;
    bool isSpeakersShown;
    bool isSphereShown;
    bool isHighPerformance;

    //==============================================================================
    // The following methods implement the ApplicationCommandTarget interface, allowing
    // this window to publish a set of actions it can perform, and which can be mapped
    // onto menus, keypresses, etc.

    ApplicationCommandTarget* getNextCommandTarget() override
    {
        // this will return the next parent component that is an ApplicationCommandTarget (in this
        // case, there probably isn't one, but it's best to use this method in your own apps).
        return findFirstTargetParentComponent();
    }

    void getAllCommands (Array<CommandID>& commands) override;

    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;

    bool perform (const InvocationInfo& info) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED


