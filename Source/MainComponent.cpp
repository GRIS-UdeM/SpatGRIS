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

    LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);
    
    listSpeaker = std::vector<Speaker *>();
    
    
    /*listSpeaker.push_back(new Speaker(glm::vec3(5,0.5,0)));
    listSpeaker.push_back(new Speaker(glm::vec3(5,3,0)));
    listSpeaker.push_back(new Speaker(glm::vec3(4,6,0)));
    listSpeaker.push_back(new Speaker(glm::vec3(3,8,0)));
    
    
    listSpeaker.push_back(new Speaker(glm::vec3(-5,0.5,0)));
    listSpeaker.push_back(new Speaker(glm::vec3(-5,3,0)));
    listSpeaker.push_back(new Speaker(glm::vec3(-4,6,0)));
    listSpeaker.push_back(new Speaker(glm::vec3(-3,8,0)));
    
    listSpeaker.push_back(new Speaker(glm::vec3(0,0.5,5)));
    listSpeaker.push_back(new Speaker(glm::vec3(0,3,5)));
    listSpeaker.push_back(new Speaker(glm::vec3(0,6,4)));
    listSpeaker.push_back(new Speaker(glm::vec3(0,8,3)));
    
    listSpeaker.push_back(new Speaker(glm::vec3(0,0.5,-5)));
    listSpeaker.push_back(new Speaker(glm::vec3(0,3,-5)));
    listSpeaker.push_back(new Speaker(glm::vec3(0,6,-4)));
    listSpeaker.push_back(new Speaker(glm::vec3(0,8,-3)));
    
    
    
    listSpeaker.push_back(new Speaker(glm::vec3(5,0.5,5)));
    listSpeaker.push_back(new Speaker(glm::vec3(5,3,5)));
    listSpeaker.push_back(new Speaker(glm::vec3(4,6,4)));
    listSpeaker.push_back(new Speaker(glm::vec3(3,8,3)));
    
    listSpeaker.push_back(new Speaker(glm::vec3(5,0.5,-5)));
    listSpeaker.push_back(new Speaker(glm::vec3(5,3,-5)));
    listSpeaker.push_back(new Speaker(glm::vec3(4,6,-4)));
    listSpeaker.push_back(new Speaker(glm::vec3(3,8,-3)));
    
    
    
    listSpeaker.push_back(new Speaker(glm::vec3(-5,0.5,5)));
    listSpeaker.push_back(new Speaker(glm::vec3(-5,3,5)));
    listSpeaker.push_back(new Speaker(glm::vec3(-4,6,4)));
    listSpeaker.push_back(new Speaker(glm::vec3(-3,8,3)));
    
    listSpeaker.push_back(new Speaker(glm::vec3(-5,0.5,-5)));
    listSpeaker.push_back(new Speaker(glm::vec3(-5,3,-5)));
    listSpeaker.push_back(new Speaker(glm::vec3(-4,6,-4)));
    listSpeaker.push_back(new Speaker(glm::vec3(-3,8,-3)));*/
    
    
   
    
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

    
    
    openXmlFile("/Users/gris/Documents/GRIS/zirkonium/ZirkSpeakers_Dome 16 UdeM.xml");
    //Start JACK
    /*this->jackClient = new jackClientGris();
    
    if(!jackClient->isReady()){
        this->rightLabel->setText("jackClient Not Connected", NotificationType::dontSendNotification);
    }*/
       
}

MainContentComponent::~MainContentComponent() {
    //elete this->jackClient;
    for (std::vector< Speaker * >::iterator it = listSpeaker.begin() ; it != listSpeaker.end(); ++it)
    {
        delete (*it);
    }
    listSpeaker.clear();
    
   
    delete this->rightLabel;
    delete this->speakerView;
    //shutdownAudio();
}

/*
vector<Speaker *> MainContentComponent::getListSpeaker() {
    return this->listSpeaker;
}*/


void MainContentComponent::openXmlFile(String path)
{
    XmlDocument myDocument (File (path.toStdString()));
    ScopedPointer<XmlElement> mainElement (myDocument.getDocumentElement());
    if (mainElement == nullptr)
    {
        String error = myDocument.getLastParseError();
    }
    else
    {
        if(mainElement->hasTagName("SpeakerSetup")){
            nameConfig =  mainElement->getStringAttribute("Name");
            cout << nameConfig << newLine;
            this->speakerView->setNameConfig(nameConfig);
            
            forEachXmlChildElement (*mainElement, ring)
            {
                if (ring->hasTagName ("Ring"))
                {
                    forEachXmlChildElement (*ring, spk)
                    {
                        if (spk->hasTagName ("Speaker"))
                        {
                            
                            listSpeaker.push_back(new Speaker(glm::vec3(spk->getDoubleAttribute("PositionX")*10.0f,
                                                                        spk->getDoubleAttribute("PositionZ")*10.0f,
                                                                        spk->getDoubleAttribute("PositionY")*10.0f)));
                 

                        }
                    }
                    
                }
            }
            
        }
        
    }
    
    for (std::vector< Speaker * >::iterator it = listSpeaker.begin() ; it != listSpeaker.end(); ++it)
    {
        this->addAndMakeVisible(*it);
    }
    this->resized();
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
    g.fillAll (mGrisFeel.getWinBackgroundColour());
}


void MainContentComponent::buttonClicked (Button *button)
{
    
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
    
    /*if(listSpeaker.size()!=0){
        std::vector<Speaker *>::iterator iter = listSpeaker.begin();
        delete *iter;
        listSpeaker.erase(iter);
    }*/
    for (std::vector< Speaker * >::iterator it = listSpeaker.begin() ; it != listSpeaker.end(); ++it)
    {
        (*it)->setBounds(r.removeFromTop(26));
    }
    
}

