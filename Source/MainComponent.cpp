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
//#include "SpeakerViewComponent.cpp"


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   : public AudioAppComponent
{
public:
    //==============================================================================
    MainContentComponent() :
    leftLabel (String::empty, "LEFT"),
    rightLabel (String::empty, "RIGHT")
    {
        

        
        //add the components
        addAndMakeVisible (leftLabel);
        addAndMakeVisible (rightLabel);
        
        
        // set up the layout and resizer bars
        verticalLayout.setItemLayout (0, -0.2, -0.8, -0.35); // width of the font list must be between 20% and 80%, preferably 50%
        verticalLayout.setItemLayout (1, 8, 8, 8);           // the vertical divider drag-bar thing is always 8 pixels wide
        verticalLayout.setItemLayout (2, 150, -1.0, -0.65);  // the components on the right must be at least 150 pixels wide, preferably 50% of the total width
        verticalDividerBar = new StretchableLayoutResizerBar (&verticalLayout, 1, true);
        addAndMakeVisible (verticalDividerBar);

        
        
        
        
        
        // #1: this is not working with jack with non-built-in audio devices
        //        setAudioChannels (2, 2);
        
        setSize (1200, 600);
        
    }

    ~MainContentComponent()
    {
        shutdownAudio();
    }

    //=======================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        // This function will be called when the audio device is started, or when
        // its settings (i.e. sample rate, block size, etc) are changed.

        // You can use this function to initialise any resources you might need,
        // but be careful - it will be called on the audio thread, not the GUI thread.

        // For more details, see the help for AudioProcessor::prepareToPlay()
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        // Your audio-processing code goes here!

        // For more details, see the help for AudioProcessor::getNextAudioBlock()

        // Right now we are not producing any data, in which case we need to clear the buffer
        // (to prevent the output of random noise)
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
    
    void releaseResources() override
    {
        // This will be called when the audio device stops, or when it is being
        // restarted due to a setting change.

        // For more details, see the help for AudioProcessor::releaseResources()
    }

    //=======================================================================
    void paint (Graphics& g) override
    {

        // You can add your drawing code here!
    }

    void resized() override {
        
        Rectangle<int> r (getLocalBounds().reduced (5));
        
        // lay out the list box and vertical divider..
        Component* vcomps[] = { &leftLabel, verticalDividerBar, nullptr };
        
        // lay out side-by-side and resize the components' heights as well as widths
        verticalLayout.layOutComponents (vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);
        
        
        r.removeFromLeft (verticalDividerBar->getRight());
        
        rightLabel.setBounds (r.removeFromBottom (26));
        r.removeFromBottom (8);
        
    }
    
    
private:
    //==============================================================================

    jackClientGris jackClient;
    
    StretchableLayoutManager verticalLayout;
    ScopedPointer<StretchableLayoutResizerBar> verticalDividerBar;
    
    Label leftLabel, rightLabel;
    SpeakerViewComponent speakerView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
