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
    
    
    this->listSpeaker = vector<Speaker *>();
    this->listLevelComp = vector<LevelComponent *>();

    this->lockSpeakers = new mutex();
    //SpeakerViewComponent 3D VIEW------------------------------
    this->speakerView= new SpeakerViewComponent(this);
    this->addAndMakeVisible (speakerView);
    
    //BOX Inputs------------------------------------------------
    this->boxInputsUI = new Box(&mGrisFeel, "Inputs~");
    addAndMakeVisible(this->boxInputsUI);
    
    //BOX Outputs-----------------------------------------------
    this->boxOutputsUI = new Box(&mGrisFeel, "Outputs~");
    addAndMakeVisible(this->boxOutputsUI);
    
    //BOX Control-----------------------------------------------
    this->boxControlUI = new Box(&mGrisFeel);
    addAndMakeVisible(this->boxControlUI);

    
    this->labelJackStatus = new Label();
    this->labelJackStatus->setText("Jack Unknown", NotificationType::dontSendNotification);
    this->labelJackStatus->setFont(mGrisFeel.getFont());
    this->labelJackStatus->setLookAndFeel(&mGrisFeel);
    this->labelJackStatus->setColour(Label::textColourId, mGrisFeel.getFontColour());
    this->labelJackStatus->setBounds(0, 0, 150, 28);
    this->boxControlUI->getContent()->addAndMakeVisible(this->labelJackStatus);
    

    // set up the layout and resizer bars
    this->verticalLayout.setItemLayout (0, -0.2, -0.8, -0.5); // width of the font list must be between 20% and 80%, preferably 50%
    this->verticalLayout.setItemLayout (1, 8, 8, 8);           // the vertical divider drag-bar thing is always 8 pixels wide
    this->verticalLayout.setItemLayout (2, 150, -1.0, -0.65);  // the components on the right must be at least 150 pixels wide, preferably 50% of the total width
    this->verticalDividerBar = new StretchableLayoutResizerBar (&verticalLayout, 1, true);
    this->addAndMakeVisible (verticalDividerBar);

    this->setSize (1360, 760);
    
    

#if USE_JACK
    // #1: this is not working with jack with non-built-in audio devices
    //        setAudioChannels (2, 2);
    
    //Start JACK
    this->jackClient = new jackClientGris();
    
    if(!jackClient->isReady()){
        this->labelJackStatus->setText("jackClient Not Connected", NotificationType::dontSendNotification);
    }
#endif
    
    
    openXmlFile("/Users/gris/Documents/GRIS/zirkonium/ZirkSpeakers_Dome 16 UdeM.xml");
    
    for(int i = 1; i <= 64; i ++){
        this->listLevelComp.push_back(new LevelComponent(this, &mGrisFeel, i));
    }
    
    
    
    int x = 2;
    Component *compBoxInputs = this->boxOutputsUI->getContent();
    for (auto&& it : listLevelComp)
    {
        juce::Rectangle<int> level(x, 4, sizeWidthLevelComp, 200);
        it->setBounds(level);
        compBoxInputs->addAndMakeVisible(it);
        x+=sizeWidthLevelComp;
    }
   
    this->resized();
}

MainContentComponent::~MainContentComponent() {
    
    delete this->boxInputsUI;
    delete this->boxOutputsUI;
    delete this->boxControlUI;
    
    delete this->labelJackStatus;
    delete this->speakerView;
    
    this->lockSpeakers->lock();
    for (auto&& it : listSpeaker)
    {
        delete (it);
    }
    listSpeaker.clear();
    this->lockSpeakers->unlock();
    delete this->lockSpeakers;
    
    
    for (auto&& it : listLevelComp)
    {
        delete (it);
    }
    listLevelComp.clear();
    
    
    #if USE_JACK
    shutdownAudio();
    delete  this->jackClient;
    #endif
}



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
                            
                            listSpeaker.push_back(new Speaker(this, spk->getIntAttribute("LayoutIndex"),
                                                              spk->getIntAttribute("OutputPatch"),
                                                              glm::vec3(spk->getDoubleAttribute("PositionX")*10.0f,
                                                                        spk->getDoubleAttribute("PositionZ")*10.0f,
                                                                        spk->getDoubleAttribute("PositionY")*10.0f)));
                 

                        }
                    }
                    
                }
            }
            
        }
        
    }
    

    int y = 0;
    Component *compBoxInputs = this->boxInputsUI->getContent();
    
    for (auto&& it : listSpeaker)
    {
        juce::Rectangle<int> boundsSpeak(0, y,550, 26);
        (it)->setBounds(boundsSpeak);
        compBoxInputs->addAndMakeVisible(it);
        y+=28;
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
    this->listLevelComp[0]->setVisible(false);
}

void MainContentComponent::resized() {
    
    Rectangle<int> r (getLocalBounds().reduced (2));
    
    // lay out the list box and vertical divider..
    Component* vcomps[] = { speakerView, verticalDividerBar, nullptr };
    
    // lay out side-by-side and resize the components' heights as well as widths
    verticalLayout.layOutComponents (vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);
    

    this->boxInputsUI->setBounds(speakerView->getWidth()+6, 2, getWidth()-(speakerView->getWidth()+10),240);
    this->boxInputsUI->correctSize(this->boxInputsUI->getWidth()-8, 450);

    this->boxOutputsUI->setBounds(speakerView->getWidth()+6, 244, getWidth()-(speakerView->getWidth()+10),240);
    this->boxOutputsUI->correctSize((listLevelComp.size()*(sizeWidthLevelComp))+4, 210);
    
    this->boxControlUI->setBounds(speakerView->getWidth()+6, 488, getWidth()-(speakerView->getWidth()+10), getHeight()-490);
    this->boxControlUI->correctSize(this->boxControlUI->getWidth()-8, 450);
    

}

