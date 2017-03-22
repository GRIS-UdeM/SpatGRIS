/*
 This file is part of spatServerGRIS.
 
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

#include "../JuceLibraryCode/JuceHeader.h"

#include "../../GrisCommonFiles/GrisLookAndFeel.h"


#include "jackClientGRIS.h"
#include "jackServerGRIS.h"
#include "Speaker.h"
#include "SpeakerViewComponent.h"
#include "UiComponent.h"
#include "LevelComponent.h"
#include "OscInput.h"
#include "Input.h"

#ifndef USE_JACK
#define USE_JACK 1
#endif


using namespace std;


static const unsigned int sizeWidthLevelComp = 36;
static const unsigned int hertzRefresh = 30;

static inline float linearToDb(float linear) {
    return log10f(linear) * 20.f;
}

//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
 your controls and content.
 */
class MainContentComponent   :  public AudioAppComponent,
                                public Button::Listener,
                                public TextEditor::Listener,
                                private Timer
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();
    
    vector<Speaker *> getListSpeaker() { return this->listSpeaker; }
    mutex* getLockSpeakers(){ return this->lockSpeakers; }
    
    vector<LevelComponent *> getListLevelComp() { return this->listLevelComp; }
    mutex* getLockLevelComp(){ return this->lockLevelComp; }
    
    vector<Input *> getListSourceInput(){ return this->listSourceInput; }
    mutex* getLockInputs(){ return this->lockInputs; }
    
    
    void setShowShepre(bool value){ this->speakerView->setShowSphere(value); }
    void addSpeaker();
    void removeSpeaker(int idSpeaker);
    void updateLevelComp();
    //=======================================================================
    float getLevel(int indexLevel){return 0.85f;} //TODO
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    
    void releaseResources() override;
    
    void refreshWinSpeakConf(int r) { if(this->winSpeakConfig != nullptr){ this->winSpeakConfig->selectedRow(r); } }
    void destroyWinSpeakConf() { this->winSpeakConfig = nullptr; }
    //=======================================================================
    void timerCallback();
    void paint (Graphics& g) override;
    
    void resized() override;
    void buttonClicked (Button *button) override;
    void textEditorFocusLost (TextEditor &textEditor) override;
    void textEditorReturnKeyPressed (TextEditor &textEditor) override;
    
    
private:

    
    Label*          addLabel(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    TextButton*     addButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    ToggleButton*   addToggleButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, bool toggle = false);
    TextEditor*     addTextEditor(const String &s, const String &emptyS, const String &stooltip, int x, int y, int w, int h, Component *into, int wLab = 80);
    
    void openXmlFileSpeaker(String path);
    
    void updateSkeapersConf();
    //==============================================================================
    #if USE_JACK
    jackClientGris *jackClient;
    jackServerGRIS *jackServer;
    #endif
    
    
    vector<Speaker *> listSpeaker;
    mutex *lockSpeakers;
    
    vector<LevelComponent *> listLevelComp;
    mutex *lockLevelComp;
 
    String nameConfig;
    
    OscInput * oscReceiver;
    vector<Input *> listSourceInput;
    mutex *lockInputs;

    //UI Components---------------------------
    TooltipWindow tooltipWindow;
    StretchableLayoutManager verticalLayout;
    ScopedPointer<StretchableLayoutResizerBar> verticalDividerBar;
    GrisLookAndFeel mGrisFeel;
    SpeakerViewComponent *speakerView;
    
    WindowEditSpeaker* winSpeakConfig;
    //3 Main Box---------------------
    Box * boxInputsUI;
    Box * boxOutputsUI;
    Box * boxControlUI;
    
    //Component in Box 3 -----------
    Label *         labelJackStatus;
    TextButton *    butLoadXMLSpeakers;
    TextButton *    butEditableSpeakers;
    ToggleButton *  butShowSpeakerNumber;
    
    TextEditor *    tedOSCInPort;
    Label *         labOSCStatus;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED


