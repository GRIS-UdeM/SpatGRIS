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
    //this->listLevelComp = vector<LevelComponent *>();
    this->listSourceInput = vector<Input *>();
    
    this->lockSpeakers = new mutex();
    //this->lockLevelComp = new mutex();
    this->lockInputs = new mutex();
    
    
    this->winSpeakConfig = nullptr;
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

    
    
    //Components in BOX 3 ------------------------------------------------------------------
    this->labelJackStatus = addLabel("Jack Unknown","",0, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackLoad = addLabel("0.000000 %","",80, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackRate = addLabel("00000 Hz","",160, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackBuffer = addLabel("0000 spls","",240, 0, 80, 28,this->boxControlUI->getContent());
    
    this->labelJackStatus->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackLoad->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackRate->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackBuffer->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());

    
    this->butLoadXMLSpeakers = addButton("XML Speakers","Load Xml File Coniguration",4,36,124,24,this->boxControlUI->getContent());
    
    this->butEditableSpeakers = addButton("Edit Speakers","Edit position of spkeakers",4,70,124,24,this->boxControlUI->getContent());
    
    this->butShowSpeakerNumber = addToggleButton("Show numbers", "Show numbers skeapers", 4, 100, 124, 24, this->boxControlUI->getContent());
    
    this->tedOSCInPort = addTextEditor("Port OSC In :", "Port Socket", "Port Socket OSC Input", 140, 36, 50, 24, this->boxControlUI->getContent());
    this->tedOSCInPort->setText("18032");
    this->labOSCStatus= addLabel("...","OSC Receiver status",270, 36, 50, 24,this->boxControlUI->getContent());
    
    this->tedAddInputs= addTextEditor("Inputs :", "0", "Numbers of Inputs", 140, 70, 50, 24, this->boxControlUI->getContent());
    
    this->butAutoConnectJack= addButton("Auto Connect","Auto connection with jack",140,104,130,24,this->boxControlUI->getContent());
    
    
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
    this->jackServer = new jackServerGRIS();
    this->jackClient = new jackClientGris();
  
    
    if(!jackClient->isReady()){
        this->labelJackStatus->setText("JackClient Not Connected", dontSendNotification);
    }else{
         this->labelJackStatus->setText("Jack Ready", dontSendNotification);
    }
    this->labelJackRate->setText(String(this->jackClient->sampleRate)+ " Hz", dontSendNotification);
    this->labelJackBuffer->setText(String(this->jackClient->bufferSize)+ " spls", dontSendNotification);
#endif
    
    
    //OSC Receiver----------------------------------------------------------------------------

    /**/
    this->oscReceiver = new OscInput(this);

    textEditorReturnKeyPressed(*this->tedOSCInPort);


    openXmlFileSpeaker("/Users/gris/Documents/GRIS/zirkonium/ZirkSpeakers_Dome 16 UdeM.xml");
    
    this->resized();
    startTimerHz(hertzRefresh);
}


Label* MainContentComponent::addLabel(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into)
{
    Label *lb = new Label();
    lb->setText(s, NotificationType::dontSendNotification);
    lb->setTooltip (stooltip);
    lb->setJustificationType(Justification::left);
    lb->setFont(mGrisFeel.getFont());
    lb->setLookAndFeel(&mGrisFeel);
    lb->setColour(Label::textColourId, mGrisFeel.getFontColour());
    lb->setBounds(x, y, w, h);
    into->addAndMakeVisible(lb);
    return lb;
}

TextButton* MainContentComponent::addButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into)
{
    TextButton *tb = new TextButton();
    tb->setTooltip (stooltip);
    tb->setButtonText(s);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    tb->setColour(ToggleButton::textColourId, mGrisFeel.getFontColour());
    tb->setLookAndFeel(&mGrisFeel);
    into->addAndMakeVisible(tb);
    return tb;
}

ToggleButton* MainContentComponent::addToggleButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, bool toggle)
{
    ToggleButton *tb = new ToggleButton();
    tb->setTooltip (stooltip);
    tb->setButtonText(s);
    tb->setToggleState(toggle, dontSendNotification);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    tb->setColour(ToggleButton::textColourId, mGrisFeel.getFontColour());
    tb->setLookAndFeel(&mGrisFeel);
    into->addAndMakeVisible(tb);
    return tb;
}

TextEditor* MainContentComponent::addTextEditor(const String &s, const String &emptyS, const String &stooltip, int x, int y, int w, int h, Component *into, int wLab)
{
    TextEditor *te = new TextEditor();
    te->setTooltip (stooltip);
    te->setTextToShowWhenEmpty(emptyS, mGrisFeel.getOffColour());
    te->setColour(ToggleButton::textColourId, mGrisFeel.getFontColour());
    te->setLookAndFeel(&mGrisFeel);
    
    if (s.isEmpty()){
        te->setBounds(x, y, w, h);
    }else{
        te->setBounds(x+wLab, y, w, h);
        Label *lb =addLabel(s, "", x, y, wLab, h, into);
        lb->setJustificationType(Justification::centredRight);
    }
    
    te->addListener(this);
    into->addAndMakeVisible(te);
    return te;
}


MainContentComponent::~MainContentComponent() {
    delete this->oscReceiver;
    
    if(this->winSpeakConfig != nullptr){
        delete this->winSpeakConfig;
    }
    
    delete this->speakerView;
    
    this->lockSpeakers->lock();
    for (auto&& it : listSpeaker)
    {
        delete (it);
    }
    listSpeaker.clear();
    this->lockSpeakers->unlock();
    delete this->lockSpeakers;

    
    this->lockInputs->lock();
    for (auto&& it : listSourceInput)
    {
        delete (it);
    }
    listSourceInput.clear();
    this->lockInputs->unlock();
    delete this->lockInputs;
    
   
    delete this->boxInputsUI;
    delete this->boxOutputsUI;
    delete this->boxControlUI;


#if USE_JACK
    shutdownAudio();
    delete  this->jackClient;
    
    delete this->jackServer;
    #endif
}

void MainContentComponent::addSpeaker(){
    this->lockSpeakers->lock();
    int idNewSpeaker = listSpeaker.size()+1;
    this->listSpeaker.push_back(new Speaker(this, idNewSpeaker, idNewSpeaker, glm::vec3(0.0f, 0.0f, 0.0f)));
    this->lockSpeakers->unlock();
    this->jackClient->addOutput();
    updateLevelComp();
}

void MainContentComponent::removeSpeaker(int idSpeaker){

    this->lockSpeakers->lock();
    int index = 0;
    for (auto&& it : this->listSpeaker)
    {
        if(index == idSpeaker){
            delete (it);
            this->listSpeaker.erase(this->listSpeaker.begin() + idSpeaker);
        }
        index+=1;
    }

    this->lockSpeakers->unlock();
    updateLevelComp();

}

void MainContentComponent::updateLevelComp(){
    int x = 2;
    int indexS = 0;
    for (auto&& it : this->listSpeaker)
    {
        juce::Rectangle<int> level(x, 4, sizeWidthLevelComp, 200);
        it->getVuMeter()->setBounds(level);
        this->boxOutputsUI->getContent()->addAndMakeVisible(it->getVuMeter());
        it->getVuMeter()->repaint();
        x+=sizeWidthLevelComp;
        indexS+=1;
    }
    
    x = 2;
    indexS = 0;
    for (auto&& it : this->listSourceInput)
    {
        juce::Rectangle<int> level(x, 4, sizeWidthLevelComp, 200);
        it->getVuMeter()->setBounds(level);
        this->boxInputsUI->getContent()->addAndMakeVisible(it->getVuMeter());
        it->getVuMeter()->repaint();
        x+=sizeWidthLevelComp;
        indexS+=1;
    }
    if(this->winSpeakConfig != nullptr){
        this->winSpeakConfig->updateWinContent();
    }
    
    this->boxOutputsUI->repaint();
    this->resized();
}

void MainContentComponent::openXmlFileSpeaker(String path)
{
    this->lockSpeakers->lock();
    for (auto&& it : listSpeaker)
    {
        delete (it);
    }
    listSpeaker.clear();
    this->lockSpeakers->unlock();


    XmlDocument myDocument (File (path.toStdString()));
    ScopedPointer<XmlElement> mainElement (myDocument.getDocumentElement());
    if (mainElement == nullptr)
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::AlertIconType::WarningIcon,"Error XML !",
                                          "Your file is corrupted !\n"+myDocument.getLastParseError(),String(),0);
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
                            
                            this->listSpeaker.push_back(new Speaker(this, spk->getIntAttribute("LayoutIndex"),
                                                              spk->getIntAttribute("OutputPatch"),
                                                              glm::vec3(spk->getDoubleAttribute("PositionX")*10.0f,
                                                                        spk->getDoubleAttribute("PositionZ")*10.0f,
                                                                        spk->getDoubleAttribute("PositionY")*10.0f)));
                            this->jackClient->addOutput();
                        }
                    }
                    
                }
            }
            
        }else{
            AlertWindow::showMessageBoxAsync (AlertWindow::AlertIconType::WarningIcon,"Error XML !",
                                              "SpeakerSetup not found !",String(),0);
        }
        
    }
    

    updateLevelComp();
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

