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
#include "MainComponent.h"

MainContentComponent::MainContentComponent(){

    this->rightLabel = new Label();
    this->rightLabel->setText("RIGHT", NotificationType::dontSendNotification);
    
    this->speakerView= new SpeakerViewComponent();
    
    this->addAndMakeVisible (speakerView);
    this->addAndMakeVisible(this->rightLabel);

    // set up the layout and resizer bars
    this->verticalLayout.setItemLayout (0, -0.2, -0.8, -0.5); // width of the font list must be between 20% and 80%, preferably 50%
    this->verticalLayout.setItemLayout (1, 8, 8, 8);           // the vertical divider drag-bar thing is always 8 pixels wide
    this->verticalLayout.setItemLayout (2, 150, -1.0, -0.65);  // the components on the right must be at least 150 pixels wide, preferably 50% of the total width
    this->verticalDividerBar = new StretchableLayoutResizerBar (&verticalLayout, 1, true);
    this->addAndMakeVisible (verticalDividerBar);

    
    
    
    // #1: this is not working with jack with non-built-in audio devices
    //        setAudioChannels (2, 2);
    
    this->setSize (1200, 600);

    

    
    //Start JACK
    /*this->jackClient = new jackClientGris();
    
    if(!jackClient->isReady()){
        this->rightLabel->setText("jackClient Not Connected", NotificationType::dontSendNotification);
    }*/
       
}

MainContentComponent::~MainContentComponent() {
    //elete this->jackClient;
    
    delete this->rightLabel;
    delete this->speakerView;
    //shutdownAudio();
}

void MainContentComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate) {
    
}

void MainContentComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) {
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

void MainContentComponent::releaseResources() {
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.
    
    // For more details, see the help for AudioProcessor::releaseResources()
}

void MainContentComponent::paint (Graphics& g) {
    
}

void MainContentComponent::resized() {
    
    Rectangle<int> r (getLocalBounds().reduced (5));
    
    // lay out the list box and vertical divider..
    Component* vcomps[] = { speakerView, verticalDividerBar, nullptr };
    
    // lay out side-by-side and resize the components' heights as well as widths
    verticalLayout.layOutComponents (vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);
    
    r.removeFromLeft (verticalDividerBar->getRight());
    
    this->rightLabel->setBounds (r.removeFromBottom (26));
    r.removeFromBottom (8);
    
}

