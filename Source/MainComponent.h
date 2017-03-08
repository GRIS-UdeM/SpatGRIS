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
#include "Speaker.h"
#include "SpeakerViewComponent.h"


vector<Speaker *> listSpeaker;
using namespace std;
//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
 your controls and content.
 */
class MainContentComponent   : public AudioAppComponent
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();
    
    
    //vector<Speaker *> getListSpeaker();
    //=======================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    
    void releaseResources() override;
    
    
    //=======================================================================
    void paint (Graphics& g) override;
    
    void resized() override;
    void buttonClicked (Button *button);
    
    
private:
    //==============================================================================
    
    jackClientGris *jackClient;
    
    StretchableLayoutManager verticalLayout;
    ScopedPointer<StretchableLayoutResizerBar> verticalDividerBar;
    
    Label *rightLabel;
    SpeakerViewComponent *speakerView;
    
    GrisLookAndFeel mGrisFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent(){
    return new MainContentComponent();
}


#endif  // MAINCOMPONENT_H_INCLUDED