void MainContentComponent::timerCallback(){
    this->labelJackLoad->setText(String(this->jackClient->getCpuUsed())+ " %", dontSendNotification);
    for (auto&& it : listSourceInput)
    {
        it->getVuMeter()->update();
    }
    for (auto&& it : listSpeaker)
    {
        it->getVuMeter()->update();
    }
   
}

void MainContentComponent::paint (Graphics& g) {
    g.fillAll (mGrisFeel.getWinBackgroundColour());
}


void MainContentComponent::textEditorFocusLost (TextEditor &textEditor)
{
    textEditorReturnKeyPressed(textEditor);
}

void MainContentComponent::textEditorReturnKeyPressed (TextEditor & textEditor){
    if(&textEditor == this->tedOSCInPort){
        if(this->oscReceiver->startConnection(this->tedOSCInPort->getTextValue().toString().getIntValue())){
            this->labOSCStatus->setText("OK", dontSendNotification);
            this->labOSCStatus->setColour(Label::textColourId, mGrisFeel.getFontColour());
        }else{
            this->labOSCStatus->setText("Error", dontSendNotification);
            this->labOSCStatus->setColour(Label::textColourId, Colours::red);
        }
        
    }else if(&textEditor == this->tedAddInputs){
        this->jackClient->addRemoveInput(this->tedAddInputs->getTextValue().toString().getIntValue());
        this->lockInputs->lock();
        for (auto&& it : listSourceInput)
        {
            delete (it);
        }
        listSourceInput.clear();
        for(int i = 0 ; i< this->jackClient->inputs.size();i++){
            this->listSourceInput.push_back(new Input(this, &mGrisFeel,i+1));
        }
        this->lockInputs->unlock();
        updateLevelComp();
    }
}

void MainContentComponent::buttonClicked (Button *button)
{
    if(button == this->butLoadXMLSpeakers){
        
        FileChooser fc ("Choose a file to open...",File::getCurrentWorkingDirectory(),"*.xml",true);
        if (fc.browseForFileToOpen())
        {
            String chosen = fc.getResults().getReference(0).getFullPathName();
            int r = AlertWindow::showOkCancelBox (AlertWindow::AlertIconType::WarningIcon,"Open XML !",
                                          "You want to load : "+chosen+"\nEverything not saved will be lost !",
                                                  String(),String(),0);
            //Click OK -> Open xml
            if(r==1){
                this->openXmlFileSpeaker(chosen);
            }
        }
    }else if(button == this->butEditableSpeakers){
    
        if(this->winSpeakConfig == nullptr){
            this->winSpeakConfig = new WindowEditSpeaker("Speakers config", this->mGrisFeel.getWinBackgroundColour(),DocumentWindow::allButtons, this, &this->mGrisFeel);
            
            Rectangle<int> result (this->getScreenX()+ this->speakerView->getWidth()+22,this->getScreenY(),600,480);
            this->winSpeakConfig->setBounds (result);
            this->winSpeakConfig->setResizable (true, true);
            this->winSpeakConfig->setUsingNativeTitleBar (true);
            this->winSpeakConfig->setVisible (true);
            this->winSpeakConfig->setAlwaysOnTop(true);
            
            this->winSpeakConfig->initComp();
            this->winSpeakConfig->repaint();
        }
    }else if(button == butShowSpeakerNumber){
        this->speakerView->setShowNumber(this->butShowSpeakerNumber->getToggleState());
        
    }else if(button == this->butAutoConnectJack){
        this->jackClient->autoConnectClient();
    }
}

void MainContentComponent::resized() {
    
    Rectangle<int> r (getLocalBounds().reduced (2));
    
    // lay out the list box and vertical divider..
    Component* vcomps[] = { this->speakerView, this->verticalDividerBar, nullptr };
    
    // lay out side-by-side and resize the components' heights as well as widths
    this->verticalLayout.layOutComponents (vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);
    

    this->boxInputsUI->setBounds(this->speakerView->getWidth()+6, 2, getWidth()-(this->speakerView->getWidth()+10),240);
    this->boxInputsUI->correctSize((this->listSourceInput.size()*(sizeWidthLevelComp))+4, 210);

    this->boxOutputsUI->setBounds(this->speakerView->getWidth()+6, 244, getWidth()-(this->speakerView->getWidth()+10),240);
    this->boxOutputsUI->correctSize((this->listSpeaker.size()*(sizeWidthLevelComp))+4, 210);
    
    this->boxControlUI->setBounds(this->speakerView->getWidth()+6, 488, getWidth()-(this->speakerView->getWidth()+10), getHeight()-490);
    this->boxControlUI->correctSize(this->boxControlUI->getWidth()-8, 450);
    

}

