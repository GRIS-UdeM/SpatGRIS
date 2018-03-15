/*
 This file is part of ServerGris.
 
 Developers: Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MainComponent.h"

/* FIXME
 * Resources path should be set only once, based on the system, in a config.h header.
 */

MainContentComponent::MainContentComponent(DocumentWindow *parent)
{
    this->parent = parent;

    this->addAndMakeVisible (menuBar = new MenuBarComponent (this));

    File fs;
#ifdef __linux__
    fs = File ( File::getCurrentWorkingDirectory().getFullPathName() + ("/../../Resources/splash_screen.png"));
#else
    String cwd = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
    fs = File (cwd + ("/Contents/Resources/splash_screen.png"));
#endif
    if(fs.exists()) {
        this->splash = new SplashScreen("ServerGRIS", ImageFileFormat::loadFrom(fs), true);
    }
    
    //Local save (Last xml file open)
    PropertiesFile::Options options;
    options.applicationName = "ServerGRIS";
    options.commonToAllUsers = false;
    options.filenameSuffix = "xml";
    options.folderName = "GRIS";
    options.storageFormat = PropertiesFile::storeAsXML;
    options.ignoreCaseOfKeyNames = true;
    options.osxLibrarySubFolder = "Application Support";
    this->applicationProperties.setStorageParameters(options);

    PropertiesFile *props = this->applicationProperties.getUserSettings();

    LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);

    this->isProcessForeground = true;

    this->isNumbersShown = false;
    this->isSpeakersShown = true;
    this->isSourceLevelShown = false;
    this->isSphereShown = false;
    this->isHighPerformance = false;
    this->isSpanShown = true;

    this->pathLastVbapSpeakerSetup = String("");

    this->listSpeaker = vector<Speaker *>();
    this->listSourceInput = vector<Input *>();
    
    this->lockSpeakers = new mutex();
    this->lockInputs = new mutex();
    
    this->winSpeakConfig = nullptr;
    this->windowProperties = nullptr;
    this->winControlSource = nullptr;
    this->aboutWindow = nullptr;

    //SpeakerViewComponent 3D VIEW------------------------------
    this->speakerView = new SpeakerViewComponent(this);
    addAndMakeVisible(this->speakerView);

    //BOX Main--------------------------------------------------
    this->boxMainUI = new Box(&mGrisFeel, "", true, false);
    addAndMakeVisible(this->boxMainUI);

    //BOX Inputs------------------------------------------------
    this->boxInputsUI = new Box(&mGrisFeel, "Inputs");
    addAndMakeVisible(this->boxInputsUI);
    
    //BOX Outputs-----------------------------------------------
    this->boxOutputsUI = new Box(&mGrisFeel, "Outputs");
    addAndMakeVisible(this->boxOutputsUI);
    
    //BOX Control-----------------------------------------------
    this->boxControlUI = new Box(&mGrisFeel);
    addAndMakeVisible(this->boxControlUI);

    this->boxMainUI->getContent()->addAndMakeVisible(this->boxInputsUI);
    this->boxMainUI->getContent()->addAndMakeVisible(this->boxOutputsUI);
    this->boxMainUI->getContent()->addAndMakeVisible(this->boxControlUI);

    //Components in BOX Control ------------------------------------------------------------------
    this->labelJackStatus = addLabel("Jack Unknown","Jack Status",0, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackLoad =   addLabel("0.000000 %","Load Jack CPU",80, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackRate =   addLabel("00000 Hz","Rate",160, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackBuffer = addLabel("0000 spls","Buffer Size",240, 0, 80, 28,this->boxControlUI->getContent());
    this->labelJackInfo =   addLabel("...","Jack Inputs/Outputs system",320, 0, 90, 28,this->boxControlUI->getContent());
    
    this->labelJackStatus->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackLoad->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackRate->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackBuffer->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackInfo->setColour(Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());

    addLabel("Gain","Master Gain Outputs",15, 30, 120, 20,this->boxControlUI->getContent());
    this->sliderMasterGainOut = addSlider("Master Gain", "Master Gain Outputs", 5, 45, 60, 60, this->boxControlUI->getContent());
    this->sliderMasterGainOut->setRange(0.0, 1.0, 0.001);
    
    addLabel("Interpolation", "Master Interpolation", 60, 30, 120, 20, this->boxControlUI->getContent());
    this->sliderInterpolation = addSlider("Inter", "Interpolation", 70, 45, 60, 60, this->boxControlUI->getContent());
    this->sliderInterpolation->setRange(0.0, 1.0, 0.001); // val * 0.29 + 0.7

    addLabel("Mode :","Mode of spatilization", 150, 30, 60, 20,this->boxControlUI->getContent());
    // TODO: labelModeInfo should be removed.
    this->labelModeInfo = addLabel("...","Status of spatilization", 195, 30, 120, 20,this->boxControlUI->getContent());
    this->comBoxModeSpat = addComboBox("", "Mode of spatilization", 155, 48, 90, 22, this->boxControlUI->getContent());
    for (int i = 0; i < ModeSpatString.size(); i++) {
        this->comBoxModeSpat->addItem(ModeSpatString[i], i+1);
        if (i == 0 || i >= 2) {
            this->comBoxModeSpat->setItemEnabled(i+1, true);
        } else {
            this->comBoxModeSpat->setItemEnabled(i+1, false);
        }
    }

    this->tedAddInputs = addTextEditor("Inputs :", "0", "Numbers of Inputs", 122, 83, 43, 22, this->boxControlUI->getContent());
    this->tedAddInputs->setInputRestrictions(3,"0123456789");

    this->butInitRecord = addButton("Init Recording", "Init Recording", 268, 48, 103, 24, this->boxControlUI->getContent());
    
    this->butStartRecord = addButton("Record", "Start/Stop Record", 268, 83, 60, 24, this->boxControlUI->getContent());
    this->butStartRecord->setEnabled(false);

    this->labelTimeRecorded = addLabel("00:00","Record time", 327, 83, 50, 24,this->boxControlUI->getContent());

    /* Are these functions really necessary? Just hidding them for the time being... */
    //this->butDisconnectAllJack  = addButton("X All","Disconnect all Jack",480,120,40,24,this->boxControlUI->getContent());
    //this->butDisconnectAllJack->setColour(TextButton::buttonColourId, mGrisFeel.getRedColour());
    //this->butAutoConnectJack    = addButton("Auto Connect","Auto connection with jack",610,120,130,24,this->boxControlUI->getContent());

    this->boxClientJack = new BoxClient(this, &mGrisFeel);
    this->boxClientJack->setBounds(410, 0, 304, 138);
    this->boxControlUI->getContent()->addAndMakeVisible(this->boxClientJack);
    
    // set up the layout and resizer bars
    this->verticalLayout.setItemLayout (0, -0.2, -0.8, -0.5); // width of the font list must be between 20% and 80%, preferably 50%
    this->verticalLayout.setItemLayout (1, 8, 8, 8);           // the vertical divider drag-bar thing is always 8 pixels wide
    this->verticalLayout.setItemLayout (2, 150, -1.0, -0.65);  // the components on the right must be at least 150 pixels wide, preferably 50% of the total width
    this->verticalDividerBar = new StretchableLayoutResizerBar (&verticalLayout, 1, true);
    this->addAndMakeVisible (verticalDividerBar);

    this->setSize (1285, 610);

    // Jack Init and Param -------------------------------------------------------------------------------
    unsigned int BufferValue = props->getIntValue("BufferValue", 1024);
    unsigned int RateValue = props->getIntValue("RateValue", 48000);
    unsigned int FileFormat = props->getIntValue("FileFormat", 0);

    //cout << "Buffer Rate: " << BufferValue << ", Sampling Rate: " << RateValue << ", File Format: " << FileFormat << endl;
    
    if(isnan(BufferValue) || BufferValue == 0 || isnan(RateValue) || RateValue == 0){
        BufferValue = 1024;
        RateValue = 48000;
    }
    //Start JACK Server and client
    this->jackServer = new jackServerGRIS(RateValue);
    this->jackClient = new jackClientGris(BufferValue);

    this->samplingRate = RateValue;

    if(!jackClient->isReady()){
        this->labelJackStatus->setText("Jack ERROR", dontSendNotification);
    }else{
        this->labelJackStatus->setText("Jack Run", dontSendNotification);
    }
    //--------------------------------------------------------------------------------
    this->labelJackRate->setText(String(this->jackClient->sampleRate)+ " Hz", dontSendNotification);
    this->labelJackBuffer->setText(String(this->jackClient->bufferSize)+ " spls", dontSendNotification);
    this->labelJackInfo->setText("I : "+String(this->jackClient->numberOutputs)+ " - O : "+String(this->jackClient->numberInputs), dontSendNotification);

    this->sliderMasterGainOut->setValue(1.0);
    this->sliderInterpolation->setValue(0.33);
    this->comBoxModeSpat->setSelectedId(1);

    //OSC Receiver----------------------------------------------------------------------------
    this->oscReceiver = new OscInput(this);

    this->oscReceiver->startConnection(this->oscInputPort);

    this->tedAddInputs->setText("16",dontSendNotification);
    textEditorReturnKeyPressed(*this->tedAddInputs);

    // Opens the default preset if lastOpenPreset is not a valid file.
    File preset = File(props->getValue("lastOpenPreset", "not_saved_yet"));
    if (!preset.existsAsFile()) {
#ifdef __linux__
        String cwd = File::getCurrentWorkingDirectory().getFullPathName();
        this->openPreset(cwd + ("/../../Resources/default_preset/default_preset.xml"));
#else
        String cwd = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
        this->openPreset(cwd + ("/Contents/Resources/default_preset/default_preset.xml"));
#endif
    }
    else {
        this->openPreset(props->getValue("lastOpenPreset"));
    }

    // Do the same with speaker setup.
    File setup = File(props->getValue("lastOpenSpeakerSetup", "not_saved_yet"));
    if (!setup.existsAsFile()) {
#ifdef __linux__
        String cwd = File::getCurrentWorkingDirectory().getFullPathName();
        this->openXmlFileSpeaker(cwd + ("/../../Resources/default_preset/default_speaker_setup.xml"));
#else
        String cwd = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
        this->openXmlFileSpeaker(cwd + ("/Contents/Resources/default_preset/default_speaker_setup.xml"));
#endif
    }
    else {
        this->openXmlFileSpeaker(props->getValue("lastOpenSpeakerSetup"));
    }

    this->resized();
    startTimerHz(HertzRefreshNormal);
    
    //End Splash
    if(fs.exists()){
        this->splash->deleteAfterDelay(RelativeTime::seconds(4), false);
    }

    ApplicationCommandManager* commandManager = &MainWindow::getApplicationCommandManager();
    commandManager->registerAllCommandsForTarget(this);
}

// ====================== builder utilities ==========================
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
//================ Menu item actions =====================================
void MainContentComponent::handleNew() {
    ScopedPointer<AlertWindow> alert = new AlertWindow ("Closing current preset !","Do you want to save ?", AlertWindow::InfoIcon);
    alert->setLookAndFeel(&mGrisFeel);
    alert->addButton ("Cancel", 0);
    alert->addButton ("yes", 1);
    alert->addButton ("No", 2);
    int status = alert->runModalLoop();
    if (status == 1) {
        this->handleSavePreset();
    } else if (status == 0) {
        return;
    }

#ifdef __linux__
    String cwd = File::getCurrentWorkingDirectory().getFullPathName();
    this->openPreset(cwd + ("/../../Resources/default_preset/default_preset.xml"));
#else
    String cwd = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
    this->openPreset(cwd + ("/Contents/Resources/default_preset/default_preset.xml"));
#endif
}

void MainContentComponent::handleOpenPreset() {
    String dir = this->applicationProperties.getUserSettings()->getValue("lastPresetDirectory");
    if (! File(dir).isDirectory()) {
        dir = File("~").getFullPathName();
    }
    String filename = File(this->pathCurrentPreset).getFileName();
    
#ifdef __linux__
    FileChooser fc ("Choose a file to open...", dir + "/" + filename, "*.xml", false);
#else
    FileChooser fc ("Choose a file to open...", dir + "/" + filename, "*.xml", true);
#endif
    if (fc.browseForFileToOpen()) {
        String chosen = fc.getResults().getReference(0).getFullPathName();
        ScopedPointer<AlertWindow> alert = new AlertWindow ("Open Project !", 
                                                            "You want to load : " + chosen + "\nEverything not saved will be lost !", 
                                                            AlertWindow::WarningIcon);
        alert->setLookAndFeel(&mGrisFeel);
        alert->addButton ("Cancel", 0);
        alert->addButton ("Ok", 1);
        int status = alert->runModalLoop();
        if (status == 1) {
            this->openPreset(chosen);
        }
    }
}

void MainContentComponent::handleSavePreset() {
    if (! File(this->pathCurrentPreset).existsAsFile() || this->pathCurrentPreset.endsWith("default_preset/default_preset.xml")) {
        this->handleSaveAsPreset();
    }
    this->savePreset(this->pathCurrentPreset);
}

void MainContentComponent::handleSaveAsPreset() {
    String dir = this->applicationProperties.getUserSettings()->getValue("lastPresetDirectory");
    if (! File(dir).isDirectory()) {
        dir = File("~").getFullPathName();
    }
    String filename = File(this->pathCurrentPreset).getFileName();

#ifdef __linux__
    FileChooser fc ("Choose a file to save...", dir + "/" + filename, "*.xml", false);
#else
    FileChooser fc ("Choose a file to save...", dir + "/" + filename, "*.xml", true);
#endif
    if (fc.browseForFileToSave (true)) {
        String chosen = fc.getResults().getReference(0).getFullPathName();
        this->savePreset(chosen);
    }
}

void MainContentComponent::handleOpenSpeakerSetup() {
    String dir = this->applicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory");
    if (! File(dir).isDirectory()) {
        dir = File("~").getFullPathName();
    }
    String filename = File(this->pathCurrentFileSpeaker).getFileName();
#ifdef __linux__
    FileChooser fc ("Choose a file to open...", dir + "/" + filename, "*.xml", false);
#else
    FileChooser fc ("Choose a file to open...", dir + "/" + filename, "*.xml", true);
#endif
    if (fc.browseForFileToOpen()) {
        String chosen = fc.getResults().getReference(0).getFullPathName();
        ScopedPointer<AlertWindow> alert = new AlertWindow ("Load Speaker Setup !", 
                                                            "You want to load : " + chosen + "\nEverything not saved will be lost !", 
                                                            AlertWindow::WarningIcon);
        alert->setLookAndFeel(&mGrisFeel);
        alert->addButton ("Cancel", 0);
        alert->addButton ("Ok", 1);
        int status = alert->runModalLoop();
        if (status == 1) {
            this->openXmlFileSpeaker(chosen);
        }
    }
}

// Should we call "this->updateLevelComp(); before saving to validate that the setup is legal?"
void MainContentComponent::handleSaveSpeakerSetup() {
    if (! File(this->pathCurrentFileSpeaker).existsAsFile() || this->pathCurrentFileSpeaker.endsWith("default_preset/default_speaker_setup.xml")) {
        this->handleSaveAsSpeakerSetup();
    }
    this->saveSpeakerSetup(this->pathCurrentFileSpeaker);
}

void MainContentComponent::handleSaveAsSpeakerSetup() {
    String dir = this->applicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory");
    if (! File(dir).isDirectory()) {
        dir = File("~").getFullPathName();
    }
    String filename = File(this->pathCurrentFileSpeaker).getFileName();

#ifdef __linux__
    FileChooser fc ("Choose a file to save...", dir + "/" + filename, "*.xml", false);
#else
    FileChooser fc ("Choose a file to save...", dir + "/" + filename, "*.xml", true);
#endif
    if (fc.browseForFileToSave (true)) {
        String chosen = fc.getResults().getReference(0).getFullPathName();
        this->saveSpeakerSetup(chosen);
    }
}

void MainContentComponent::handleShowSpeakerEditWindow() {
    Rectangle<int> result (this->getScreenX() + this->speakerView->getWidth() + 20, this->getScreenY() + 20, 850, 600);
    if (this->winSpeakConfig == nullptr) {
        this->winSpeakConfig = new WindowEditSpeaker("Speakers Setup Edition - " + File(this->pathCurrentFileSpeaker).getFileName(),
                                                     this->nameConfig, this->mGrisFeel.getWinBackgroundColour(),
                                                     DocumentWindow::allButtons, this, &this->mGrisFeel);
        this->winSpeakConfig->setBounds(result);
        this->winSpeakConfig->initComp();
    }
    this->winSpeakConfig->setBounds(result);
    this->winSpeakConfig->setResizable(true, true);
    this->winSpeakConfig->setUsingNativeTitleBar(true);
    this->winSpeakConfig->setVisible(true);
    this->winSpeakConfig->setAlwaysOnTop(true);
    this->winSpeakConfig->repaint();
}

void MainContentComponent::handleShowPreferences() {
    PropertiesFile *props = this->applicationProperties.getUserSettings();
    if (this->windowProperties == nullptr) {
        unsigned int BufferValue = props->getIntValue("BufferValue", 1024);
        unsigned int RateValue = props->getIntValue("RateValue", 48000);
        unsigned int FileFormat = props->getIntValue("FileFormat", 0);
        unsigned int OscInputPort = props->getIntValue("OscInputPort", 18032);
        if (isnan(BufferValue) || BufferValue == 0) { BufferValue = 1024; }
        if (isnan(RateValue) || RateValue == 0) { RateValue = 48000; }
        if (isnan(FileFormat)) { FileFormat = 0; }
        if (isnan(OscInputPort)) { OscInputPort = 18032; }
        this->windowProperties = new WindowProperties("Preferences", this->mGrisFeel.getWinBackgroundColour(),
                                                     DocumentWindow::allButtons, this, &this->mGrisFeel, 
                                                     RateValues.indexOf(String(RateValue)), 
                                                     BufferSize.indexOf(String(BufferValue)),
                                                     FileFormat, OscInputPort);
    }
    Rectangle<int> result (this->getScreenX()+ (this->speakerView->getWidth()/2)-150, this->getScreenY()+(this->speakerView->getHeight()/2)-75, 270, 290);
    this->windowProperties->setBounds(result);
    this->windowProperties->setResizable(false, false);
    this->windowProperties->setUsingNativeTitleBar(true);
    this->windowProperties->setVisible(true);
    this->windowProperties->repaint();
}

void MainContentComponent::handleShow2DView() {
    if (this->winControlSource == nullptr) {
        this->winControlSource = new WinControl("2D View", this->mGrisFeel.getWinBackgroundColour(), DocumentWindow::allButtons, this, &this->mGrisFeel);
        this->winControlSource->setTimerHz(this->isHighPerformance ? HertzRefresh2DLowCpu : HertzRefreshNormal);
    }
    Rectangle<int> result (this->getScreenX()+ this->speakerView->getWidth()+22,this->getScreenY()+100,500,500);
    this->winControlSource->setBounds(result);
    this->winControlSource->setResizable(true, true);
    this->winControlSource->setUsingNativeTitleBar(true);
    this->winControlSource->setVisible(true);
}

void MainContentComponent::handleShowAbout() {
    if (this->aboutWindow == nullptr) {
        this->aboutWindow = new AboutWindow("About ServerGRIS", this->mGrisFeel.getWinBackgroundColour(),
                                            DocumentWindow::allButtons, this, &this->mGrisFeel);
    }
    this->aboutWindow->centreWithSize(400, 500);
    this->aboutWindow->setResizable(false, false);
    this->aboutWindow->setUsingNativeTitleBar(true);
    this->aboutWindow->setVisible(true);
    this->aboutWindow->repaint();
}

void MainContentComponent::handleOpenManual() {
    File fs;
#ifdef __linux__
    fs = File ( File::getCurrentWorkingDirectory().getFullPathName() + ("/../../Resources/ServerGRIS_1.0_Manual.pdf"));
#else
    String cwd = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
    fs = File (cwd + ("/Contents/Resources/ServerGRIS_1.0_Manual.pdf"));
#endif
    if(fs.exists()) {
        juce::Process::openDocument("file:" + fs.getFullPathName(), juce::String::empty);
    }
}

void MainContentComponent::handleShowNumbers() {
    this->setShowNumbers(!this->isNumbersShown);
}

void MainContentComponent::setShowNumbers(bool state) {
    this->isNumbersShown = state;
    this->speakerView->setShowNumber(state);
}

void MainContentComponent::handleShowSpeakers() {
    this->setShowSpeakers(!this->isSpeakersShown);
}

void MainContentComponent::setShowSpeakers(bool state) {
    this->isSpeakersShown = state;
    this->speakerView->setHideSpeaker(!state);
}

void MainContentComponent::handleShowTriplets() {
    this->setShowTriplets(!this->isTripletsShown);
}

void MainContentComponent::setShowTriplets(bool state) {
    if (this->validateShowTriplets() || state == false) {
        this->isTripletsShown = state;
        this->speakerView->setShowTriplets(state);
    } else {
        ScopedPointer<AlertWindow> alert = new AlertWindow("Can't draw all triplets !",
                                                           "Maybe you didn't compute your current speaker setup ?",
                                                           AlertWindow::InfoIcon);
        alert->addButton("Close", 0);
        alert->runModalLoop();
        this->setShowTriplets(false);
    }
}

bool MainContentComponent::validateShowTriplets() {
    int success = true;
    for (unsigned int i = 0; i < this->getListTriplet().size(); ++i) {
        Speaker *spk1 = this->getSpeakerFromOutputPatch(this->getListTriplet()[i].id1);
        Speaker *spk2 = this->getSpeakerFromOutputPatch(this->getListTriplet()[i].id2);
        Speaker *spk3 = this->getSpeakerFromOutputPatch(this->getListTriplet()[i].id3);

        if (spk1 == nullptr || spk2 == nullptr || spk3 == nullptr) {
            success = false;
            break;
        }
    }
    return success;
}

void MainContentComponent::handleShowSourceLevel() {
    this->isSourceLevelShown = !this->isSourceLevelShown;
}

void MainContentComponent::handleShowSpeakerLevel() {
    this->isSpeakerLevelShown = !this->isSpeakerLevelShown;
}

void MainContentComponent::handleShowSphere() {
    this->isSphereShown = !this->isSphereShown;
    this->speakerView->setShowSphere(this->isSphereShown);
}

void MainContentComponent::handleHighPerformance() {
    this->setHighPerformance(!this->isHighPerformance);
}

void MainContentComponent::setHighPerformance(bool state) {
    // HighPerformance removed until proof that we need it.
    return; // Just in case HighPerformance is set in a saved project.
    this->isHighPerformance = state;
    this->stopTimer();
    if (state) {
        startTimerHz(HertzRefreshLowCpu);
        if (this->winControlSource != nullptr) {
            this->winControlSource->setTimerHz(HertzRefresh2DLowCpu);
        }
    } else {
        startTimerHz(HertzRefreshNormal);
        if (this->winControlSource != nullptr) {
            this->winControlSource->setTimerHz(HertzRefreshNormal);
        }
    }
    this->speakerView->setHighPerfor(state);

    this->isSourceLevelShown = false;
    this->isSpeakerLevelShown = false;
}

void MainContentComponent::handleResetInputPositions() {
    for (auto&& it : this->listSourceInput) {
        it->resetPosition();
    }
}

void MainContentComponent::handleInputColours() {
    float hue = 0.0f;
    float inc = 1.0 / (this->listSourceInput.size() + 1);
    for (auto&& it : this->listSourceInput) {
        it->setColor(Colour::fromHSV(hue, 1, 0.75, 1), true);
        hue += inc;
    }
}

//=====================================================
void MainContentComponent::getAllCommands (Array<CommandID>& commands)
{
    // this returns the set of all commands that this target can perform..
    const CommandID ids[] = { MainWindow::NewPresetID,
                              MainWindow::OpenPresetID,
                              MainWindow::SavePresetID,
                              MainWindow::SaveAsPresetID,
                              MainWindow::OpenSpeakerSetupID,
                              MainWindow::SaveSpeakerSetupID,
                              MainWindow::SaveAsSpeakerSetupID,
                              MainWindow::ShowSpeakerEditID,
                              MainWindow::Show2DViewID,
                              MainWindow::ShowNumbersID,
                              MainWindow::ShowSpeakersID,
                              MainWindow::ShowTripletsID,
                              MainWindow::ShowSourceLevelID,
                              MainWindow::ShowSpeakerLevelID,
                              MainWindow::ShowSphereID,
                              MainWindow::HighPerformanceID,
                              MainWindow::ColorizeInputsID,
                              MainWindow::ResetInputPosID,
                              MainWindow::PrefsID,
                              MainWindow::QuitID,
                              MainWindow::AboutID,
                              MainWindow::OpenManualID,
                            };

    commands.addArray (ids, numElementsInArray(ids));
}

void MainContentComponent::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    const String generalCategory ("General");

    switch (commandID)
    {
        case MainWindow::NewPresetID:
            result.setInfo ("New Project", "Close the current preset and open the default.", generalCategory, 0);
            result.addDefaultKeypress ('N', ModifierKeys::commandModifier);
            break;
        case MainWindow::OpenPresetID:
            result.setInfo ("Open Project", "Choose a new preset on disk.", generalCategory, 0);
            result.addDefaultKeypress ('O', ModifierKeys::commandModifier);
            break;
        case MainWindow::SavePresetID:
            result.setInfo ("Save Project", "Save the current preset on disk.", generalCategory, 0);
            result.addDefaultKeypress ('S', ModifierKeys::commandModifier);
            break;
        case MainWindow::SaveAsPresetID:
            result.setInfo ("Save Project As...", "Save the current preset under a new name on disk.", generalCategory, 0);
            result.addDefaultKeypress ('S', ModifierKeys::shiftModifier|ModifierKeys::commandModifier);
            break;
        case MainWindow::OpenSpeakerSetupID:
            result.setInfo ("Load Speaker Setup", "Choose a new speaker setup on disk.", generalCategory, 0);
            result.addDefaultKeypress ('L', ModifierKeys::commandModifier);
            break;
        case MainWindow::SaveSpeakerSetupID:
            result.setInfo ("Export Speaker Setup", "Save the current speaker setup on disk.", generalCategory, 0);
            result.addDefaultKeypress ('E', ModifierKeys::commandModifier);
            break;
        case MainWindow::SaveAsSpeakerSetupID:
            result.setInfo ("Export Speaker Setup As...", "Save the current speaker setup under a new name on disk.", generalCategory, 0);
            result.addDefaultKeypress ('E', ModifierKeys::shiftModifier|ModifierKeys::commandModifier);
            break;
        case MainWindow::ShowSpeakerEditID:
            result.setInfo ("Show Speaker Setup Edition Window", "Edit the current speaker setup.", generalCategory, 0);
            result.addDefaultKeypress ('W', ModifierKeys::altModifier);
            break;
        case MainWindow::Show2DViewID:
            result.setInfo ("Show 2D View", "Show the 2D action window.", generalCategory, 0);
            result.addDefaultKeypress ('D', ModifierKeys::altModifier);
            break;
        case MainWindow::ShowNumbersID:
            result.setInfo ("Show Numbers", "Show source and speaker numbers on the 3D view.", generalCategory, 0);
            result.addDefaultKeypress ('N', ModifierKeys::altModifier);
            result.setTicked(this->isNumbersShown);
            break;
        case MainWindow::ShowSpeakersID:
            result.setInfo ("Show Speakers", "Show speakers on the 3D view.", generalCategory, 0);
            result.addDefaultKeypress ('S', ModifierKeys::altModifier);
            result.setTicked(this->isSpeakersShown);
            break;
        case MainWindow::ShowTripletsID:
            result.setInfo ("Show Speaker Triplets", "Show speaker triplets on the 3D view.", generalCategory, 0);
            result.addDefaultKeypress ('T', ModifierKeys::altModifier);
            result.setTicked(this->isTripletsShown);
            break;
        case MainWindow::ShowSourceLevelID:
            result.setInfo ("Show Source Activity", "Activate brightness on sources on the 3D view.", generalCategory, 0);
            result.addDefaultKeypress ('A', ModifierKeys::altModifier);
            result.setTicked(this->isSourceLevelShown);
            break;
        case MainWindow::ShowSpeakerLevelID:
            result.setInfo ("Show Speaker Level", "Activate brightness on speakers on the 3D view.", generalCategory, 0);
            result.addDefaultKeypress ('L', ModifierKeys::altModifier);
            result.setTicked(this->isSpeakerLevelShown);
            break;
        case MainWindow::ShowSphereID:
            result.setInfo ("Show Sphere", "Show the sphere on the 3D view.", generalCategory, 0);
            result.addDefaultKeypress ('O', ModifierKeys::altModifier);
            result.setTicked(this->isSphereShown);
            break;
        case MainWindow::HighPerformanceID:
            result.setInfo ("High Performance", "Lower the CPU usage on the graphical display.", generalCategory, 0);
            result.addDefaultKeypress ('H', ModifierKeys::altModifier);
            result.setTicked(this->isHighPerformance);
            break;
        case MainWindow::ColorizeInputsID:
            result.setInfo ("Colorize Inputs", "Spread the colour of the inputs over the colour range.", generalCategory, 0);
            result.addDefaultKeypress ('C', ModifierKeys::altModifier);
            break;
        case MainWindow::ResetInputPosID:
            result.setInfo ("Reset Input Position", "Reset the position of the input sources.", generalCategory, 0);
            result.addDefaultKeypress ('R', ModifierKeys::altModifier);
            break;
        case MainWindow::PrefsID:
            result.setInfo ("Preferences...", "Open the preferences window.", generalCategory, 0);
            result.addDefaultKeypress (';', ModifierKeys::commandModifier);
            break;
        case MainWindow::QuitID:
            result.setInfo ("Quit", "Quit the ServerGris.", generalCategory, 0);
            result.addDefaultKeypress ('Q', ModifierKeys::commandModifier);
            break;
        case MainWindow::AboutID:
            result.setInfo ("About ServerGRIS", "Open the about window.", generalCategory, 0);
            break;
        case MainWindow::OpenManualID:
            result.setInfo ("Open Documentation", "Open the manual in pdf viewer.", generalCategory, 0);
            break;
        default:
            break;
    }
}

bool MainContentComponent::perform (const InvocationInfo& info)
{
    if (auto* mainWindow = MainWindow::getMainAppWindow())
    {
        switch (info.commandID)
        {
            case MainWindow::NewPresetID: this->handleNew(); break;
            case MainWindow::OpenPresetID: this->handleOpenPreset(); break;
            case MainWindow::SavePresetID: this->handleSavePreset(); break;
            case MainWindow::SaveAsPresetID: this->handleSaveAsPreset(); break;
            case MainWindow::OpenSpeakerSetupID: this->handleOpenSpeakerSetup(); break;
            case MainWindow::SaveSpeakerSetupID: this->handleSaveSpeakerSetup(); break;
            case MainWindow::SaveAsSpeakerSetupID: this->handleSaveAsSpeakerSetup(); break;
            case MainWindow::ShowSpeakerEditID: this->handleShowSpeakerEditWindow(); break;
            case MainWindow::Show2DViewID: this->handleShow2DView(); break;
            case MainWindow::ShowNumbersID: this->handleShowNumbers(); break;
            case MainWindow::ShowSpeakersID: this->handleShowSpeakers(); break;
            case MainWindow::ShowTripletsID: this->handleShowTriplets(); break;
            case MainWindow::ShowSourceLevelID: this->handleShowSourceLevel(); break;
            case MainWindow::ShowSpeakerLevelID: this->handleShowSpeakerLevel(); break;
            case MainWindow::ShowSphereID: this->handleShowSphere(); break;
            case MainWindow::HighPerformanceID: this->handleHighPerformance(); break;
            case MainWindow::ColorizeInputsID: this->handleInputColours(); break;
            case MainWindow::ResetInputPosID: this->handleResetInputPositions(); break;
            case MainWindow::PrefsID: this->handleShowPreferences(); break;
            case MainWindow::QuitID: dynamic_cast<MainWindow*>(this->parent)->closeButtonPressed(); break;
            case MainWindow::AboutID: this->handleShowAbout(); break;
            case MainWindow::OpenManualID: this->handleOpenManual(); break;
            default:
                return false;
        }
    }
    return true;
}

PopupMenu MainContentComponent::getMenuForIndex (int menuIndex, const String& menuName) {

    ApplicationCommandManager* commandManager = &MainWindow::getApplicationCommandManager();

    PopupMenu menu;

    if (menuName == "File")
    {
        menu.addCommandItem(commandManager, MainWindow::NewPresetID);
        menu.addCommandItem(commandManager, MainWindow::OpenPresetID);
        menu.addCommandItem(commandManager, MainWindow::SavePresetID);
        menu.addCommandItem(commandManager, MainWindow::SaveAsPresetID);
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::OpenSpeakerSetupID);
        // Speaker Setup saving is handled when we close the Speaker Setup window.
        //menu.addCommandItem(commandManager, MainWindow::SaveSpeakerSetupID);
        //menu.addCommandItem(commandManager, MainWindow::SaveAsSpeakerSetupID);
        menu.addSeparator();
        menu.addCommandItem (commandManager, MainWindow::PrefsID);
#if ! JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::QuitID);
#endif
    }
    else if (menuName == "View")
    {
        menu.addCommandItem(commandManager, MainWindow::Show2DViewID);
        menu.addCommandItem(commandManager, MainWindow::ShowSpeakerEditID);
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::ShowNumbersID);
        menu.addCommandItem(commandManager, MainWindow::ShowSpeakersID);
        if (this->jackClient->vbapDimensions == 3) {
            menu.addCommandItem(commandManager, MainWindow::ShowTripletsID);
        } else {
            menu.addItem(MainWindow::ShowTripletsID, "Show Speaker Triplets",
                         false, false);
        }
        menu.addCommandItem(commandManager, MainWindow::ShowSourceLevelID);
        menu.addCommandItem(commandManager, MainWindow::ShowSpeakerLevelID);
        menu.addCommandItem(commandManager, MainWindow::ShowSphereID);
        // HighPerformance removed until proof that we need it.
        //menu.addCommandItem(commandManager, MainWindow::HighPerformanceID);
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::ColorizeInputsID);
        menu.addCommandItem(commandManager, MainWindow::ResetInputPosID);
    }
    else if (menuName == "Help")
    {
        menu.addCommandItem(commandManager, MainWindow::AboutID);
        menu.addCommandItem(commandManager, MainWindow::OpenManualID);
    }
    return menu;
}

void MainContentComponent::menuItemSelected (int menuItemID, int /*topLevelMenuIndex*/)
{
    switch (menuItemID) {}
}

//=====================================================
bool MainContentComponent::exitApp()
{
    ScopedPointer<AlertWindow> alert = new AlertWindow("Exit ServerGRIS !",
                                                       "Do you want to save the current project ?",
                                                       AlertWindow::InfoIcon);
    alert->setLookAndFeel(&mGrisFeel);
    alert->addButton ("Save", 1);
    alert->addButton ("Cancel", 0);
    alert->addButton ("Exit", 2);
    int exitV = alert->runModalLoop();
    if (exitV == 1) {
        alert->setVisible(false);
        ModalComponentManager::getInstance()->cancelAllModalComponents();
        String dir = this->applicationProperties.getUserSettings()->getValue("lastPresetDirectory");
        if (! File(dir).isDirectory()) {
            dir = File("~").getFullPathName();
        }
        String filename = File(this->pathCurrentPreset).getFileName();
#ifdef __linux__
        FileChooser fc ("Choose a file to save...", dir + "/" + filename, "*.xml", false);
#else
        FileChooser fc ("Choose a file to save...", dir + "/" + filename, "*.xml", true);
#endif
        if (fc.browseForFileToSave(true)) {
            String chosen = fc.getResults().getReference(0).getFullPathName();
            this->savePreset(chosen);
        } else {
            exitV = 0;
        }
    }
    return (exitV != 0); 
}

MainContentComponent::~MainContentComponent()
{
    this->applicationProperties.getUserSettings()->setValue("lastOpenPreset", this->pathCurrentPreset);
    this->applicationProperties.getUserSettings()->setValue("lastOpenSpeakerSetup", this->pathCurrentFileSpeaker);
    this->applicationProperties.saveIfNeeded();
    this->applicationProperties.closeFiles();

    delete this->oscReceiver;
    
    if(this->winSpeakConfig != nullptr){
        delete this->winSpeakConfig;
    }
    if(this->windowProperties != nullptr){
        delete this->windowProperties;
    }
    if(this->winControlSource != nullptr){
        delete this->winControlSource;
    }

/* FIXME
 * This call segfaults on linux.
 */
#ifndef __linux__
    delete this->speakerView;
#endif

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
    delete this->boxMainUI;

    delete  this->jackClient;
    delete this->jackServer;
}

void MainContentComponent::loadVbapHrtfSpeakerSetup() {
#ifdef __linux__
    String cwd = File::getCurrentWorkingDirectory().getFullPathName();
    this->openXmlFileSpeaker(cwd + ("/../../Resources/default_preset/vbap_hrtf_speaker_setup.xml"));
#else
    String cwd = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
    this->openXmlFileSpeaker(cwd + ("/Contents/Resources/default_preset/vbap_hrtf_speaker_setup.xml"));
#endif
}

void MainContentComponent::selectSpeaker(int idS)
{
    for(unsigned int i = 0; i < this->listSpeaker.size(); ++i) {
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

void MainContentComponent::selectTripletSpeaker(int idS)
{
    int countS = 0;
    for(unsigned int i = 0; i < this->listSpeaker.size(); ++i) {
        if(this->listSpeaker[i]->isSelected()){
            countS+=1;
        }
    }

    if(!this->listSpeaker[idS]->isSelected() && countS < 3){
        this->listSpeaker[idS]->selectSpeaker();
        countS+=1;
    }else{
        this->listSpeaker[idS]->unSelectSpeaker();
    }

    if(countS == 3){
        int i1 = -1, i2= -1, i3 = -1;
        for(unsigned int i = 0; i < this->listSpeaker.size(); ++i) {
            if(this->listSpeaker[i]->isSelected()){
                if(i1 == -1){
                    i1=i;
                }else{
                    if(i2 == -1){
                        i2=i;
                    }else{
                        if(i3 == -1){
                            i3=i;
                        }
                    }
                }
            }
        }
        if(i1 != -1 && i2 != -1 && i3 != -1){
            Triplet tri;
            tri.id1 = i1;
            tri.id2 = i2;
            tri.id3 = i3;
            int posDel = -1;
            if(this->tripletExist(tri, posDel)){
                this->listTriplet.erase(this->listTriplet.begin() + posDel);
            }else{
                
                this->listTriplet.push_back(tri);
            }
        }

    }
}

bool MainContentComponent::tripletExist(Triplet tri, int &pos)
{
    pos = 0;
    for (auto&& ti : this->listTriplet)
    {
        if((ti.id1 == tri.id1 && ti.id2 == tri.id2 && ti.id3 == tri.id3) ||
           (ti.id1 == tri.id1 && ti.id2 == tri.id3 && ti.id3 == tri.id2) ||
           (ti.id1 == tri.id2 && ti.id2 == tri.id1 && ti.id3 == tri.id3) ||
           (ti.id1 == tri.id2 && ti.id2 == tri.id3 && ti.id3 == tri.id1) ||
           (ti.id1 == tri.id3 && ti.id2 == tri.id2 && ti.id3 == tri.id1) ||
           (ti.id1 == tri.id3 && ti.id2 == tri.id1 && ti.id3 == tri.id2)
           ){
            return true;
        }
        pos += 1;
    }
    
    return false;
}

void MainContentComponent::addSpeaker()
{
    this->lockSpeakers->lock();
    unsigned int idNewSpeaker = (unsigned int)listSpeaker.size()+1;
    this->listSpeaker.push_back(new Speaker(this, idNewSpeaker, idNewSpeaker, glm::vec3(0.0f, 0.0f, 10.0f)));
    this->lockSpeakers->unlock();
    this->jackClient->addOutput(idNewSpeaker);
}

void MainContentComponent::removeSpeaker(int idSpeaker)
{
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
}

void MainContentComponent::updateInputJack(int inInput, Input &inp)
{
    SourceIn *si = &this->jackClient->listSourceIn[inInput]; //.getReference(inInput);

    si->x = inp.getCenter().x/10.0f;
    si->y = inp.getCenter().y/10.0f;
    si->z = inp.getCenter().z/10.0f;

    si->azimuth = ((inp.getAziMuth()/M2_PI)*360.0f);
    if (si->azimuth > 180.0f) {
        si->azimuth = si->azimuth-360.0f;
    }
    si->zenith  = 90.0f-(inp.getZenith()/M2_PI)*360.0f;
    si->radius  = inp.getRad();
    
    si->aziSpan = inp.getAziMuthSpan() * 0.5f;
    si->zenSpan = inp.getZenithSpan() * 2.0f;
    
    if (this->jackClient->modeSelected == VBap) {
        this->jackClient->vbapSourcesToUpdate[inInput] = 1;
    } else if (this->jackClient->modeSelected == VBap_HRTF) {
        this->jackClient->vbapSourcesToUpdate[inInput] = 1;
    } else if (this->jackClient->modeSelected == STEREO) {
        // nothing to do yet.
    }
}

void MainContentComponent::setListTripletFromVbap() {
    this->clearListTriplet();
    for (unsigned int i=0; i<this->jackClient->vbap_triplets.size(); i++) {
        Triplet tri;
        tri.id1 = this->jackClient->vbap_triplets[i][0];
        tri.id2 = this->jackClient->vbap_triplets[i][1];
        tri.id3 = this->jackClient->vbap_triplets[i][2];
        this->listTriplet.push_back(tri);
    }
}

Speaker *
MainContentComponent::getSpeakerFromOutputPatch(int out) {
    for (auto&& it : this->listSpeaker) {
        if (it->getOutputPatch() == out && !it->getDirectOut()) {
            return it;
        }
    }
    return nullptr;
}

static void
Linkwitz_Riley_compute_variables(double freq, double sr, double **coeffs, int length) {

    double wc = 2 * M_PI * freq;
    double wc2 = wc * wc;
    double wc3 = wc2 * wc;
    double wc4 = wc2 * wc2;
    double k = wc / tan(M_PI * freq / sr);
    double k2 = k * k;
    double k3 = k2 * k;
    double k4 = k2 * k2;
    double sqrt2 = sqrt(2.0);
    double sq_tmp1 = sqrt2 * wc3 * k;
    double sq_tmp2 = sqrt2 * wc * k3;
    double a_tmp = 4.0 * wc2 * k2 + 2.0 * sq_tmp1 + k4 + 2.0 * sq_tmp2 + wc4;
    double k4_a_tmp = k4 / a_tmp;

    *coeffs = (double *)malloc(length * sizeof(double));

    /* common */
    double b1 = (4.0 * (wc4 + sq_tmp1 - k4 - sq_tmp2)) / a_tmp;
    double b2 = (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / a_tmp;
    double b3 = (4.0 * (wc4 - sq_tmp1 + sq_tmp2 - k4)) / a_tmp;
    double b4 = (k4 - 2.0 * sq_tmp1 + wc4 - 2.0 * sq_tmp2 + 4.0 * wc2 * k2) / a_tmp;

    /* highpass */
    double ha0 = k4_a_tmp;
    double ha1 = -4.0 * k4_a_tmp;
    double ha2 = 6.0 * k4_a_tmp;

    (*coeffs)[0] = b1; (*coeffs)[1] = b2; (*coeffs)[2] = b3; (*coeffs)[3] = b4;
    (*coeffs)[4] = ha0; (*coeffs)[5] = ha1; (*coeffs)[6] = ha2; 
}

bool MainContentComponent::updateLevelComp() {
    int dimensions = 2;
    int x = 2;
    int indexS = 0;
    int directOutSpeakers = 0;

    if (this->listSpeaker.size() == 0)
        return false;

    // Test for a 2-D or 3-D configuration
    float zenith = -1.0f;
    for (auto&& it : this->listSpeaker) {
        if (it->getDirectOut()) {
            directOutSpeakers++;
            continue;
        }
        if (zenith == -1.0f) {
            zenith = it->getAziZenRad().y;
        }
        else if (it->getAziZenRad().y < (zenith - 4.9) || it->getAziZenRad().y > (zenith + 4.9)) {
            dimensions = 3;
        }
    }

    if ((this->listSpeaker.size() - directOutSpeakers) < dimensions) {
        ScopedPointer<AlertWindow> alert = new AlertWindow ("Not enough speakers!    ",
                                                            "Do you want to reload previous config?    ", 
                                                            AlertWindow::WarningIcon);
        alert->setLookAndFeel(&mGrisFeel);
        alert->addButton ("No", 0);
        alert->addButton ("Yes", 1);
        int ret = alert->runModalLoop();
        if (ret == 1) {
            this->openXmlFileSpeaker(this->pathCurrentFileSpeaker);
        }
        return false;
    }

    this->jackClient->processBlockOn = false;
    this->jackClient->maxOutputPatch = 0;

    // Save mute/solo/directout states
    bool inputsIsMuted[MaxInputs];
    bool inputsIsSolo[MaxInputs];
    bool soloIn = this->jackClient->soloIn;
    int directOuts[MaxInputs];
    for (unsigned int i = 0; i < MaxInputs; i++) {
        inputsIsMuted[i] = (&this->jackClient->listSourceIn[i])->isMuted;
        inputsIsSolo[i] = (&this->jackClient->listSourceIn[i])->isSolo;
        directOuts[i] = (&this->jackClient->listSourceIn[i])->directOut;
    }

    bool outputsIsMuted[MaxInputs];
    bool outputsIsSolo[MaxInputs];
    bool soloOut = this->jackClient->soloOut;
    for (int i = 0; i < MaxOutputs; i++) {
        outputsIsMuted[i] = (&this->jackClient->listSpeakerOut[i])->isMuted;
        outputsIsSolo[i] = (&this->jackClient->listSpeakerOut[i])->isSolo;
    }


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
        so.zenith  = it->getAziZenRad().y;
        so.radius  = it->getAziZenRad().z;

        so.outputPatch = it->getOutputPatch();

        so.directOut = it->getDirectOut();
        
        this->jackClient->listSpeakerOut[i] = so;
        i++;

        if (it->getOutputPatch() > this->jackClient->maxOutputPatch)
            this->jackClient->maxOutputPatch = it->getOutputPatch();
    }

    // Set user gain and highpass filter cutoff frequency for each speaker.
    i = 0;
    for (auto&& it : this->listSpeaker) {
        this->jackClient->listSpeakerOut[it->outputPatch-1].gain = pow(10.0, it->getGain() * 0.05);
        if (it->getHighPassCutoff() > 0.0f) {
            double *coeffs;
            Linkwitz_Riley_compute_variables((double)it->getHighPassCutoff(), (double)this->samplingRate, &coeffs, 7);
            this->jackClient->listSpeakerOut[it->outputPatch-1].b1 = coeffs[0];
            this->jackClient->listSpeakerOut[it->outputPatch-1].b2 = coeffs[1];
            this->jackClient->listSpeakerOut[it->outputPatch-1].b3 = coeffs[2];
            this->jackClient->listSpeakerOut[it->outputPatch-1].b4 = coeffs[3];
            this->jackClient->listSpeakerOut[it->outputPatch-1].ha0 = coeffs[4];
            this->jackClient->listSpeakerOut[it->outputPatch-1].ha1 = coeffs[5];
            this->jackClient->listSpeakerOut[it->outputPatch-1].ha2 = coeffs[6];
            this->jackClient->listSpeakerOut[it->outputPatch-1].hpActive = true;
            free(coeffs);
        }
    i++;
    }
    
    x = 2;
    indexS = 0;
    i=0;
    this->lockInputs->lock();
    for (auto&& it : this->listSourceInput)
    {
        juce::Rectangle<int> level(x, 4, SizeWidthLevelComp, 200);
        it->getVuMeter()->setBounds(level);
        it->getVuMeter()->updateDirectOutMenu(this->listSpeaker);
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
        si.zenith  = it->getZenith();
        si.radius  = it->getRad();
        
        this->jackClient->listSourceIn[i] = si;
        i++;
    }
    this->lockInputs->unlock();
    if(this->winSpeakConfig != nullptr){
        this->winSpeakConfig->updateWinContent();
    }

    this->boxOutputsUI->repaint();
    this->resized();

    this->jackClient->vbapDimensions = dimensions;
    if (dimensions == 2) {
        this->setShowTriplets(false);
    }

    i = 0;
    vector<Speaker *> tempListSpeaker;
    tempListSpeaker.resize(this->listSpeaker.size());
    for (auto&& it : this->listSpeaker) {
        if (! it->getDirectOut()) {
            tempListSpeaker[i++] = it;
        }
    }
    tempListSpeaker.resize(i);
    bool retval = this->jackClient->initSpeakersTripplet(tempListSpeaker, dimensions, this->needToComputeVbap);

    if (retval) {
        this->setListTripletFromVbap();
        this->needToComputeVbap = false;
    }

    // Restore mute/solo/directout states
    this->jackClient->soloIn = soloIn;
    for (unsigned int i = 0; i < MaxInputs; i++) {
        (&this->jackClient->listSourceIn[i])->isMuted = inputsIsMuted[i];
        (&this->jackClient->listSourceIn[i])->isSolo = inputsIsSolo[i];
        (&this->jackClient->listSourceIn[i])->directOut = directOuts[i];
    }
    i = 0;
    this->lockInputs->lock();
    for (auto&& it : this->listSourceInput) {
        this->listSourceInput[i]->setDirectOutChannel(directOuts[i]);
        i++;
    }
    this->lockInputs->unlock();

    this->jackClient->soloOut = soloOut;
    for (int i = 0; i < MaxOutputs; i++) {
        (&this->jackClient->listSpeakerOut[i])->isMuted = outputsIsMuted[i];
        (&this->jackClient->listSpeakerOut[i])->isSolo = outputsIsSolo[i];
    }


    this->jackClient->processBlockOn = true;
    return retval;
}


void MainContentComponent::setNameConfig()
{
    this->nameConfig = this->pathCurrentFileSpeaker.fromLastOccurrenceOf("/", false, false);
    this->speakerView->setNameConfig(this->nameConfig);
}

void MainContentComponent::muteInput(int id, bool mute)
{
    (&this->jackClient->listSourceIn[id-1])->isMuted = mute;
}
void MainContentComponent::muteOutput(int id, bool mute)
{
    (&this->jackClient->listSpeakerOut[id-1])->isMuted = mute;
}
void MainContentComponent::soloInput(int id, bool solo)
{
    (&this->jackClient->listSourceIn[id-1])->isSolo = solo;

    this->jackClient->soloIn = false;
    for (unsigned int i = 0; i < MaxInputs; i++) {
        if((&this->jackClient->listSourceIn[i])->isSolo){
            this->jackClient->soloIn = true;
            break;
        }
    }
}
void MainContentComponent::soloOutput(int id, bool solo)
{
    (&this->jackClient->listSpeakerOut[id-1])->isSolo = solo;
    
    this->jackClient->soloOut = false;
    for (unsigned int i = 0; i < MaxOutputs; i++) {
        if((&this->jackClient->listSpeakerOut[i])->isSolo){
            this->jackClient->soloOut = true;
            break;
        }
    }
}

void MainContentComponent::setDirectOut(int id, int chn)
{
    (&this->jackClient->listSourceIn[id-1])->directOut = chn;
}

void MainContentComponent::openXmlFileSpeaker(String path) {
    String msg;
    String oldPath = this->pathCurrentFileSpeaker;
    int isNewSameAsOld = oldPath.compare(path);
    bool ok = false;
    if (! File(path.toStdString()).existsAsFile()) {
        ScopedPointer<AlertWindow> alert = new AlertWindow ("Error in Load Speaker Setup !", 
                                                            "Can't found file " + path.toStdString() + ", the current setup will be kept.", 
                                                            AlertWindow::WarningIcon);
        alert->setLookAndFeel(&mGrisFeel);
        alert->addButton ("Ok", 0);
        alert->runModalLoop();
    } else {
        this->pathCurrentFileSpeaker = path.toStdString();
        this->jackClient->processBlockOn = false;
        XmlDocument xmlDoc (File (this->pathCurrentFileSpeaker));
        ScopedPointer<XmlElement> mainXmlElem (xmlDoc.getDocumentElement());
        if (mainXmlElem == nullptr) {
            ScopedPointer<AlertWindow> alert = new AlertWindow ("Error in Load Speaker Setup !", 
                                                                "Your file is corrupted !\n" + xmlDoc.getLastParseError(), 
                                                                AlertWindow::WarningIcon);
            alert->setLookAndFeel(&mGrisFeel);
            alert->addButton ("Ok", 0);
            alert->runModalLoop();
        } else {
            if (mainXmlElem->hasTagName("SpeakerSetup")) {
                this->lockSpeakers->lock();
                for (auto&& it : this->listSpeaker) {
                    delete (it);
                }
                this->listSpeaker.clear();
                this->lockSpeakers->unlock();
                this->setNameConfig();
                this->jackClient->clearOutput();
                this->jackClient->maxOutputPatch = 0;
                forEachXmlChildElement (*mainXmlElem, ring) {
                    if (ring->hasTagName ("Ring")) {
                        forEachXmlChildElement (*ring, spk) {
                            if (spk->hasTagName ("Speaker")) {
                                this->listSpeaker.push_back(new Speaker(this,
                                                                        spk->getIntAttribute("LayoutIndex"),
                                                                        spk->getIntAttribute("OutputPatch"),
                                                                        glm::vec3(spk->getDoubleAttribute("PositionX")*10.0f,
                                                                                  spk->getDoubleAttribute("PositionZ")*10.0f,
                                                                                  spk->getDoubleAttribute("PositionY")*10.0f)));
                                if (spk->hasAttribute("Gain")) {
                                    this->listSpeaker.back()->setGain(spk->getDoubleAttribute("Gain"));
                                }
                                if (spk->hasAttribute("HighPassCutoff")) {
                                    this->listSpeaker.back()->setHighPassCutoff(spk->getDoubleAttribute("HighPassCutoff"));
                                }
                                if (spk->hasAttribute("DirectOut")) {
                                    this->listSpeaker.back()->setDirectOut(spk->getBoolAttribute("DirectOut"));
                                }
                                this->jackClient->addOutput(spk->getIntAttribute("OutputPatch"));
                            }
                        }
                    }
                    if (ring->hasTagName ("triplet")) {
                        Triplet tri;
                        tri.id1 = ring->getIntAttribute("id1");
                        tri.id2 = ring->getIntAttribute("id2");
                        tri.id3 = ring->getIntAttribute("id3");
                        this->listTriplet.push_back(tri);
                    }
                }
                ok = true;
            } else {
                if (mainXmlElem->hasTagName("ServerGRIS_Preset")) {
                    msg = "You are trying to open a Server document, and not a Speaker Setup !";
                } else {
                    msg = "Your file is corrupted !\n" + xmlDoc.getLastParseError();
                }
                ScopedPointer<AlertWindow> alert = new AlertWindow ("Error in Load Speaker Setup !", 
                                                                    msg, 
                                                                    AlertWindow::WarningIcon);
                alert->setLookAndFeel(&mGrisFeel);
                alert->addButton ("Ok", 0);
                alert->runModalLoop();
            }
        }
    }
    if (ok) {
        if (this->pathCurrentFileSpeaker.endsWith("default_preset/default_speaker_setup.xml")) {
            this->applicationProperties.getUserSettings()->setValue("lastSpeakerSetupDirectory", 
                                                                    File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
        } else {
            this->applicationProperties.getUserSettings()->setValue("lastSpeakerSetupDirectory", 
                                                                    File(this->pathCurrentFileSpeaker).getParentDirectory().getFullPathName());
        }
        if (this->getJackClient()->modeSelected != VBap_HRTF) {
            this->pathLastVbapSpeakerSetup = this->pathCurrentFileSpeaker;
        }
        this->needToComputeVbap = true;
        this->updateLevelComp();
    } else {
        if (isNewSameAsOld == 0) {
#ifdef __linux__
            String cwd = File::getCurrentWorkingDirectory().getFullPathName();
            this->openXmlFileSpeaker(cwd + ("/../../Resources/default_preset/default_speaker_setup.xml"));
#else
            String cwd = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
            this->openXmlFileSpeaker(cwd + ("/Contents/Resources/default_preset/default_speaker_setup.xml"));
#endif
        } else {
            this->openXmlFileSpeaker(oldPath);
        }
    }
}

void MainContentComponent::setTitle() {
    String version = STRING(JUCE_APP_VERSION);
    version = "ServerGRIS v" + version + " - ";
    this->parent->setName(version + File(this->pathCurrentPreset).getFileName());
}

void MainContentComponent::openPreset(String path)
{
    String msg;
    this->jackClient->processBlockOn = false;
    File xmlFile = File(path.toStdString());
    XmlDocument xmlDoc (xmlFile);
    ScopedPointer<XmlElement> mainXmlElem (xmlDoc.getDocumentElement());
    if (mainXmlElem == nullptr) {
        ScopedPointer<AlertWindow> alert = new AlertWindow ("Error in Open Preset !", 
                                                            "Your file is corrupted !\n"+ path.toStdString() + "\n" + xmlDoc.getLastParseError(), 
                                                            AlertWindow::WarningIcon);
        alert->setLookAndFeel(&mGrisFeel);
        alert->addButton ("Ok", 1);
        alert->runModalLoop();
    } else {
        if (mainXmlElem->hasTagName("SpatServerGRIS_Preset") || mainXmlElem->hasTagName("ServerGRIS_Preset")) {
            this->pathCurrentPreset = path;
            this->oscInputPort = mainXmlElem->getIntAttribute("OSC_Input_Port"); // app preferences instead of project setting?
            this->tedAddInputs->setText(mainXmlElem->getStringAttribute("Number_Of_Inputs"));
            this->sliderMasterGainOut->setValue(mainXmlElem->getDoubleAttribute("Master_Gain_Out", 1.0), sendNotification);
            this->sliderInterpolation->setValue(mainXmlElem->getDoubleAttribute("Master_Interpolation", 0.33), sendNotification);
            this->comBoxModeSpat->setSelectedItemIndex(mainXmlElem->getIntAttribute("Mode_Process"),sendNotification);
            this->setShowNumbers(mainXmlElem->getBoolAttribute("Show_Numbers"));
            this->setHighPerformance(mainXmlElem->getBoolAttribute("High_Performance"));
            if (mainXmlElem->hasAttribute("Show_Speakers")) {
                this->setShowSpeakers(mainXmlElem->getBoolAttribute("Show_Speakers"));
            } else {
                this->setShowSpeakers(true);
            }
            if (mainXmlElem->hasAttribute("Show_Triplets")) {
                this->setShowTriplets(mainXmlElem->getBoolAttribute("Show_Triplets"));
            } else {
                this->setShowTriplets(false);
            }
            if (mainXmlElem->hasAttribute("Use_Alpha")) {
                this->isSourceLevelShown = mainXmlElem->getBoolAttribute("Use_Alpha");
            } else {
                this->isSourceLevelShown = false;
            }
            if (mainXmlElem->hasAttribute("Use_Alpha")) {
                this->isSourceLevelShown = mainXmlElem->getBoolAttribute("Use_Alpha");
            } else {
                this->isSourceLevelShown = false;
            }
            if (mainXmlElem->hasAttribute("Show_Speaker_Level")) {
                this->isSpeakerLevelShown = mainXmlElem->getBoolAttribute("Show_Speaker_Level");
            } else {
                this->isSpeakerLevelShown = false;
            }
            if (mainXmlElem->hasAttribute("Show_Sphere")) {
                this->isSphereShown = mainXmlElem->getBoolAttribute("Show_Sphere");
            } else {
                this->isSphereShown = false;
            }
            this->speakerView->setShowSphere(this->isSphereShown);

            if (mainXmlElem->hasAttribute("CamAngleX")) {
                float angleX = mainXmlElem->getDoubleAttribute("CamAngleX");
                float angleY = mainXmlElem->getDoubleAttribute("CamAngleY");
                float distance = mainXmlElem->getDoubleAttribute("CamDistance");
                this->speakerView->setCamPosition(angleX, angleY, distance);
            } else {
                this->speakerView->setCamPosition(80.0f, 25.0f, 22.0f);
            }

            if (mainXmlElem->hasAttribute("Record_Format")) {
                this->jackClient->setRecordFormat(mainXmlElem->getIntAttribute("Record_Format")); // app preferences instead of project setting?
            } else {
                this->jackClient->setRecordFormat(0);
            }

            //Update----------------------------------
            this->textEditorReturnKeyPressed(*this->tedAddInputs);
            this->sliderValueChanged(this->sliderMasterGainOut);
            this->sliderValueChanged(this->sliderInterpolation);

            File speakerSetup = File(this->pathCurrentFileSpeaker.toStdString());
            if (!this->pathCurrentFileSpeaker.startsWith("/")) {
#ifdef __linux__
                String cwd = File::getCurrentWorkingDirectory().getFullPathName();
                this->pathCurrentFileSpeaker = cwd + ("/../../Resources/default_preset/") + this->pathCurrentFileSpeaker;
#else
                String cwd = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
                this->pathCurrentFileSpeaker = cwd + ("/Contents/Resources/default_preset/") + this->pathCurrentFileSpeaker;
#endif
            }

            forEachXmlChildElement (*mainXmlElem, input) {
                if (input->hasTagName ("Input")) {
                    for (auto&& it : listSourceInput) {
                        if (it->getId() == input->getIntAttribute("Index")) {
                            it->setColor(Colour::fromFloatRGBA((float)input->getDoubleAttribute("R"),
                                                               (float)input->getDoubleAttribute("G"),
                                                               (float)input->getDoubleAttribute("B"), 1.0f), true);
                            if (input->hasAttribute("DirectOut")) {
                                it->setDirectOutChannel(input->getIntAttribute("DirectOut"));
                                this->setDirectOut(it->getId(), input->getIntAttribute("DirectOut"));
                            } else {
                                it->setDirectOutChannel(0);
                                this->setDirectOut(it->getId(), 0);
                            }
                        }
                    }
                }
            }
        } else {
            if (mainXmlElem->hasTagName("SpeakerSetup")) {
                msg = "You are trying to open a Speaker Setup, and not a Server document !";
            } else {
                msg = "Your file is corrupted !\n" + xmlDoc.getLastParseError();
            }
            ScopedPointer<AlertWindow> alert = new AlertWindow ("Error in Open Preset !", 
                                                                msg, 
                                                                AlertWindow::WarningIcon);
            alert->setLookAndFeel(&mGrisFeel);
            alert->addButton ("Ok", 0);
            alert->runModalLoop();
        }
    }
    this->jackClient->noiseSound = false;
    this->jackClient->processBlockOn = true;
    if (this->pathCurrentPreset.endsWith("default_preset/default_preset.xml")) {
        this->applicationProperties.getUserSettings()->setValue("lastPresetDirectory", 
                                                                File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
    } else {
        this->applicationProperties.getUserSettings()->setValue("lastPresetDirectory", 
                                                                File(this->pathCurrentPreset).getParentDirectory().getFullPathName());
    }
    this->setTitle();
}

void MainContentComponent::savePreset(String path)
{
    File xmlFile = File (path.toStdString());
    XmlDocument xmlDoc (xmlFile);
    ScopedPointer<XmlElement>  xml = new XmlElement("ServerGRIS_Preset");
    
    xml->setAttribute("OSC_Input_Port", String(this->oscInputPort));
    xml->setAttribute("Number_Of_Inputs", this->tedAddInputs->getTextValue().toString());
    xml->setAttribute("Master_Gain_Out", this->sliderMasterGainOut->getValue());
    xml->setAttribute("Master_Interpolation", this->sliderInterpolation->getValue());
    xml->setAttribute("Mode_Process", this->comBoxModeSpat->getSelectedItemIndex());
    xml->setAttribute("Show_Numbers", this->isNumbersShown);
    xml->setAttribute("Show_Speakers", this->isSpeakersShown);
    xml->setAttribute("Show_Triplets", this->isTripletsShown);
    xml->setAttribute("High_Performance", this->isHighPerformance);
    xml->setAttribute("Use_Alpha", this->isSourceLevelShown);
    xml->setAttribute("Show_Speaker_Level", this->isSpeakerLevelShown);
    xml->setAttribute("Show_Sphere", this->isSphereShown);
    xml->setAttribute("Record_Format", this->jackClient->getRecordFormat());
    xml->setAttribute("CamAngleX", this->speakerView->getCamAngleX());
    xml->setAttribute("CamAngleY", this->speakerView->getCamAngleY());
    xml->setAttribute("CamDistance", this->speakerView->getCamDistance());
    
    for (auto&& it : listSourceInput)
    {
        XmlElement * xmlInput = new  XmlElement("Input");
        xmlInput->setAttribute("Index", it->getId());
        xmlInput->setAttribute("R", it->getColor().x);
        xmlInput->setAttribute("G", it->getColor().y);
        xmlInput->setAttribute("B", it->getColor().z);
        xmlInput->setAttribute("DirectOut", String(it->getDirectOutChannel()));
        xml->addChildElement(xmlInput);
    }
    xml->writeToFile(xmlFile, "");
    xmlFile.create();
    this->pathCurrentPreset = path;
    this->applicationProperties.getUserSettings()->setValue("lastPresetDirectory", 
                                                            File(this->pathCurrentPreset).getParentDirectory().getFullPathName());
    this->setTitle();
}

String MainContentComponent::getCurrentFileSpeakerPath() {
    return this->pathCurrentFileSpeaker;
}

void MainContentComponent::saveSpeakerSetup(String path)
{
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
        xmlInput->setAttribute("Gain", it->getGain());
        xmlInput->setAttribute("HighPassCutoff", it->getHighPassCutoff());
        xmlInput->setAttribute("DirectOut", it->getDirectOut());
        xmlRing->addChildElement(xmlInput);
    }
    xml->addChildElement(xmlRing);

    for (auto&& it : listTriplet)
    {
        XmlElement * xmlInput = new  XmlElement("triplet");
        xmlInput->setAttribute("id1", it.id1);
        xmlInput->setAttribute("id2", it.id2);
        xmlInput->setAttribute("id3", it.id3);
        xml->addChildElement(xmlInput);
    }
    
    xml->writeToFile(xmlFile,"");
    xmlFile.create();

    this->applicationProperties.getUserSettings()->setValue("lastSpeakerSetupDirectory", 
                                                            File(this->pathCurrentFileSpeaker).getParentDirectory().getFullPathName());

    this->needToSaveSpeakerSetup = false;

    this->setNameConfig();
}

void MainContentComponent::saveProperties(unsigned int rate, unsigned int buff, int fileformat, int oscPort) {

    PropertiesFile *props = this->applicationProperties.getUserSettings();

    unsigned int BufferValue = props->getIntValue("BufferValue", 1024);
    unsigned int RateValue = props->getIntValue("RateValue", 48000);
    unsigned int OscInputPort = props->getIntValue("OscInputPort", 18032);

    // ======== handle Jack settings ===============
    if (isnan(BufferValue) || BufferValue == 0) { BufferValue = 1024; }
    if (isnan(RateValue) || RateValue == 0) { RateValue = 48000; }

    if(rate != RateValue || buff != BufferValue) {
        ScopedPointer<AlertWindow> alert = new AlertWindow ("Restart ServerGRIS",
                                                            "Need to restart ServerGRIS to apply new settings !", 
                                                            AlertWindow::InfoIcon);
        alert->setLookAndFeel(&mGrisFeel);
        alert->addButton ("Cancel", 0);
        alert->addButton ("Ok", 1);
        int status = alert->runModalLoop();
        //Click OK -> Open xml
        if (status) {
            props->setValue("BufferValue", (int)buff);
            props->setValue("RateValue", (int)rate);

            /* FIXME: This does not work under linux (not sure about OSX). It should be possible to just shutdown
                      and restart the Jack server instead of the application, as in qjackctl.
            */
            //Restart APP
            String applicationPath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
            String relaunchCommand = "open " + applicationPath;
            ScopedPointer<ChildProcess> scriptProcess = new ChildProcess();            
            JUCEApplication::getInstance()->systemRequestedQuit();
            scriptProcess->start(relaunchCommand, (!ChildProcess::wantStdErr | !ChildProcess::wantStdOut));
        }
    }

    // ======== handle OSC Input Port ===============
    if (oscPort < 0 || oscPort > 65535) { oscPort = 18032; }
    if (oscPort != OscInputPort) {
        this->oscInputPort = oscPort;
        props->setValue("OscInputPort", oscPort);
        this->oscReceiver->closeConnection();
        if (this->oscReceiver->startConnection(this->oscInputPort)) {
            cout << "OSC receiver connected to port " << oscPort << endl;
        } else {
            cout << "OSC receiver connection to port " << oscPort << " failed... Should popup an alert window." << endl;
        }
    }

    // ======== handle recording settings ===============
    this->jackClient->setRecordFormat(fileformat);
    props->setValue("FileFormat", fileformat);

    applicationProperties.saveIfNeeded();
}

void MainContentComponent::timerCallback()
{
    this->labelJackLoad->setText(String(this->jackClient->getCpuUsed(),4)+ " %", dontSendNotification);
    int seconds = this->jackClient->indexRecord/this->jackClient->sampleRate;
    int minute = (int(seconds / 60)%60);
    seconds = int(seconds%60);
    String timeRecorded =   ((minute < 10) ? "0"+String(minute) : String(minute)) +" : "+
                            ((seconds < 10) ? "0"+String(seconds) : String(seconds));///(this->jackClient->sampleRate* this->jackClient->bufferSize);
    this->labelTimeRecorded->setText(timeRecorded, dontSendNotification);
    
    if (this->butStartRecord->getToggleState()) {
        this->butStartRecord->setToggleState(false, dontSendNotification);
    }
    
    if (this->jackClient->isSavingRun()) {
        this->butStartRecord->setButtonText("Stop");
        this->butStartRecord->setEnabled(true);
    } else if (this->jackClient->getRecordingPath() == "") {
        this->butStartRecord->setButtonText("Record");
        this->butStartRecord->setEnabled(false);
    } else {
        this->butStartRecord->setButtonText("Record");
        this->butStartRecord->setEnabled(true);
    } 
    
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

    if (this->isProcessForeground != Process::isForegroundProcess()) {
        this->isProcessForeground = Process::isForegroundProcess();
        if (this->winSpeakConfig != nullptr && this->isProcessForeground) {
            this->winSpeakConfig->setVisible(true);
            this->winSpeakConfig->setAlwaysOnTop(true);
        }
        else if (this->winSpeakConfig != nullptr && !this->isProcessForeground) {
            this->winSpeakConfig->setVisible(false);
            this->winSpeakConfig->setAlwaysOnTop(false);
        }
    }
}

void MainContentComponent::paint (Graphics& g)
{
    g.fillAll (mGrisFeel.getWinBackgroundColour());
}


void MainContentComponent::textEditorFocusLost (TextEditor &textEditor)
{
    textEditorReturnKeyPressed(textEditor);
}

void MainContentComponent::textEditorReturnKeyPressed (TextEditor & textEditor)
{
    if (&textEditor == this->tedAddInputs) {
        if (this->tedAddInputs->getTextValue().toString().getIntValue() < 1) {
            this->tedAddInputs->setText("1");
        }
        if (this->tedAddInputs->getTextValue().toString().getIntValue() > MaxInputs) {
            this->tedAddInputs->setText(String(MaxInputs));
        }

        if (this->jackClient->inputsPort.size() != this->tedAddInputs->getTextValue().toString().getIntValue()) {
            this->jackClient->addRemoveInput(this->tedAddInputs->getTextValue().toString().getIntValue());
            
            this->lockInputs->lock();
            bool addInput = false;
            for (unsigned int i = 0 ; i < this->jackClient->inputsPort.size(); i++) {
                if (i >= this->listSourceInput.size()) {
                    this->listSourceInput.push_back(new Input(this, &mSmallTextGrisFeel,i+1));
                    addInput = true;
                }
            }
            if (!addInput) {
                for (auto it = this->listSourceInput.begin()+(this->jackClient->inputsPort.size()) ; it != this->listSourceInput.end();) {
                    delete *it;
                    it = this->listSourceInput.erase(it);
                }
            }
            this->lockInputs->unlock();
        }
        this->unfocusAllComponents();
        updateLevelComp();
    }
}

void MainContentComponent::buttonClicked (Button *button)
{
    if(button == this->butStartRecord){
        //Record sound
        if (this->jackClient->recording) {
            this->jackClient->stopRecord();
            this->labelTimeRecorded->setColour(Label::textColourId, mGrisFeel.getFontColour());
        } else {
            this->jackClient->startRecord();
            this->labelTimeRecorded->setColour(Label::textColourId, mGrisFeel.getRedColour());
        }
        this->butStartRecord->setToggleState(this->jackClient->recording, dontSendNotification);
    }
    else if (button == this->butInitRecord) {
        this->chooseRecordingPath();
        this->butStartRecord->setEnabled(true);
    }
    //}else if(button == this->butAutoConnectJack){
    //    
    //    this->jackClient->autoConnectClient();
        
    //}else if(button == this->butDisconnectAllJack){
    //    
    //    this->jackClient->disconnectAllClient();
    //}
}

void MainContentComponent::sliderValueChanged (Slider* slider)
{
    if(slider == this->sliderMasterGainOut){
        this->jackClient->masterGainOut = this->sliderMasterGainOut->getValue();
    }
    
    else if(slider == this->sliderInterpolation){
        this->jackClient->interMaster = this->sliderInterpolation->getValue();
    }
}

void MainContentComponent::comboBoxChanged (ComboBox *comboBox)
{
    int ret;
    if(this->comBoxModeSpat == comboBox){
        this->jackClient->modeSelected = (ModeSpatEnum)(this->comBoxModeSpat->getSelectedId()-1);
        
        switch (this->jackClient->modeSelected) {
            case VBap:
                if (this->pathLastVbapSpeakerSetup != this->pathCurrentFileSpeaker) {
                    this->openXmlFileSpeaker(this->pathLastVbapSpeakerSetup);
                    ret = 1;
                } else {
                    ret = this->updateLevelComp();
                }
                if (ret) {
                    this->labelModeInfo->setText("Ready", dontSendNotification);
                    this->labelModeInfo->setColour(Label::textColourId, mGrisFeel.getGreenColour());
                    this->isSpanShown = true;
                    this->sliderInterpolation->setEnabled(true);
                } else {
                    this->labelModeInfo->setText("ERROR", dontSendNotification);
                    this->labelModeInfo->setColour(Label::textColourId, mGrisFeel.getRedColour());
                }
                break;
            case DBap:
                this->labelModeInfo->setText("Not ready yet", dontSendNotification);
                this->labelModeInfo->setColour(Label::textColourId, mGrisFeel.getRedColour());
                break;
            case VBap_HRTF:
                this->loadVbapHrtfSpeakerSetup();
                this->jackClient->resetHRTF();
                if (this->updateLevelComp()) {
                    this->labelModeInfo->setText("Ready", dontSendNotification);
                    this->labelModeInfo->setColour(Label::textColourId, mGrisFeel.getGreenColour());
                    this->isSpanShown = false;
                    this->sliderInterpolation->setEnabled(true);
                } else {
                    this->labelModeInfo->setText("ERROR", dontSendNotification);
                    this->labelModeInfo->setColour(Label::textColourId, mGrisFeel.getRedColour());
                }
                break;
            case STEREO:
                this->labelModeInfo->setText("Ready", dontSendNotification);
                this->labelModeInfo->setColour(Label::textColourId, mGrisFeel.getGreenColour());
                this->sliderInterpolation->setEnabled(true);
                this->isSpanShown = false;
                break;
            default:
                this->labelModeInfo->setText("ERROR UNK", dontSendNotification);
                this->labelModeInfo->setColour(Label::textColourId, mGrisFeel.getRedColour());
                break;
        }        
    }
}

void MainContentComponent::chooseRecordingPath() {
    String dir = this->applicationProperties.getUserSettings()->getValue("lastRecordingDirectory");
    if (! File(dir).isDirectory()) {
        dir = File("~").getFullPathName();
    }
    String extF;
    String extChoice;
    if (this->jackClient->getRecordFormat() == 0) {
        extF = ".wav";
        extChoice = "*.wav,*.aif";
    } else {
        extF = ".aif";
        extChoice = "*.aif,*.wav";
    }

#ifdef __linux__
    FileChooser fc ("Choose a file to save...", dir + "/recording" + extF, extChoice, false);
#else
    FileChooser fc ("Choose a file to save...", dir + "/recording" + extF, extChoice, true);
#endif

    if (fc.browseForFileToSave (true)) {
        String filePath = fc.getResults().getReference(0).getFullPathName();
        this->applicationProperties.getUserSettings()->setValue("lastRecordingDirectory", 
                                                                File(filePath).getParentDirectory().getFullPathName());
        this->jackClient->setRecordingPath(filePath);
    }
    this->jackClient->prepareToRecord();
}

void MainContentComponent::resized()
{
    Rectangle<int> r (getLocalBounds().reduced (2));

    menuBar->setBounds(0, 0, this->getWidth(), 20);
    r.removeFromTop (20);
    
    // lay out the list box and vertical divider..
    Component* vcomps[] = { this->speakerView, this->verticalDividerBar, nullptr };
    
    // lay out side-by-side and resize the components' heights as well as widths
    this->verticalLayout.layOutComponents (vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);
    
    this->boxMainUI->setBounds(this->speakerView->getWidth()+6, 20, getWidth()-(this->speakerView->getWidth()+10), getHeight());
    this->boxMainUI->correctSize(getWidth()-this->speakerView->getWidth()-6, 610);

    this->boxInputsUI->setBounds(0, 2, getWidth()-(this->speakerView->getWidth()+10), 231);
    this->boxInputsUI->correctSize(((unsigned int )this->listSourceInput.size()*(SizeWidthLevelComp))+4, 200);

    this->boxOutputsUI->setBounds(0, 233, getWidth()-(this->speakerView->getWidth()+10), 210);
    this->boxOutputsUI->correctSize(((unsigned int )this->listSpeaker.size()*(SizeWidthLevelComp))+4, 180);
    
    this->boxControlUI->setBounds(0, 443, getWidth()-(this->speakerView->getWidth()+10), 145);
    this->boxControlUI->correctSize(720, 145);

}

