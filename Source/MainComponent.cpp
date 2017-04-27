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


#include "MainComponent.h"

MainContentComponent::MainContentComponent(){


    File fs = File ( File::getCurrentWorkingDirectory().getFullPathName()+("/spatServerGRIS.app/Contents/Resources/splash.png"));
    if(fs.exists()){
        this->splash = new SplashScreen ("SpatServerGRIS",ImageFileFormat::loadFrom (fs),true);
    }
    
    PropertiesFile::Options options;
    options.applicationName = "SpatServerGRIS";
    options.commonToAllUsers = false;
    options.filenameSuffix = "xml";
    options.folderName = "GRIS";
    options.storageFormat = PropertiesFile::storeAsXML;
    options.ignoreCaseOfKeyNames = true;
    options.osxLibrarySubFolder = "Application Support";
    this->applicationProperties.setStorageParameters(options);
    this->applicationProperties.getCommonSettings(true);
    
   


    LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);
    
    this->listSpeaker = vector<Speaker *>();
    //this->listLevelComp = vector<LevelComponent *>();
    this->listSourceInput = vector<Input *>();
    
    this->lockSpeakers = new mutex();
    //this->lockLevelComp = new mutex();
    this->lockInputs = new mutex();
    
    
    this->winSpeakConfig = nullptr;
    this->winJackSetting = nullptr;
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
    this->labelJackStatus = addLabel("Jack Unknown","Jack Status",0, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackLoad =   addLabel("0.000000 %","Load Jack CPU",80, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackRate =   addLabel("00000 Hz","Rate",160, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackBuffer = addLabel("0000 spls","Buffer Size",240, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackInfo =   addLabel("...","Jack Inputs/Outputs system",320, 0, 80, 28,this->boxControlUI->getContent());
    
    this->labelJackStatus->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackLoad->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackRate->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackBuffer->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackInfo->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    
    this->butJackParam = addButton("Jack settings","Change jack settings",400,0,80,28,this->boxControlUI->getContent());
    
    this->butLoadXMLSpeakers = addButton("XML Speakers","Load Xml File Configuration",4,36,124,24,this->boxControlUI->getContent());
    this->butEditableSpeakers = addButton("Edit Speakers","Edit position of spkeakers",4,66,124,24,this->boxControlUI->getContent());
    this->butLoadPreset = addButton("Open","Open preset",4,96,124,24,this->boxControlUI->getContent());
    this->butSavePreset = addButton("Save","Save preset",4,126,124,24,this->boxControlUI->getContent());

    
    this->butShowSpeakerNumber =    addToggleButton("Show numbers", "Show numbers skeapers", 140, 100, 124, 24, this->boxControlUI->getContent());
    this->butHighPerformance =      addToggleButton("High performance", "Enable Low CPU Usage", 140, 124, 124, 24, this->boxControlUI->getContent());
    this->butNoiseSound =           addToggleButton("Noise Sound", "Enable bip noise", 140, 148, 124, 24, this->boxControlUI->getContent());
    

    
    this->tedOSCInPort = addTextEditor("Port OSC In :", "Port Socket", "Port Socket OSC Input", 140, 36, 50, 24, this->boxControlUI->getContent());
    this->tedOSCInPort->setText("18032");
    this->labOSCStatus= addLabel("...","OSC Receiver status",276, 36, 50, 24,this->boxControlUI->getContent());
    
    this->tedAddInputs =        addTextEditor("Inputs :", "0", "Numbers of Inputs", 140, 70, 50, 24, this->boxControlUI->getContent());
    this->butDefaultColorIn =   addButton("C","Set Default color Inputs",276,70,24,24,this->boxControlUI->getContent());
    
    addLabel("Gain :","Master Gain Outputs",320, 36, 120, 20,this->boxControlUI->getContent());
    this->sliderMasterGainOut = addSlider("Master Gain", "Master Gain Outputs", 360, 26, 80, 80, this->boxControlUI->getContent());
    this->sliderMasterGainOut->setRange(0.0, 1.0, 0.001);
    
    addLabel("Mode :","Mode of spatilization",320, 110, 120, 20,this->boxControlUI->getContent());
    this->comBoxModeSpat = addComboBox("", "Mode of spatilization", 320, 128, 150, 22, this->boxControlUI->getContent());
    for(int i = 0; i < ModeSpatString.size(); i++){
         this->comBoxModeSpat->addItem(ModeSpatString[i], i+1);
    }
    
   
    //this->labelAllClients= addLabel("...","Clients Connected",140, 130, 120, 80,this->boxControlUI->getContent());
    
    this->butAutoConnectJack = addButton("Auto Connect","Auto connection with jack",480,120,130,24,this->boxControlUI->getContent());
    
    this->boxClientJack = new BoxClient(this, &mGrisFeel);
    this->boxClientJack->setBounds(480, 0, 260, 120);
    this->boxControlUI->getContent()->addAndMakeVisible(this->boxClientJack);
    
    // set up the layout and resizer bars
    this->verticalLayout.setItemLayout (0, -0.2, -0.8, -0.5); // width of the font list must be between 20% and 80%, preferably 50%
    this->verticalLayout.setItemLayout (1, 8, 8, 8);           // the vertical divider drag-bar thing is always 8 pixels wide
    this->verticalLayout.setItemLayout (2, 150, -1.0, -0.65);  // the components on the right must be at least 150 pixels wide, preferably 50% of the total width
    this->verticalDividerBar = new StretchableLayoutResizerBar (&verticalLayout, 1, true);
    this->addAndMakeVisible (verticalDividerBar);

    this->setSize (1360, 670);
    
    

#if USE_JACK
    unsigned int BufferValue = applicationProperties.getUserSettings()->getValue("BufferValue").getIntValue();
    unsigned int RateValue = applicationProperties.getUserSettings()->getValue("RateValue").getIntValue();
    
    //Start JACK Server and client
    this->jackServer = new jackServerGRIS(RateValue);
    this->jackClient = new jackClientGris(BufferValue);
    
    
    if(!jackClient->isReady()){
        this->labelJackStatus->setText("Jack ERROR", dontSendNotification);
    }else{
        this->labelJackStatus->setText("Jack Run", dontSendNotification);
    }
    this->labelJackRate->setText(String(this->jackClient->sampleRate)+ " Hz", dontSendNotification);
    this->labelJackBuffer->setText(String(this->jackClient->bufferSize)+ " spls", dontSendNotification);
    this->labelJackInfo->setText("I : "+String(this->jackClient->numberInputs)+ " - O : "+String(this->jackClient->numberOutputs), dontSendNotification);
    this->sliderMasterGainOut->setValue(1.0);
#endif

    this->comBoxModeSpat->setSelectedId(1);

    
    //OSC Receiver----------------------------------------------------------------------------
    this->oscReceiver = new OscInput(this);
    textEditorReturnKeyPressed(*this->tedOSCInPort);
    
    this->tedAddInputs->setText("16",dontSendNotification);
    textEditorReturnKeyPressed(*this->tedAddInputs);

    
    this->openPreset(applicationProperties.getUserSettings()->getValue("lastOpentPreset"));


    this->resized();
    startTimerHz(HertzRefreshNormal);
    
    if(fs.exists()){
        this->splash->deleteAfterDelay(RelativeTime::seconds (1), false);
    }
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

Slider* MainContentComponent::addSlider(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into)
{
    Slider *sd = new Slider();
    sd->setTooltip (stooltip);
    //sd->setTextValueSuffix(s);
    sd->setSize(w, h);
    sd->setTopLeftPosition(x, y);
    sd->setSliderStyle(Slider::Rotary);
    sd->setRotaryParameters(M_PI * 1.3f, M_PI * 2.7f, true);
    sd->setTextBoxStyle (Slider::TextBoxBelow, false, 60, 20);
    sd->setColour(ToggleButton::textColourId, mGrisFeel.getFontColour());
    sd->setLookAndFeel(&mGrisFeel);
    sd->addListener(this);
    into->addAndMakeVisible(sd);
    return sd;
}

ComboBox* MainContentComponent::addComboBox(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into)
{
    ComboBox *cb = new ComboBox();
    cb->setTooltip(stooltip);
    cb->setSize(w, h);
    cb->setTopLeftPosition(x, y);
    cb->setLookAndFeel(&mGrisFeel);
    cb->addListener(this);
    into->addAndMakeVisible(cb);
    return cb;
}

//=====================================================
bool MainContentComponent::exitApp(){
    ScopedPointer<AlertWindow> alert = new AlertWindow ("Exit SpatServerGRIS !","Do you want to save preset ?", AlertWindow::InfoIcon);
    alert->addButton ("Save", 1);
    alert->addButton ("Cancel", 0);
    alert->addButton ("Exit", 2);
    int exitV = alert->runModalLoop();
    if (exitV == 1) {
        FileChooser fc ("Choose a file to save...",File::getCurrentWorkingDirectory(), "*.xml", true);
        if (fc.browseForFileToSave (true))
        {
            String chosen = fc.getResults().getReference(0).getFullPathName();
            bool r = AlertWindow::showOkCancelBox (AlertWindow::InfoIcon,"Save preset","Save to : " + chosen);
            //Save preset
            if(r){
                this->savePreset(chosen);
            }else {
                return 2;
            }
        }
    }
    return (exitV != 0);
 
}
MainContentComponent::~MainContentComponent() {

    this->applicationProperties.getUserSettings()->setValue("lastOpentPreset", this->pathCurrentPreset);
    this->applicationProperties.saveIfNeeded();
    this->applicationProperties.closeFiles();

    delete this->oscReceiver;
    
    if(this->winSpeakConfig != nullptr){
        delete this->winSpeakConfig;
    }
    if(this->winJackSetting != nullptr){
        delete this->winJackSetting;
    }
    
    delete this->speakerView;
    
    this->lockSpeakers->lock();
    for (auto&& it : this->listSpeaker)
    {
        delete (it);
    }
    this->listSpeaker.clear();
    this->lockSpeakers->unlock();
    delete this->lockSpeakers;

    
    this->lockInputs->lock();
    for (auto&& it : this->listSourceInput)
    {
        delete (it);
    }
    this->listSourceInput.clear();
    this->lockInputs->unlock();
    delete this->lockInputs;
    
   
    delete this->boxInputsUI;
    delete this->boxOutputsUI;
    delete this->boxControlUI;


#if USE_JACK
    delete  this->jackClient;
    delete this->jackServer;
#endif
}


void MainContentComponent::selectSpeaker(int idS)
{
    for(int i = 0; i < this->listSpeaker.size(); ++i) {
        if(i!=idS)
        {
            this->listSpeaker[i]->unSelectSpeaker();
        }else{
            this->listSpeaker[i]->selectSpeaker();
        }
    }
    if(this->winSpeakConfig != nullptr){
        this->winSpeakConfig->selectedRow(idS);
    }
}
void MainContentComponent::addSpeaker(){
    this->lockSpeakers->lock();
    unsigned int idNewSpeaker = (unsigned int)listSpeaker.size()+1;
    this->listSpeaker.push_back(new Speaker(this, idNewSpeaker, idNewSpeaker, glm::vec3(0.0f, 0.0f, 0.0f)));
    this->lockSpeakers->unlock();
    this->jackClient->addOutput();
    updateLevelComp();
}

void MainContentComponent::removeSpeaker(int idSpeaker){
    this->jackClient->removeOutput(idSpeaker);
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

void MainContentComponent::updateInputJack(int inInput, Input &inp){
    SourceIn *si = &this->jackClient->listSourceIn.at(inInput); //.getReference(inInput);
    si->x = inp.getCenter().x/10.0f;
    si->y = inp.getCenter().y/10.0f;
    si->z = inp.getCenter().z/10.0f;

    si->azimuth = inp.getAziMuth();
    si->zenith = inp.getZenith();
    si->radius = inp.getRad();
}

void MainContentComponent::updateLevelComp(){

    int x = 2;
    int indexS = 0;
    
    //this->jackClient->listSpeakerOut.clear();
    int i = 0;
    for (auto&& it : this->listSpeaker)
    {
        juce::Rectangle<int> level(x, 4, SizeWidthLevelComp, 200);
        it->getVuMeter()->setBounds(level);
        this->boxOutputsUI->getContent()->addAndMakeVisible(it->getVuMeter());
        it->getVuMeter()->repaint();
        x+=SizeWidthLevelComp;
        indexS+=1;
        SpeakerOut so;
        so.id = it->getOutputPatch();
        
        so.x = it->getCoordinate().x;
        so.y = it->getCoordinate().y;
        so.z = it->getCoordinate().z;

        so.azimuth = it->getAziZenRad().x;
        so.zenith = it->getAziZenRad().y;
        so.radius = it->getAziZenRad().z;
        this->jackClient->listSpeakerOut[it->getOutputPatch()-1] = so;
        i++;
    }
    
    x = 2;
    indexS = 0;
    //this->jackClient->listSourceIn.clear();
    i=0;
    for (auto&& it : this->listSourceInput)
    {
        juce::Rectangle<int> level(x, 4, SizeWidthLevelComp, 200);
        it->getVuMeter()->setBounds(level);
        this->boxInputsUI->getContent()->addAndMakeVisible(it->getVuMeter());
        it->getVuMeter()->repaint();
        x+=SizeWidthLevelComp;
        indexS+=1;
        SourceIn si;
        si.id = it->getId();
        si.x = it->getCenter().x/10.0f;
        si.y = it->getCenter().y/10.0f;
        si.z = it->getCenter().z/10.0f;
        si.azimuth = it->getAziMuth();
        si.zenith = it->getZenith();
        si.radius = it->getRad();
        
        this->jackClient->listSourceIn[i] = si;
        i++;
    }
    if(this->winSpeakConfig != nullptr){
        this->winSpeakConfig->updateWinContent();
    }

    this->boxOutputsUI->repaint();
    this->resized();
}


void MainContentComponent::setNameConfig(String name){
    this->nameConfig = name;
    this->speakerView->setNameConfig(nameConfig);
}

void MainContentComponent::muteInput(int id, bool mute){
    this->jackClient->muteIn[id-1] = mute;
}
void MainContentComponent::muteOutput(int id, bool mute){
    this->jackClient->muteOut[id-1] = mute;
}
void MainContentComponent::soloInput(int id, bool solo){
    this->jackClient->soloIn[id-1] = solo;
    this->jackClient->soloIn[MaxInputs] = false;
    for (int i = 0; i < MaxInputs; i++) {
        if(this->jackClient->soloIn[i]){
            this->jackClient->soloIn[MaxInputs] = true;
        }
    }
}
void MainContentComponent::soloOutput(int id, bool solo){
    this->jackClient->soloOut[id-1] = solo;
    this->jackClient->soloOut[MaxOutputs] = false;
    for (int i = 0; i < MaxOutputs; i++) {
        if(this->jackClient->soloOut[i]){
            this->jackClient->soloOut[MaxOutputs] = true;
        }
    }
}

void MainContentComponent::openXmlFileSpeaker(String path)
{
    this->pathCurrentFileSpeaker = path.toStdString();
    XmlDocument xmlDoc (File (this->pathCurrentFileSpeaker));
    ScopedPointer<XmlElement> mainXmlElem (xmlDoc.getDocumentElement());
    if (mainXmlElem == nullptr)
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::AlertIconType::WarningIcon,"Error XML !",
                                          "Your file is corrupted !\n"+xmlDoc.getLastParseError(),String(),0);
    }
    else
    {
        if(mainXmlElem->hasTagName("SpeakerSetup")){
            
            this->lockSpeakers->lock();
            for (auto&& it : listSpeaker)
            {
                delete (it);
            }
            listSpeaker.clear();
            this->lockSpeakers->unlock();
    
            nameConfig =  mainXmlElem->getStringAttribute("Name");
            cout << nameConfig << newLine;
            this->speakerView->setNameConfig(nameConfig);
            this->jackClient->clearOutput();
            forEachXmlChildElement (*mainXmlElem, ring)
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

void MainContentComponent::openPreset(String path){
    this->pathCurrentPreset = path;
    File xmlFile = File (path.toStdString());
    XmlDocument xmlDoc (xmlFile);
    ScopedPointer<XmlElement> mainXmlElem (xmlDoc.getDocumentElement());
    if (mainXmlElem == nullptr)
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::AlertIconType::WarningIcon,"Error XML !",
                                          "Your file is corrupted !\n"+xmlDoc.getLastParseError(),String(),0);
    }else{
        if(mainXmlElem->hasTagName("SpatServerGRIS_Preset")){
            this->tedOSCInPort->setText(mainXmlElem->getStringAttribute("OSC_Input_Port"));
            this->tedAddInputs->setText(mainXmlElem->getStringAttribute("Number_Of_Inputs"));
            this->sliderMasterGainOut->setValue(mainXmlElem->getDoubleAttribute("Master_Gain_Out"));
            this->butShowSpeakerNumber->setToggleState(mainXmlElem->getBoolAttribute("Show_Numbers"),sendNotification);
            this->butHighPerformance->setToggleState(mainXmlElem->getBoolAttribute("High_Performance"),sendNotification);
            this->pathCurrentFileSpeaker = mainXmlElem->getStringAttribute("Speaker_Setup_File");
            
            
            //Update----------------------------------
            this->textEditorReturnKeyPressed(*this->tedOSCInPort);
            this->textEditorReturnKeyPressed(*this->tedAddInputs);
            this->sliderValueChanged(this->sliderMasterGainOut);
            
            /*this->buttonClicked(this->butShowSpeakerNumber);
            this->buttonClicked(this->butHighPerformance);*/
            
            forEachXmlChildElement (*mainXmlElem, input)
            {
                if (input->hasTagName ("Input"))
                {
                    for (auto&& it : listSourceInput)
                    {
                        if(it->getId() == input->getIntAttribute("Index")){
                            it->setColor(Colour::fromFloatRGBA((float)input->getDoubleAttribute("R"), (float)input->getDoubleAttribute("G"), (float)input->getDoubleAttribute("B"), 1.0f),true);
                        }
                    }
                }
            }
            
            this->openXmlFileSpeaker(this->pathCurrentFileSpeaker);
            
        }
    }
}

void MainContentComponent::savePreset(String path){
    File xmlFile = File (path.toStdString());
    XmlDocument xmlDoc (xmlFile);
    ScopedPointer<XmlElement>  xml = new XmlElement("SpatServerGRIS_Preset");
    
    xml->setAttribute ("OSC_Input_Port",     this->tedOSCInPort->getTextValue().toString());
    xml->setAttribute ("Number_Of_Inputs",   this->tedAddInputs->getTextValue().toString());
    xml->setAttribute ("Master_Gain_Out",    this->sliderMasterGainOut->getValue());
    xml->setAttribute ("Show_Numbers",       this->butShowSpeakerNumber->getToggleState());
    xml->setAttribute ("High_Performance",   this->butHighPerformance->getToggleState());
    xml->setAttribute ("Speaker_Setup_File", this->pathCurrentFileSpeaker);
    
    for (auto&& it : listSourceInput)
    {
        XmlElement * xmlInput = new  XmlElement("Input");
        xmlInput->setAttribute("Index", it->getId());
        xmlInput->setAttribute("R", it->getColor().x);
        xmlInput->setAttribute("G", it->getColor().y);
        xmlInput->setAttribute("B", it->getColor().z);
        xml->addChildElement(xmlInput);
    }
    xml->writeToFile(xmlFile,"");
    xmlFile.create();
}


void MainContentComponent::savePresetSpeakers(String path){
    
    this->pathCurrentFileSpeaker = path;
    File xmlFile = File (path.toStdString());
    XmlDocument xmlDoc (xmlFile);
    ScopedPointer<XmlElement>  xml = new XmlElement("SpeakerSetup");
    
    xml->setAttribute ("Name", this->nameConfig);
    xml->setAttribute ("Dimension", 3);
    
    XmlElement * xmlRing = new  XmlElement("Ring");
    

    for (auto&& it : listSpeaker)
    {
        XmlElement * xmlInput = new  XmlElement("Speaker");
        xmlInput->setAttribute("PositionX", it->getCoordinate().x);
        xmlInput->setAttribute("PositionY", it->getCoordinate().z);
        xmlInput->setAttribute("PositionZ", it->getCoordinate().y);
        
        xmlInput->setAttribute("Azimuth", it->getAziZenRad().x);
        xmlInput->setAttribute("Zenith", it->getAziZenRad().y);
        xmlInput->setAttribute("Radius", it->getAziZenRad().z);

        xmlInput->setAttribute("LayoutIndex", it->getIdSpeaker());
        xmlInput->setAttribute("OutputPatch", it->getOutputPatch());
        xmlRing->addChildElement(xmlInput);
    }
    xml->addChildElement(xmlRing);
    xml->writeToFile(xmlFile,"");
    xmlFile.create();
}


void MainContentComponent::saveJackSettings(unsigned int rate, unsigned int buff){
    unsigned int BufferValue = applicationProperties.getUserSettings()->getValue("BufferValue").getIntValue();
    unsigned int RateValue = applicationProperties.getUserSettings()->getValue("RateValue").getIntValue();

    if(rate != RateValue || buff != BufferValue){
        bool r = AlertWindow::showOkCancelBox(AlertWindow::AlertIconType::WarningIcon,"Restart SpatServerGRIS",
                                               "Need to restart SpatServerGRIS for apply new settings !");
        //Click OK -> Open xml
        if(r){
            applicationProperties.getUserSettings()->setValue("BufferValue", (int)buff);
            applicationProperties.getUserSettings()->setValue("RateValue", (int)rate );
            applicationProperties.saveIfNeeded();
            
            //Restart APP
            String applicationPath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
            String relaunchCommand = "open " + applicationPath;
            ScopedPointer<ChildProcess> scriptProcess = new ChildProcess();
            
            JUCEApplication::getInstance()->systemRequestedQuit();
            cout << relaunchCommand << newLine;
            scriptProcess->start(relaunchCommand, (!ChildProcess::wantStdErr | !ChildProcess::wantStdOut));
        }
    }
}

void MainContentComponent::timerCallback(){
    this->labelJackLoad->setText(String(this->jackClient->getCpuUsed(),4)+ " %", dontSendNotification);
    if(this->jackClient->overload){
        this->labelJackLoad->setColour(Label::backgroundColourId, Colours::darkred);
    }else{
        this->labelJackLoad->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    }
    
    for (auto&& it : listSourceInput)
    {
        it->getVuMeter()->update();
    }
    for (auto&& it : listSpeaker)
    {
        it->getVuMeter()->update();
    }
    
    this->boxClientJack->updateContentCli();
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
        
    }
    else if(&textEditor == this->tedAddInputs){
        if(this->tedAddInputs->getTextValue().toString().getIntValue() < 0){
            this->tedAddInputs->setText("0");
        }
        if(this->tedAddInputs->getTextValue().toString().getIntValue() > MaxInputs){
            this->tedAddInputs->setText(String(MaxInputs));
        }
        if(this->jackClient->inputsPort.size() != this->tedAddInputs->getTextValue().toString().getIntValue()){
            this->jackClient->addRemoveInput(this->tedAddInputs->getTextValue().toString().getIntValue());
            this->lockInputs->lock();
            for (auto&& it : listSourceInput)
            {
                delete (it);
            }
            listSourceInput.clear();
            for(int i = 0 ; i< this->jackClient->inputsPort.size();i++){
                this->listSourceInput.push_back(new Input(this, &mGrisFeel,i+1));
            }
            this->lockInputs->unlock();
        }
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
            bool r = AlertWindow::showOkCancelBox (AlertWindow::AlertIconType::WarningIcon,"Open XML !",
                                          "You want to load : "+chosen+"\nEverything not saved will be lost !",
                                                  String(),String(),0);
            //Click OK -> Open xml
            if(r){
                this->openXmlFileSpeaker(chosen);
            }
        }
        
    }
    else if(button == this->butLoadPreset){
        
        FileChooser fc ("Choose a file to open...",File::getCurrentWorkingDirectory(),"*.xml",true);
        if (fc.browseForFileToOpen())
        {
            String chosen = fc.getResults().getReference(0).getFullPathName();
            bool r = AlertWindow::showOkCancelBox (AlertWindow::AlertIconType::WarningIcon,"Open Preset !",
                                                   "You want to load : "+chosen+"\nEverything not saved will be lost !",
                                                   String(),String(),0);
            //Click OK -> Open
            if(r){
                this->openPreset(chosen);
            }
        }
        
    }
    else if(button == this->butSavePreset){
        
        FileChooser fc ("Choose a file to save...",File::getCurrentWorkingDirectory(), "*.xml", true);
        if (fc.browseForFileToSave (true))
        {
            String chosen = fc.getResults().getReference(0).getFullPathName();
            bool r = AlertWindow::showOkCancelBox (AlertWindow::InfoIcon,"Save preset","Save to : " + chosen);
            //Save preset
            if(r){
                this->savePreset(chosen);
            }
        }
    }
    else if(button == this->butEditableSpeakers){
    
        if(this->winSpeakConfig == nullptr){
            this->winSpeakConfig = new WindowEditSpeaker("Speakers config", this->nameConfig, this->mGrisFeel.getWinBackgroundColour(),DocumentWindow::allButtons, this, &this->mGrisFeel);
            
            Rectangle<int> result (this->getScreenX()+ this->speakerView->getWidth()+22,this->getScreenY(),600,500);
            this->winSpeakConfig->setBounds (result);
            this->winSpeakConfig->setResizable (true, true);
            this->winSpeakConfig->setUsingNativeTitleBar (true);
            this->winSpeakConfig->setVisible (true);
            this->winSpeakConfig->setAlwaysOnTop(true);
            
            this->winSpeakConfig->initComp();
            this->winSpeakConfig->repaint();
        }
        
    }else if(button == this->butJackParam){
        
        if(this->winJackSetting == nullptr){
            unsigned int BufferValue = applicationProperties.getUserSettings()->getValue("BufferValue").getIntValue();
            unsigned int RateValue = applicationProperties.getUserSettings()->getValue("RateValue").getIntValue();

            this->winJackSetting = new WindowJackSetting("Jack Settings", this->mGrisFeel.getWinBackgroundColour(),DocumentWindow::allButtons, this, &this->mGrisFeel, RateValues.indexOf(String(RateValue)), BufferSize.indexOf(String(BufferValue)));
            Rectangle<int> result (this->getScreenX()+ (this->speakerView->getWidth()/2)-150, this->getScreenY()+(this->speakerView->getHeight()/2)-75, 250, 120);
            this->winJackSetting->setBounds (result);
            this->winJackSetting->setResizable (false, false);
            this->winJackSetting->setUsingNativeTitleBar (true);
            this->winJackSetting->setVisible (true);
            this->winJackSetting->setAlwaysOnTop(true);
            this->winJackSetting->repaint();
        }
        
    }else if(button == butShowSpeakerNumber){
        
        this->speakerView->setShowNumber(this->butShowSpeakerNumber->getToggleState());
        
    }else if(button == this->butAutoConnectJack){
        
        this->jackClient->autoConnectClient();
        
    }else if(button == this->butHighPerformance){
        
        stopTimer();
        if(this->butHighPerformance->getToggleState()){
            startTimerHz(HertzRefreshLowCpu);
        }else{
            startTimerHz(HertzRefreshNormal);
        }
        this->speakerView->setHighPerfor(this->butHighPerformance->getToggleState());
        
    }else if(button == this->butNoiseSound){
        
        this->jackClient->noiseSound = butNoiseSound->getToggleState();
    }
    else if(button == this->butDefaultColorIn){
        
        float hue = 0;
        for (auto&& it : listSourceInput)
        {
            it->setColor(Colour::fromHSV(hue, 1, 0.85, 1), true);
            hue+=0.01f;
        }
    }
    
}

void MainContentComponent::sliderValueChanged (Slider* slider)
{
    if(slider == this->sliderMasterGainOut){
        this->jackClient->masterGainOut = this->sliderMasterGainOut->getValue();
    }
}

void MainContentComponent::comboBoxChanged (ComboBox *comboBox)
{
    if(this->comBoxModeSpat == comboBox){
        this->jackClient->modeSelected = (ModeSpatEnum)(this->comBoxModeSpat->getSelectedId()-1);
    }
}
void MainContentComponent::resized()
{
    
    Rectangle<int> r (getLocalBounds().reduced (2));
    
    // lay out the list box and vertical divider..
    Component* vcomps[] = { this->speakerView, this->verticalDividerBar, nullptr };
    
    // lay out side-by-side and resize the components' heights as well as widths
    this->verticalLayout.layOutComponents (vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);
    

    this->boxInputsUI->setBounds(this->speakerView->getWidth()+6, 2, getWidth()-(this->speakerView->getWidth()+10),240);
    this->boxInputsUI->correctSize(((unsigned int )this->listSourceInput.size()*(SizeWidthLevelComp))+4, 210);

    this->boxOutputsUI->setBounds(this->speakerView->getWidth()+6, 244, getWidth()-(this->speakerView->getWidth()+10),240);
    this->boxOutputsUI->correctSize(((unsigned int )this->listSpeaker.size()*(SizeWidthLevelComp))+4, 210);
    
    this->boxControlUI->setBounds(this->speakerView->getWidth()+6, 488, getWidth()-(this->speakerView->getWidth()+10), getHeight()-490);
    this->boxControlUI->correctSize(750, 180);
    

}

