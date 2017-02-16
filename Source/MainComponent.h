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
#include "jackClientGRIS.h"
#include "SpeakerViewComponent.h"


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
    
    ~MainContentComponent() {
        shutdownAudio();
    }
    
    //=======================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override {
    }
    
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override {
        bufferToFill.clearActiveBufferRegion();
        
        //sample code taken from juce 4.3.0 audio app example
        //        for (int chan = 0; chan < bufferToFill.buffer->getNumChannels(); ++chan) {
        //            phase = originalPhase;
        //            float* const channelData = bufferToFill.buffer->getWritePointer (chan, bufferToFill.startSample);
        //            for (int i = 0; i < bufferToFill.numSamples ; ++i) {
        //                channelData[i] = amplitude * std::sin (phase);
        //
        //                // increment the phase step for the next sample
        //                phase = std::fmod (phase + phaseDelta, float_Pi * 2.0f);
        //            }
        //        }
    }
    
    void releaseResources() override {
        // This will be called when the audio device stops, or when it is being
        // restarted due to a setting change.
        
        // For more details, see the help for AudioProcessor::releaseResources()
    }
    
    //=======================================================================
    void paint (Graphics& g) override {
        
    }
    
    void resized() override {
        
        Rectangle<int> r (getLocalBounds().reduced (5));
        
        // lay out the list box and vertical divider..
        Component* vcomps[] = { &speakerView, verticalDividerBar, nullptr };
        
        // lay out side-by-side and resize the components' heights as well as widths
        verticalLayout.layOutComponents (vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);
        
        r.removeFromLeft (verticalDividerBar->getRight());
        
        this->rightLabel.setBounds (r.removeFromBottom (26));
        r.removeFromBottom (8);
        
    }
    
    
private:
    //==============================================================================
    
    jackClientGris jackClient;
    
    StretchableLayoutManager verticalLayout;
    ScopedPointer<StretchableLayoutResizerBar> verticalDividerBar;
    
    Label rightLabel;
    SpeakerViewComponent speakerView;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
