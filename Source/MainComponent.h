/*
 This file is part of spatServerGRIS.
 
 Developers: Nicolas Masson
 
 spatServerGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 spatServerGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with spatServerGRIS.  If not, see <http://www.gnu.org/licenses/>.
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

//============================


#include "../JuceLibraryCode/JuceHeader.h"

#include "GrisLookAndFeel.h"

#include "jackClientGRIS.h"
#include "jackServerGRIS.h"
#include "Speaker.h"
#include "SpeakerViewComponent.h"
#include "UiComponent.h"
#include "LevelComponent.h"
#include "OscInput.h"
#include "Input.h"
#include "WinControl.h"
#include "FileWriter.h"

class spatServerGRISApplication;


using namespace std;

static const unsigned int SizeWidthLevelComp   = 36;
static const unsigned int HertzRefreshNormal   = 24;
static const unsigned int HertzRefreshLowCpu   = 6;
static const unsigned int HertzRefresh2DLowCpu = 10;

//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
 your controls and content.
 */
class MainContentComponent   :  public Component,
                                public Button::Listener,
                                public TextEditor::Listener,
                                public Slider::Listener,
                                public ComboBox::Listener,
                                private Timer
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();
    bool exitApp();
    
    vector<Speaker *> getListSpeaker() { return this->listSpeaker; }
    mutex* getLockSpeakers(){ return this->lockSpeakers; }
    
    
    vector<Input *> getListSourceInput(){ return this->listSourceInput; }
    mutex* getLockInputs(){ return this->lockInputs; }
    void updateInputJack(int inInput, Input &inp);
    
    unsigned int    getBufferCurentIndex(){ return this->jackClient->indexRecord; }
    vector<float>   getBufferToRecord(int i){ return this->jackClient->buffersToRecord[i]; }
    vector<Client> *getListClientjack(){ return &this->jackClient->listClient; }
    void connectionClientJack(String nameCli, bool conn = true) {this->jackClient->connectionClient(nameCli, conn); }
    
    void selectSpeaker(int idS);
    void setNameConfig(String name);
    void setShowShepre(bool value){ this->speakerView->setShowSphere(value); }
    void addSpeaker();
    void savePresetSpeakers(String path);
    void removeSpeaker(int idSpeaker);
    void updateLevelComp();
    void muteInput(int id, bool mute);
    void muteOutput(int id, bool mute);
    
    void soloInput(int id, bool solo);
    void soloOutput(int id, bool solo);
    
    void saveJackSettings(unsigned int rate, unsigned int buff);
    //=======================================================================
    float getLevelsOut(int indexLevel){return (20.0f * log10f(sqrtf(this->jackClient->getLevelsOut(indexLevel))));}
    float getLevelsIn(int indexLevel){return (20.0f * log10f(sqrtf(this->jackClient->getLevelsIn(indexLevel)))); }
    

    void destroyWinSpeakConf() { this->winSpeakConfig = nullptr; this->jackClient->processBlockOn = true; }
    void destroyWinJackSetting() { this->winJackSetting = nullptr; }
    void destroyWinControl() { this->winControlSource = nullptr; }
    //=======================================================================
    void timerCallback() override;
    void paint (Graphics& g) override;
    
    void resized() override;
    void buttonClicked (Button *button) override;
    void sliderValueChanged (Slider *slider) override;
    void textEditorFocusLost (TextEditor &textEditor) override;
    void textEditorReturnKeyPressed (TextEditor &textEditor) override;
    void comboBoxChanged (ComboBox *comboBox) override;
    
    
private:

    
    Label*          addLabel(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    TextButton*     addButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    ToggleButton*   addToggleButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, bool toggle = false);
    TextEditor*     addTextEditor(const String &s, const String &emptyS, const String &stooltip, int x, int y, int w, int h, Component *into, int wLab = 80);
    Slider*         addSlider(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    ComboBox*       addComboBox(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    
    
    void openXmlFileSpeaker(String path);
    void openPreset(String path);
    void savePreset(String path);
    
    void updateSkeapersConf();
    
    ApplicationProperties applicationProperties;
    //==============================================================================
    jackClientGris *    jackClient;
    jackServerGRIS *    jackServer;
    
    FileWriter     *    fileWriter;
    
    vector<Speaker *>   listSpeaker;
    mutex *             lockSpeakers;
    
    vector<Input *>     listSourceInput;
    mutex *             lockInputs;
    OscInput *          oscReceiver;
    
    String nameConfig;
    String pathCurrentFileSpeaker;
    String pathCurrentPreset;
    
    //UI Components---------------------------
    TooltipWindow tooltipWindow;
    StretchableLayoutManager verticalLayout;
    ScopedPointer<StretchableLayoutResizerBar> verticalDividerBar;
    GrisLookAndFeel mGrisFeel;
    SpeakerViewComponent *speakerView;
    
    WindowEditSpeaker * winSpeakConfig;
    WindowJackSetting * winJackSetting;
    WinControl *        winControlSource;
    //3 Main Box---------------------
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
    
    ComboBox *      comBoxModeSpat;
    
    TextButton *    butLoadXMLSpeakers;
    TextButton *    butEditableSpeakers;
    TextButton *    butLoadPreset;
    TextButton *    butSavePreset;
    TextButton *    butShowWinControl;
    TextButton *    butDefaultColorIn;
    TextButton *    butJackParam;
    
    ToggleButton *  butShowSpeakerNumber;
    ToggleButton *  butHighPerformance;
    ToggleButton *  butNoiseSound;
    ToggleButton *  butHideSpeaker;
    
    Slider *        sliderMasterGainOut;
    
    TextEditor *    tedOSCInPort;
    Label *         labOSCStatus;
    
    TextEditor *    tedAddInputs;
    TextButton *    butAutoConnectJack;
    
    Label *         labelAllClients;
    BoxClient *     boxClientJack;
    
    //Record
    TextButton *    butStartRecord;
    TextEditor *    tedMinRecord;
    Label *         labelTimeRecorded;
    
    SplashScreen *  splash;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED


