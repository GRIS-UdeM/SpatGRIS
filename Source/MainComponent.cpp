/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MainComponent.h"

#include "AudioRenderer.h"
#include "LevelComponent.h"
#include "MainWindow.h"

//==============================================================================
MainContentComponent::MainContentComponent(MainWindow & mainWindow, GrisLookAndFeel & newLookAndFeel)
    : mLookAndFeel(newLookAndFeel)
    , mMainWindow(mainWindow)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&newLookAndFeel);

    // Create the menubar.
    mMenuBar.reset(new juce::MenuBarComponent(this));
    addAndMakeVisible(mMenuBar.get());

    // Start the Splash screen.
    if (SPLASH_SCREEN_FILE.exists()) {
        mSplashScreen.reset(
            new juce::SplashScreen("SpatGRIS2", juce::ImageFileFormat::loadFrom(SPLASH_SCREEN_FILE), true));
    }

    // App user settings storage file.
    juce::PropertiesFile::Options options;
    options.applicationName = "SpatGRIS2";
    options.commonToAllUsers = false;
    options.filenameSuffix = "xml";
    options.folderName = "GRIS";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.ignoreCaseOfKeyNames = true;
    options.osxLibrarySubFolder = "Application Support";
    mApplicationProperties.setStorageParameters(options);

    juce::PropertiesFile * props = mApplicationProperties.getUserSettings();

    // Initialize class variables.
    mIsProcessForeground = true;
    mIsNumbersShown = false;
    mIsSpeakersShown = true;
    mIsTripletsShown = false;
    mIsSourceLevelShown = false;
    mIsSphereShown = false;
    mIsSpanShown = true;
    mIsRecording = false;

    // Get a reference to the last opened VBAP speaker setup.
    juce::File lastVbap = juce::File(props->getValue("lastVbapSpeakerSetup", "./not_saved_yet"));
    if (!lastVbap.existsAsFile()) {
        mPathLastVbapSpeakerSetup = DEFAULT_SPEAKER_SETUP_FILE.getFullPathName();
    } else {
        mPathLastVbapSpeakerSetup = props->getValue("lastVbapSpeakerSetup");
    }

    mEditSpeakersWindow = nullptr;
    mPropertiesWindow = nullptr;
    mFlatViewWindow = nullptr;
    mAboutWindow = nullptr;
    mOscLogWindow = nullptr;

    // SpeakerViewComponent 3D view
    mSpeakerViewComponent.reset(new SpeakerViewComponent(*this));
    addAndMakeVisible(mSpeakerViewComponent.get());

    // Box Main
    mMainUiBox.reset(new Box(mLookAndFeel, "", true, false));
    addAndMakeVisible(mMainUiBox.get());

    // Box Inputs
    mInputsUiBox.reset(new Box(mLookAndFeel, "Inputs"));
    addAndMakeVisible(mInputsUiBox.get());

    // Box Outputs
    mOutputsUiBox.reset(new Box(mLookAndFeel, "Outputs"));
    addAndMakeVisible(mOutputsUiBox.get());

    // Box Control
    mControlUiBox.reset(new Box(mLookAndFeel));
    addAndMakeVisible(mControlUiBox.get());

    mMainUiBox->getContent()->addAndMakeVisible(mInputsUiBox.get());
    mMainUiBox->getContent()->addAndMakeVisible(mOutputsUiBox.get());
    mMainUiBox->getContent()->addAndMakeVisible(mControlUiBox.get());

    // Components in Box Control
    mJackStatusLabel.reset(addLabel("Jack Unknown", "Jack Status", 0, 0, 80, 28, mControlUiBox->getContent()));
    mJackLoadLabel.reset(addLabel("0.000000 %", "Load Jack CPU", 80, 0, 80, 28, mControlUiBox->getContent()));
    mJackRateLabel.reset(addLabel("00000 Hz", "Rate", 160, 0, 80, 28, mControlUiBox->getContent()));
    mJackBufferLabel.reset(addLabel("0000 spls", "Buffer Size", 240, 0, 80, 28, mControlUiBox->getContent()));
    mJackInfoLabel.reset(addLabel("...", "Jack Inputs/Outputs system", 320, 0, 90, 28, mControlUiBox->getContent()));

    mJackStatusLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mJackLoadLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mJackRateLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mJackBufferLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mJackInfoLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());

    addLabel("Gain", "Master Gain Outputs", 15, 30, 120, 20, mControlUiBox->getContent());
    mMasterGainOutSlider.reset(
        addSlider("Master Gain", "Master Gain Outputs", 5, 45, 60, 60, mControlUiBox->getContent()));
    mMasterGainOutSlider->setRange(-60.0, 12.0, 0.01);
    mMasterGainOutSlider->setTextValueSuffix(" dB");

    addLabel("Interpolation", "Master Interpolation", 60, 30, 120, 20, mControlUiBox->getContent());
    mInterpolationSlider.reset(addSlider("Inter", "Interpolation", 70, 45, 60, 60, mControlUiBox->getContent()));
    mInterpolationSlider->setRange(0.0, 1.0, 0.001);

    addLabel("Mode :", "Mode of spatilization", 150, 30, 60, 20, mControlUiBox->getContent());
    mModeSpatCombo.reset(addComboBox("", "Mode of spatilization", 155, 48, 90, 22, mControlUiBox->getContent()));
    for (int i = 0; i < MODE_SPAT_STRING.size(); i++) {
        mModeSpatCombo->addItem(MODE_SPAT_STRING[i], i + 1);
    }

    mAddInputsTextEditor.reset(
        addTextEditor("Inputs :", "0", "Numbers of Inputs", 122, 83, 43, 22, mControlUiBox->getContent()));
    mAddInputsTextEditor->setInputRestrictions(3, "0123456789");

    mInitRecordButton.reset(
        addButton("Init Recording", "Init Recording", 268, 48, 103, 24, mControlUiBox->getContent()));

    mStartRecordButton.reset(addButton("Record", "Start/Stop Record", 268, 83, 60, 24, mControlUiBox->getContent()));
    mStartRecordButton->setEnabled(false);

    mTimeRecordedLabel.reset(addLabel("00:00", "Record time", 327, 83, 50, 24, mControlUiBox->getContent()));

    // Jack client box.
    mJackClientListComponent.reset(new JackClientListComponent(this, &mLookAndFeel));
    mJackClientListComponent->setBounds(410, 0, 304, 138);
    mControlUiBox->getContent()->addAndMakeVisible(mJackClientListComponent.get());

    // Set up the layout and resizer bars.
    mVerticalLayout.setItemLayout(0,
                                  -0.2,
                                  -0.8,
                                  -0.435);     // width of the speaker view must be between 20% and 80%, preferably 50%
    mVerticalLayout.setItemLayout(1, 8, 8, 8); // the vertical divider drag-bar thing is always 8 pixels wide
    mVerticalLayout.setItemLayout(
        2,
        150,
        -1.0,
        -0.565); // right panes must be at least 150 pixels wide, preferably 50% of the total width
    mVerticalDividerBar.reset(new juce::StretchableLayoutResizerBar(&mVerticalLayout, 1, true));
    addAndMakeVisible(mVerticalDividerBar.get());

    // Default application window size.
    setSize(1285, 610);

    // Jack Initialization parameters.
    unsigned int BufferValue = props->getIntValue("BufferValue", 1024);
    unsigned int RateValue = props->getIntValue("RateValue", 48000);

    if (std::isnan(float(BufferValue)) || BufferValue == 0 || std::isnan(float(RateValue)) || RateValue == 0) {
        BufferValue = 1024;
        RateValue = 48000;
    }

    mSamplingRate = RateValue;

    // Start Jack Server and client.
    int errorCode = 0;
    mAlsaOutputDevice = props->getValue("AlsaOutputDevice", "");
    jackServer.reset(new JackServerGris(RateValue, BufferValue, mAlsaOutputDevice, &errorCode));
    if (errorCode > 0) {
        juce::String msg;
        if (errorCode == 1) {
            msg = "Failed to create Jack server...";
        } else if (errorCode == 2) {
            msg = "Failed to open Jack server...";
        } else if (errorCode == 3) {
            msg = "Failed to start Jack server...";
        }
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Jack Server Failure",
            msg
                + juce::String("\nYou should check for any mismatch between the server and your "
                               "device\n(Sampling Rate, Input/Ouput Channels, etc.)"));
    }
    jackClient.reset(new JackClientGris());

    mAlsaAvailableOutputDevices = jackServer->getAvailableOutputDevices();

    unsigned int fileformat = props->getIntValue("FileFormat", 0);
    jackClient->setRecordFormat(fileformat);
    unsigned int fileconfig = props->getIntValue("FileConfig", 0);
    jackClient->setRecordFileConfig(fileconfig);

    if (!jackClient->isReady()) {
        mJackStatusLabel->setText("Jack ERROR", juce::dontSendNotification);
    } else {
        mJackStatusLabel->setText("Jack Run", juce::dontSendNotification);
    }

    mJackRateLabel->setText(juce::String(jackClient->getSampleRate()) + " Hz", juce::dontSendNotification);
    mJackBufferLabel->setText(juce::String(jackClient->getBufferSize()) + " spls", juce::dontSendNotification);
    mJackInfoLabel->setText("I : " + juce::String(jackClient->getNumberOutputs())
                                + " - O : " + juce::String(jackClient->getNumberInputs()),
                            juce::dontSendNotification);

    // Start the OSC Receiver.
    mOscReceiver.reset(new OscInput(*this));
    mOscReceiver->startConnection(mOscInputPort);

    // Default widget values.
    mMasterGainOutSlider->setValue(0.0);
    mInterpolationSlider->setValue(0.1);
    mModeSpatCombo->setSelectedId(1);

    mAddInputsTextEditor->setText("16", juce::dontSendNotification);
    textEditorReturnKeyPressed(*mAddInputsTextEditor);

    // Open the default preset if lastOpenPreset is not a valid file.
    juce::File preset = juce::File(props->getValue("lastOpenPreset", "./not_saved_yet"));
    if (!preset.existsAsFile()) {
        openPreset(DEFAULT_PRESET_FILE.getFullPathName());
    } else {
        openPreset(props->getValue("lastOpenPreset"));
    }

    // Open the default speaker setup if lastOpenSpeakerSetup is not a valid file.
    juce::File setup = juce::File(props->getValue("lastOpenSpeakerSetup", "./not_saved_yet"));
    if (!setup.existsAsFile()) {
        openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
    } else {
        openXmlFileSpeaker(props->getValue("lastOpenSpeakerSetup"));
    }

    // End layout and start refresh timer.
    resized();
    startTimerHz(24);

    // End Splash screen.
    if (mSplashScreen) {
        mSplashScreen->deleteAfterDelay(juce::RelativeTime::seconds(4), false);
        mSplashScreen.release();
    }

    // Initialize the command manager for the menubar items.
    juce::ApplicationCommandManager & commandManager = mMainWindow.getApplicationCommandManager();
    commandManager.registerAllCommandsForTarget(this);

    // Restore last vertical divider position and speaker view cam distance.
    if (props->containsKey("sashPosition")) {
        int trueSize = (int)round((getWidth() - 3) * abs(props->getDoubleValue("sashPosition")));
        mVerticalLayout.setItemPosition(1, trueSize);
    }
}

//==============================================================================
MainContentComponent::~MainContentComponent()
{
    juce::PropertiesFile * props = mApplicationProperties.getUserSettings();
    props->setValue("lastOpenPreset", mPathCurrentPreset);
    props->setValue("lastOpenSpeakerSetup", mPathCurrentFileSpeaker);
    props->setValue("lastVbapSpeakerSetup", mPathLastVbapSpeakerSetup);
    props->setValue("sashPosition", mVerticalLayout.getItemCurrentRelativeSize(0));
    mApplicationProperties.saveIfNeeded();
    mApplicationProperties.closeFiles();

    mSpeakerLocks.lock();
    mSpeakers.clear();
    mSpeakerLocks.unlock();

    mInputLocks.lock();
    mSourceInputs.clear();
    mInputLocks.unlock();
}

//==============================================================================
// Widget builder utilities.
juce::Label * MainContentComponent::addLabel(const juce::String & s,
                                             const juce::String & stooltip,
                                             int x,
                                             int y,
                                             int w,
                                             int h,
                                             Component * into)
{
    juce::Label * lb = new juce::Label();
    lb->setText(s, juce::NotificationType::dontSendNotification);
    lb->setTooltip(stooltip);
    lb->setJustificationType(juce::Justification::left);
    lb->setFont(mLookAndFeel.getFont());
    lb->setLookAndFeel(&mLookAndFeel);
    lb->setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
    lb->setBounds(x, y, w, h);
    into->addAndMakeVisible(lb);
    return lb;
}

//==============================================================================
juce::TextButton * MainContentComponent::addButton(const juce::String & s,
                                                   const juce::String & stooltip,
                                                   int x,
                                                   int y,
                                                   int w,
                                                   int h,
                                                   Component * into)
{
    juce::TextButton * tb = new juce::TextButton();
    tb->setTooltip(stooltip);
    tb->setButtonText(s);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    tb->setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    tb->setLookAndFeel(&mLookAndFeel);
    into->addAndMakeVisible(tb);
    return tb;
}

//==============================================================================
juce::ToggleButton * MainContentComponent::addToggleButton(const juce::String & s,
                                                           const juce::String & stooltip,
                                                           int x,
                                                           int y,
                                                           int w,
                                                           int h,
                                                           Component * into,
                                                           bool toggle)
{
    juce::ToggleButton * tb = new juce::ToggleButton();
    tb->setTooltip(stooltip);
    tb->setButtonText(s);
    tb->setToggleState(toggle, juce::dontSendNotification);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    tb->setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    tb->setLookAndFeel(&mLookAndFeel);
    into->addAndMakeVisible(tb);
    return tb;
}

//==============================================================================
juce::TextEditor * MainContentComponent::addTextEditor(const juce::String & s,
                                                       const juce::String & emptyS,
                                                       const juce::String & stooltip,
                                                       int x,
                                                       int y,
                                                       int w,
                                                       int h,
                                                       Component * into,
                                                       int wLab)
{
    juce::TextEditor * te = new juce::TextEditor();
    te->setTooltip(stooltip);
    te->setTextToShowWhenEmpty(emptyS, mLookAndFeel.getOffColour());
    te->setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    te->setLookAndFeel(&mLookAndFeel);

    if (s.isEmpty()) {
        te->setBounds(x, y, w, h);
    } else {
        te->setBounds(x + wLab, y, w, h);
        juce::Label * lb = addLabel(s, "", x, y, wLab, h, into);
        lb->setJustificationType(juce::Justification::centredRight);
    }

    te->addListener(this);
    into->addAndMakeVisible(te);
    return te;
}

//==============================================================================
juce::Slider * MainContentComponent::addSlider(const juce::String & s,
                                               const juce::String & stooltip,
                                               int x,
                                               int y,
                                               int w,
                                               int h,
                                               Component * into)
{
    juce::Slider * sd = new juce::Slider();
    sd->setTooltip(stooltip);
    sd->setSize(w, h);
    sd->setTopLeftPosition(x, y);
    sd->setSliderStyle(juce::Slider::Rotary);
    sd->setRotaryParameters(juce::MathConstants<float>::pi * 1.3f, juce::MathConstants<float>::pi * 2.7f, true);
    sd->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    sd->setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    sd->setLookAndFeel(&mLookAndFeel);
    sd->addListener(this);
    into->addAndMakeVisible(sd);
    return sd;
}

//==============================================================================
juce::ComboBox * MainContentComponent::addComboBox(const juce::String & s,
                                                   const juce::String & stooltip,
                                                   int const x,
                                                   int const y,
                                                   int const w,
                                                   int const h,
                                                   Component * into)
{
    // TODO : naked new
    juce::ComboBox * cb = new juce::ComboBox();
    cb->setTooltip(stooltip);
    cb->setSize(w, h);
    cb->setTopLeftPosition(x, y);
    cb->setLookAndFeel(&mLookAndFeel);
    cb->addListener(this);
    into->addAndMakeVisible(cb);
    return cb;
}

//==============================================================================
// Menu item action handlers.
void MainContentComponent::handleNew()
{
    juce::AlertWindow alert("Closing current preset !", "Do you want to save ?", juce::AlertWindow::InfoIcon);
    alert.setLookAndFeel(&mLookAndFeel);
    alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::deleteKey));
    alert.addButton("yes", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert.addButton("No", 2, juce::KeyPress(juce::KeyPress::escapeKey));

    int status = alert.runModalLoop();
    if (status == 1) {
        handleSavePreset();
    } else if (status == 0) {
        return;
    }

    openPreset(DEFAULT_PRESET_FILE.getFullPathName());
}

//==============================================================================
void MainContentComponent::handleOpenPreset()
{
    juce::String dir = mApplicationProperties.getUserSettings()->getValue("lastPresetDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(mPathCurrentPreset).getFileName();

    juce::FileChooser fc("Choose a file to open...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    bool loaded = false;
    if (fc.browseForFileToOpen()) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        juce::AlertWindow alert("Open Project !",
                                "You want to load : " + chosen + "\nEverything not saved will be lost !",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert.addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            openPreset(chosen);
            loaded = true;
        }
    }

    if (loaded) { // Check for direct out OutputPatch mismatch.
        for (auto && it : mSourceInputs) {
            if (it->getDirectOutChannel() != 0) {
                std::vector<int> directOutOutputPatches = jackClient->getDirectOutOutputPatches();
                if (std::find(directOutOutputPatches.begin(), directOutOutputPatches.end(), it->getDirectOutChannel())
                    == directOutOutputPatches.end()) {
                    juce::AlertWindow alert(
                        "Direct Out Mismatch!",
                        "Some of the direct out channels of this project don't exist in the current speaker setup.\n",
                        juce::AlertWindow::WarningIcon);
                    alert.setLookAndFeel(&mLookAndFeel);
                    alert.addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
                    alert.runModalLoop();
                    break;
                }
            }
        }
    }
}

//==============================================================================
void MainContentComponent::handleSavePreset()
{
    if (!juce::File(mPathCurrentPreset).existsAsFile()
        || mPathCurrentPreset.endsWith("default_preset/default_preset.xml")) {
        handleSaveAsPreset();
    }
    savePreset(mPathCurrentPreset);
}

//==============================================================================
void MainContentComponent::handleSaveAsPreset()
{
    juce::String dir = mApplicationProperties.getUserSettings()->getValue("lastPresetDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(mPathCurrentPreset).getFileName();

    juce::FileChooser fc("Choose a file to save...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToSave(true)) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        savePreset(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleOpenSpeakerSetup()
{
    juce::String dir = mApplicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(mPathCurrentFileSpeaker).getFileName();

    juce::FileChooser fc("Choose a file to open...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToOpen()) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        juce::AlertWindow alert("Load Speaker Setup !",
                                "You want to load : " + chosen + "\nEverything not saved will be lost !",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert.addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            alert.setVisible(false);
            openXmlFileSpeaker(chosen);
        }
    }
}

//==============================================================================
void MainContentComponent::handleSaveAsSpeakerSetup()
{
    juce::String dir = mApplicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory");
    if (!juce::File(dir).isDirectory() || dir.endsWith("/default_preset")) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(mPathCurrentFileSpeaker).getFileName();

    juce::FileChooser fc("Choose a file to save...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToSave(true)) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        saveSpeakerSetup(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleShowSpeakerEditWindow()
{
    juce::Rectangle<int> result(getScreenX() + mSpeakerViewComponent->getWidth() + 20, getScreenY() + 20, 850, 600);
    if (mEditSpeakersWindow == nullptr) {
        juce::String windowName = juce::String("Speakers Setup Edition - ")
                                  + juce::String(MODE_SPAT_STRING[jackClient->getMode()]) + juce::String(" - ")
                                  + juce::File(mPathCurrentFileSpeaker).getFileName();
        mEditSpeakersWindow.reset(new EditSpeakersWindow(windowName, mLookAndFeel, *this, mNameConfig));
        mEditSpeakersWindow->setBounds(result);
        mEditSpeakersWindow->initComp();
    }
    mEditSpeakersWindow->setBounds(result);
    mEditSpeakersWindow->setResizable(true, true);
    mEditSpeakersWindow->setUsingNativeTitleBar(true);
    mEditSpeakersWindow->setVisible(true);
    mEditSpeakersWindow->setAlwaysOnTop(true);
    mEditSpeakersWindow->repaint();
}

//==============================================================================
void MainContentComponent::handleShowPreferences()
{
    juce::PropertiesFile * props = mApplicationProperties.getUserSettings();
    if (mPropertiesWindow == nullptr) {
        unsigned int BufferValue = props->getIntValue("BufferValue", 1024);
        unsigned int RateValue = props->getIntValue("RateValue", 48000);
        unsigned int FileFormat = props->getIntValue("FileFormat", 0);
        unsigned int FileConfig = props->getIntValue("FileConfig", 0);
        unsigned int AttenuationDB = props->getIntValue("AttenuationDB", 3);
        unsigned int AttenuationHz = props->getIntValue("AttenuationHz", 3);
        unsigned int OscInputPort = props->getIntValue("OscInputPort", 18032);
        if (std::isnan(float(BufferValue)) || BufferValue == 0) {
            BufferValue = 1024;
        }
        if (std::isnan(float(RateValue)) || RateValue == 0) {
            RateValue = 48000;
        }
        if (std::isnan(float(FileFormat))) {
            FileFormat = 0;
        }
        if (std::isnan(float(FileConfig))) {
            FileConfig = 0;
        }
        if (std::isnan(float(AttenuationDB))) {
            AttenuationDB = 3;
        }
        if (std::isnan(float(AttenuationHz))) {
            AttenuationHz = 3;
        }
        if (std::isnan(float(OscInputPort))) {
            OscInputPort = 18032;
        }
        mPropertiesWindow.reset(new PropertiesWindow{ *this,
                                                      mLookAndFeel,
                                                      mAlsaAvailableOutputDevices,
                                                      mAlsaOutputDevice,
                                                      RATE_VALUES.indexOf(juce::String(RateValue)),
                                                      BUFFER_SIZES.indexOf(juce::String(BufferValue)),
                                                      static_cast<int>(FileFormat),
                                                      static_cast<int>(FileConfig),
                                                      static_cast<int>(AttenuationDB),
                                                      static_cast<int>(AttenuationHz),
                                                      static_cast<int>(OscInputPort) });
    }
    int height = 450;
    if (mAlsaAvailableOutputDevices.isEmpty()) {
        height = 420;
    }
    juce::Rectangle<int> result(getScreenX() + (mSpeakerViewComponent->getWidth() / 2) - 150,
                                getScreenY() + (mSpeakerViewComponent->getHeight() / 2) - 75,
                                270,
                                height);
    mPropertiesWindow->setBounds(result);
    mPropertiesWindow->setResizable(false, false);
    mPropertiesWindow->setUsingNativeTitleBar(true);
    mPropertiesWindow->setVisible(true);
    mPropertiesWindow->repaint();
}

//==============================================================================
void MainContentComponent::handleShow2DView()
{
    if (mFlatViewWindow == nullptr) {
        mFlatViewWindow.reset(new FlatViewWindow{ *this, mLookAndFeel });
    } else {
        mFlatViewWindowRect.setBounds(mFlatViewWindow->getScreenX(),
                                      mFlatViewWindow->getScreenY(),
                                      mFlatViewWindow->getWidth(),
                                      mFlatViewWindow->getHeight());
    }

    if (mFlatViewWindowRect.getWidth() == 0) {
        mFlatViewWindowRect.setBounds(getScreenX() + mSpeakerViewComponent->getWidth() + 22,
                                      getScreenY() + 100,
                                      500,
                                      500);
    }

    mFlatViewWindow->setBounds(mFlatViewWindowRect);
    mFlatViewWindow->setResizable(true, true);
    mFlatViewWindow->setUsingNativeTitleBar(true);
    mFlatViewWindow->setVisible(true);
}

//==============================================================================
void MainContentComponent::handleShowOscLogView()
{
    if (mOscLogWindow == nullptr) {
        mOscLogWindow.reset(new OscLogWindow("OSC Logging Windows",
                                             mLookAndFeel.getWinBackgroundColour(),
                                             juce::DocumentWindow::allButtons,
                                             this,
                                             &mLookAndFeel));
    }
    mOscLogWindow->centreWithSize(500, 500);
    mOscLogWindow->setResizable(false, false);
    mOscLogWindow->setUsingNativeTitleBar(true);
    mOscLogWindow->setVisible(true);
    mOscLogWindow->repaint();
}

//==============================================================================
void MainContentComponent::handleShowAbout()
{
    if (mAboutWindow.get() == nullptr) {
        mAboutWindow.reset(new AboutWindow{ "About SpatGRIS", mLookAndFeel, *this });
    }
}

//==============================================================================
void MainContentComponent::handleOpenManual()
{
    juce::File fs = juce::File(SERVER_GRIS_MANUAL_FILE);
    if (fs.exists()) {
        juce::Process::openDocument("file:" + fs.getFullPathName(), juce::String());
    }
}

//==============================================================================
void MainContentComponent::handleShowNumbers()
{
    setShowNumbers(!mIsNumbersShown);
}

//==============================================================================
void MainContentComponent::setShowNumbers(bool state)
{
    mIsNumbersShown = state;
    mSpeakerViewComponent->setShowNumber(state);
}

//==============================================================================
void MainContentComponent::handleShowSpeakers()
{
    setShowSpeakers(!mIsSpeakersShown);
}

//==============================================================================
void MainContentComponent::setShowSpeakers(bool state)
{
    mIsSpeakersShown = state;
    mSpeakerViewComponent->setHideSpeaker(!state);
}

//==============================================================================
void MainContentComponent::handleShowTriplets()
{
    setShowTriplets(!mIsTripletsShown);
}

//==============================================================================
void MainContentComponent::setShowTriplets(bool state)
{
    if (getModeSelected() == LBAP && state == true) {
        juce::AlertWindow alert("Can't draw triplets !",
                                "Triplets are not effective with the CUBE mode.",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Close", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        setShowTriplets(false);
    } else if (validateShowTriplets() || state == false) {
        mIsTripletsShown = state;
        mSpeakerViewComponent->setShowTriplets(state);
    } else {
        juce::AlertWindow alert("Can't draw all triplets !",
                                "Maybe you didn't compute your current speaker setup ?",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Close", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        setShowTriplets(false);
    }
}

//==============================================================================
bool MainContentComponent::validateShowTriplets() const
{
    int success = true;
    for (unsigned int i = 0; i < getListTriplet().size(); ++i) {
        Speaker const * spk1 = getSpeakerFromOutputPatch(getListTriplet()[i].id1);
        Speaker const * spk2 = getSpeakerFromOutputPatch(getListTriplet()[i].id2);
        Speaker const * spk3 = getSpeakerFromOutputPatch(getListTriplet()[i].id3);

        if (spk1 == nullptr || spk2 == nullptr || spk3 == nullptr) {
            success = false;
            break;
        }
    }
    return success;
}

//==============================================================================
void MainContentComponent::handleShowSourceLevel()
{
    mIsSourceLevelShown = !mIsSourceLevelShown;
}

//==============================================================================
void MainContentComponent::handleShowSpeakerLevel()
{
    mIsSpeakerLevelShown = !mIsSpeakerLevelShown;
}

//==============================================================================
void MainContentComponent::handleShowSphere()
{
    mIsSphereShown = !mIsSphereShown;
    mSpeakerViewComponent->setShowSphere(mIsSphereShown);
}

//==============================================================================
void MainContentComponent::handleResetInputPositions()
{
    for (auto && it : mSourceInputs) {
        it->resetPosition();
    }
}

//==============================================================================
void MainContentComponent::handleResetMeterClipping()
{
    for (auto && it : mSourceInputs) {
        it->getVuMeter()->resetClipping();
    }
    for (auto && it : mSpeakers) {
        it->getVuMeter()->resetClipping();
    }
}

//==============================================================================
void MainContentComponent::handleInputColours()
{
    float hue = 0.0f;
    float inc = 1.0 / (mSourceInputs.size() + 1);
    for (auto && it : mSourceInputs) {
        it->setColor(juce::Colour::fromHSV(hue, 1, 0.75, 1), true);
        hue += inc;
    }
}

//==============================================================================
// Command manager methods.
void MainContentComponent::getAllCommands(juce::Array<juce::CommandID> & commands)
{
    // this returns the set of all commands that this target can perform.
    const juce::CommandID ids[] = {
        MainWindow::NewPresetID,
        MainWindow::OpenPresetID,
        MainWindow::SavePresetID,
        MainWindow::SaveAsPresetID,
        MainWindow::OpenSpeakerSetupID,
        MainWindow::ShowSpeakerEditID,
        MainWindow::Show2DViewID,
        MainWindow::ShowNumbersID,
        MainWindow::ShowSpeakersID,
        MainWindow::ShowTripletsID,
        MainWindow::ShowSourceLevelID,
        MainWindow::ShowSpeakerLevelID,
        MainWindow::ShowSphereID,
        MainWindow::ColorizeInputsID,
        MainWindow::ResetInputPosID,
        MainWindow::ResetMeterClipping,
        MainWindow::ShowOscLogView,
        MainWindow::PrefsID,
        MainWindow::QuitID,
        MainWindow::AboutID,
        MainWindow::OpenManualID,
    };

    commands.addArray(ids, juce::numElementsInArray(ids));
}

//==============================================================================
void MainContentComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo & result)
{
    const juce::String generalCategory("General");

    switch (commandID) {
    case MainWindow::NewPresetID:
        result.setInfo("New Project", "Close the current preset and open the default.", generalCategory, 0);
        result.addDefaultKeypress('N', juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::OpenPresetID:
        result.setInfo("Open Project", "Choose a new preset on disk.", generalCategory, 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::SavePresetID:
        result.setInfo("Save Project", "Save the current preset on disk.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::SaveAsPresetID:
        result.setInfo("Save Project As...", "Save the current preset under a new name on disk.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::shiftModifier | juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::OpenSpeakerSetupID:
        result.setInfo("Load Speaker Setup", "Choose a new speaker setup on disk.", generalCategory, 0);
        result.addDefaultKeypress('L', juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::ShowSpeakerEditID:
        result.setInfo("Speaker Setup Edition", "Edit the current speaker setup.", generalCategory, 0);
        result.addDefaultKeypress('W', juce::ModifierKeys::altModifier);
        break;
    case MainWindow::Show2DViewID:
        result.setInfo("Show 2D View", "Show the 2D action window.", generalCategory, 0);
        result.addDefaultKeypress('D', juce::ModifierKeys::altModifier);
        break;
    case MainWindow::ShowNumbersID:
        result.setInfo("Show Numbers", "Show source and speaker numbers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('N', juce::ModifierKeys::altModifier);
        result.setTicked(mIsNumbersShown);
        break;
    case MainWindow::ShowSpeakersID:
        result.setInfo("Show Speakers", "Show speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::altModifier);
        result.setTicked(mIsSpeakersShown);
        break;
    case MainWindow::ShowTripletsID:
        result.setInfo("Show Speaker Triplets", "Show speaker triplets on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('T', juce::ModifierKeys::altModifier);
        result.setTicked(mIsTripletsShown);
        break;
    case MainWindow::ShowSourceLevelID:
        result.setInfo("Show Source Activity", "Activate brightness on sources on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('A', juce::ModifierKeys::altModifier);
        result.setTicked(mIsSourceLevelShown);
        break;
    case MainWindow::ShowSpeakerLevelID:
        result.setInfo("Show Speaker Level", "Activate brightness on speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('L', juce::ModifierKeys::altModifier);
        result.setTicked(mIsSpeakerLevelShown);
        break;
    case MainWindow::ShowSphereID:
        result.setInfo("Show Sphere/Cube", "Show the sphere on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::altModifier);
        result.setTicked(mIsSphereShown);
        break;
    case MainWindow::ColorizeInputsID:
        result.setInfo("Colorize Inputs", "Spread the colour of the inputs over the colour range.", generalCategory, 0);
        result.addDefaultKeypress('C', juce::ModifierKeys::altModifier);
        break;
    case MainWindow::ResetInputPosID:
        result.setInfo("Reset Input Position", "Reset the position of the input sources.", generalCategory, 0);
        result.addDefaultKeypress('R', juce::ModifierKeys::altModifier);
        break;
    case MainWindow::ResetMeterClipping:
        result.setInfo("Reset Meter Clipping", "Reset clipping for all meters.", generalCategory, 0);
        result.addDefaultKeypress('M', juce::ModifierKeys::altModifier);
        break;
    case MainWindow::ShowOscLogView:
        result.setInfo("Show OSC Log Window", "Show the OSC logging window.", generalCategory, 0);
        break;
    case MainWindow::PrefsID:
        result.setInfo("Preferences...", "Open the preferences window.", generalCategory, 0);
        result.addDefaultKeypress(';', juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::QuitID:
        result.setInfo("Quit", "Quit the SpatGRIS.", generalCategory, 0);
        result.addDefaultKeypress('Q', juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::AboutID:
        result.setInfo("About SpatGRIS", "Open the about window.", generalCategory, 0);
        break;
    case MainWindow::OpenManualID:
        result.setInfo("Open Documentation", "Open the manual in pdf viewer.", generalCategory, 0);
        break;
    default:
        break;
    }
}

//==============================================================================
bool MainContentComponent::perform(const InvocationInfo & info)
{
    if (MainWindow::getMainAppWindow()) {
        switch (info.commandID) {
        case MainWindow::NewPresetID:
            handleNew();
            break;
        case MainWindow::OpenPresetID:
            handleOpenPreset();
            break;
        case MainWindow::SavePresetID:
            handleSavePreset();
            break;
        case MainWindow::SaveAsPresetID:
            handleSaveAsPreset();
            break;
        case MainWindow::OpenSpeakerSetupID:
            handleOpenSpeakerSetup();
            break;
        case MainWindow::ShowSpeakerEditID:
            handleShowSpeakerEditWindow();
            break;
        case MainWindow::Show2DViewID:
            handleShow2DView();
            break;
        case MainWindow::ShowNumbersID:
            handleShowNumbers();
            break;
        case MainWindow::ShowSpeakersID:
            handleShowSpeakers();
            break;
        case MainWindow::ShowTripletsID:
            handleShowTriplets();
            break;
        case MainWindow::ShowSourceLevelID:
            handleShowSourceLevel();
            break;
        case MainWindow::ShowSpeakerLevelID:
            handleShowSpeakerLevel();
            break;
        case MainWindow::ShowSphereID:
            handleShowSphere();
            break;
        case MainWindow::ColorizeInputsID:
            handleInputColours();
            break;
        case MainWindow::ResetInputPosID:
            handleResetInputPositions();
            break;
        case MainWindow::ResetMeterClipping:
            handleResetMeterClipping();
            break;
        case MainWindow::ShowOscLogView:
            handleShowOscLogView();
            break;
        case MainWindow::PrefsID:
            handleShowPreferences();
            break;
        case MainWindow::QuitID:
            dynamic_cast<MainWindow *>(&mMainWindow)->closeButtonPressed();
            break;
        case MainWindow::AboutID:
            handleShowAbout();
            break;
        case MainWindow::OpenManualID:
            handleOpenManual();
            break;
        default:
            return false;
        }
    }
    return true;
}

//==============================================================================
juce::PopupMenu MainContentComponent::getMenuForIndex(int menuIndex, const juce::String & menuName)
{
    juce::ApplicationCommandManager * commandManager = &mMainWindow.getApplicationCommandManager();

    juce::PopupMenu menu;

    if (menuName == "juce::File") {
        menu.addCommandItem(commandManager, MainWindow::NewPresetID);
        menu.addCommandItem(commandManager, MainWindow::OpenPresetID);
        menu.addCommandItem(commandManager, MainWindow::SavePresetID);
        menu.addCommandItem(commandManager, MainWindow::SaveAsPresetID);
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::OpenSpeakerSetupID);
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::PrefsID);
#if !JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::QuitID);
#endif
    } else if (menuName == "View") {
        menu.addCommandItem(commandManager, MainWindow::Show2DViewID);
        menu.addCommandItem(commandManager, MainWindow::ShowSpeakerEditID);
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::ShowNumbersID);
        menu.addCommandItem(commandManager, MainWindow::ShowSpeakersID);
        if (jackClient->getVbapDimensions() == 3) {
            menu.addCommandItem(commandManager, MainWindow::ShowTripletsID);
        } else {
            menu.addItem(MainWindow::ShowTripletsID, "Show Speaker Triplets", false, false);
        }
        menu.addCommandItem(commandManager, MainWindow::ShowSourceLevelID);
        menu.addCommandItem(commandManager, MainWindow::ShowSpeakerLevelID);
        menu.addCommandItem(commandManager, MainWindow::ShowSphereID);
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::ColorizeInputsID);
        menu.addCommandItem(commandManager, MainWindow::ResetInputPosID);
        menu.addCommandItem(commandManager, MainWindow::ResetMeterClipping);
        // TODO: Osc log window still crashes on MacOS. Useful only in debugging process.
        // menu.addSeparator();
        // menu.addCommandItem(commandManager, MainWindow::ShowOscLogView);
    } else if (menuName == "Help") {
        menu.addCommandItem(commandManager, MainWindow::AboutID);
        menu.addCommandItem(commandManager, MainWindow::OpenManualID);
    }
    return menu;
}

//==============================================================================
void MainContentComponent::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
    switch (menuItemID) {
    }
}

//==============================================================================
// Exit functions.
bool MainContentComponent::isPresetModified() const
{
    juce::File xmlFile = juce::File(mPathCurrentPreset.toStdString());
    juce::XmlDocument xmlDoc(xmlFile);
    std::unique_ptr<juce::XmlElement> savedState(xmlDoc.getDocumentElement());
    if (savedState == nullptr) {
        return true;
    }

    auto currentState = std::make_unique<juce::XmlElement>("ServerGRIS_Preset");
    getPresetData(currentState.get());

    if (!savedState->isEquivalentTo(currentState.get(), true)) {
        return true;
    }

    return false;
}

//==============================================================================
bool MainContentComponent::exitApp()
{
    int exitV = 2;

    if (isPresetModified()) {
        juce::AlertWindow alert("Exit SpatGRIS !",
                                "Do you want to save the current project ?",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert.addButton("Exit", 2, juce::KeyPress(juce::KeyPress::deleteKey));
        exitV = alert.runModalLoop();
        if (exitV == 1) {
            alert.setVisible(false);
            juce::ModalComponentManager::getInstance()->cancelAllModalComponents();
            juce::String dir = mApplicationProperties.getUserSettings()->getValue("lastPresetDirectory");
            if (!juce::File(dir).isDirectory()) {
                dir = juce::File("~").getFullPathName();
            }
            juce::String filename = juce::File(mPathCurrentPreset).getFileName();

            juce::FileChooser fc("Choose a file to save...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

            if (fc.browseForFileToSave(true)) {
                juce::String chosen = fc.getResults().getReference(0).getFullPathName();
                savePreset(chosen);
            } else {
                exitV = 0;
            }
        }
    }

    return (exitV != 0);
}

//==============================================================================
void MainContentComponent::connectionClientJack(juce::String nameCli, bool conn)
{
    unsigned int maxport = 0;
    for (auto const & cli : jackClient->getClients()) {
        if (cli.portEnd > maxport) {
            maxport = cli.portEnd;
        }
    }
    if (maxport > static_cast<unsigned int>(mAddInputsTextEditor->getTextValue().toString().getIntValue())) {
        mAddInputsTextEditor->setText(juce::String(maxport), juce::dontSendNotification);
        textEditorReturnKeyPressed(*mAddInputsTextEditor);
    }
    jackClient->setProcessBlockOn(false);
    jackClient->connectionClient(nameCli, conn);
    jackClient->setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::selectSpeaker(unsigned int idS)
{
    for (unsigned int i = 0; i < mSpeakers.size(); ++i) {
        if (i != idS) {
            mSpeakers[i]->unSelectSpeaker();
        } else {
            mSpeakers[i]->selectSpeaker();
        }
    }
    if (mEditSpeakersWindow != nullptr) {
        mEditSpeakersWindow->selectedRow(idS);
    }
}

//==============================================================================
void MainContentComponent::selectTripletSpeaker(int idS)
{
    int countS = 0;
    for (unsigned int i = 0; i < mSpeakers.size(); ++i) {
        if (mSpeakers[i]->isSelected()) {
            countS += 1;
        }
    }

    if (!mSpeakers[idS]->isSelected() && countS < 3) {
        mSpeakers[idS]->selectSpeaker();
        countS += 1;
    } else {
        mSpeakers[idS]->unSelectSpeaker();
    }

    if (countS == 3) {
        int i1 = -1, i2 = -1, i3 = -1;
        for (unsigned int i = 0; i < mSpeakers.size(); ++i) {
            if (mSpeakers[i]->isSelected()) {
                if (i1 == -1) {
                    i1 = i;
                } else {
                    if (i2 == -1) {
                        i2 = i;
                    } else {
                        if (i3 == -1) {
                            i3 = i;
                        }
                    }
                }
            }
        }
        if (i1 != -1 && i2 != -1 && i3 != -1) {
            Triplet tri;
            tri.id1 = i1;
            tri.id2 = i2;
            tri.id3 = i3;
            int posDel = -1;
            if (tripletExist(tri, posDel)) {
                mTriplets.erase(mTriplets.begin() + posDel);
            } else {
                mTriplets.push_back(tri);
            }
        }
    }
}

//==============================================================================
bool MainContentComponent::tripletExist(Triplet tri, int & pos) const
{
    pos = 0;
    for (auto const & ti : mTriplets) {
        if ((ti.id1 == tri.id1 && ti.id2 == tri.id2 && ti.id3 == tri.id3)
            || (ti.id1 == tri.id1 && ti.id2 == tri.id3 && ti.id3 == tri.id2)
            || (ti.id1 == tri.id2 && ti.id2 == tri.id1 && ti.id3 == tri.id3)
            || (ti.id1 == tri.id2 && ti.id2 == tri.id3 && ti.id3 == tri.id1)
            || (ti.id1 == tri.id3 && ti.id2 == tri.id2 && ti.id3 == tri.id1)
            || (ti.id1 == tri.id3 && ti.id2 == tri.id1 && ti.id3 == tri.id2)) {
            return true;
        }
        pos += 1;
    }

    return false;
}

//==============================================================================
void MainContentComponent::resetSpeakerIds()
{
    int id = 1;
    for (auto && it : mSpeakers) {
        it->setSpeakerId(id++);
    }
}

//==============================================================================
void MainContentComponent::reorderSpeakers(std::vector<int> const & newOrder)
{
    auto const size = mSpeakers.size();

    juce::Array<Speaker *> tempListSpeaker{};
    tempListSpeaker.resize(size);

    for (int i = 0; i < size; i++) {
        for (auto && it : mSpeakers) {
            if (it->getIdSpeaker() == newOrder[i]) {
                tempListSpeaker.setUnchecked(i, it);
                break;
            }
        }
    }

    mSpeakerLocks.lock();
    mSpeakers.clearQuick(false);
    mSpeakers.addArray(tempListSpeaker);
    mSpeakerLocks.unlock();
}

//==============================================================================
int MainContentComponent::getMaxSpeakerId() const
{
    int maxId = 0;
    for (auto && it : mSpeakers) {
        if (it->getIdSpeaker() > maxId)
            maxId = it->getIdSpeaker();
    }
    return maxId;
}

//==============================================================================
int MainContentComponent::getMaxSpeakerOutputPatch() const
{
    int maxOut = 0;
    for (auto && it : mSpeakers) {
        if (it->getOutputPatch() > maxOut)
            maxOut = it->getOutputPatch();
    }
    return maxOut;
}

//==============================================================================
void MainContentComponent::addSpeaker(int sortColumnId, bool isSortedForwards)
{
    int newId = getMaxSpeakerId() + 1;

    mSpeakerLocks.lock();
    mSpeakers.add(new Speaker(this, newId, newId, 0.0f, 0.0f, 1.0f));

    if (sortColumnId == 1 && isSortedForwards) {
        for (unsigned int i = 0; i < mSpeakers.size(); i++) {
            mSpeakers[i]->setSpeakerId(i + 1);
        }
    } else if (sortColumnId == 1 && !isSortedForwards) {
        for (unsigned int i = 0; i < mSpeakers.size(); i++) {
            mSpeakers[i]->setSpeakerId((int)mSpeakers.size() - i);
        }
    }
    mSpeakerLocks.unlock();

    jackClient->setProcessBlockOn(false);
    jackClient->addOutput(mSpeakers.getLast()->getOutputPatch());
    jackClient->setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::insertSpeaker(int position, int sortColumnId, bool isSortedForwards)
{
    int newPosition = position + 1;
    int newId = getMaxSpeakerId() + 1;
    int newOut = getMaxSpeakerOutputPatch() + 1;

    mSpeakerLocks.lock();
    if (sortColumnId == 1 && isSortedForwards) {
        newId = mSpeakers[position]->getIdSpeaker() + 1;
        mSpeakers.insert(newPosition, new Speaker(this, newId, newOut, 0.0f, 0.0f, 1.0f));
        for (unsigned int i = 0; i < mSpeakers.size(); i++) {
            mSpeakers.getUnchecked(i)->setSpeakerId(i + 1);
        }
    } else if (sortColumnId == 1 && !isSortedForwards) {
        newId = mSpeakers[position]->getIdSpeaker() - 1;
        ;
        mSpeakers.insert(newPosition, new Speaker(this, newId, newOut, 0.0f, 0.0f, 1.0f));
        for (unsigned int i = 0; i < mSpeakers.size(); i++) {
            mSpeakers.getUnchecked(i)->setSpeakerId(mSpeakers.size() - i);
        }
    } else {
        mSpeakers.insert(newPosition, new Speaker(this, newId, newOut, 0.0f, 0.0f, 1.0f));
    }
    mSpeakerLocks.unlock();

    jackClient->setProcessBlockOn(false);
    jackClient->clearOutput();
    for (auto && it : mSpeakers) {
        jackClient->addOutput(it->getOutputPatch());
    }
    jackClient->setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::removeSpeaker(int idSpeaker)
{
    jackClient->removeOutput(idSpeaker);
    mSpeakerLocks.lock();
    mSpeakers.remove(idSpeaker, true);
    mSpeakerLocks.unlock();
}

//==============================================================================
bool MainContentComponent::isRadiusNormalized() const
{
    auto const mode{ jackClient->getMode() };
    if (mode == VBAP || mode == VBAP_HRTF)
        return true;
    else
        return false;
}

//==============================================================================
void MainContentComponent::updateInputJack(int inInput, Input & inp)
{
    auto const mode{ jackClient->getMode() };
    auto & si = jackClient->getSourcesIn()[inInput];

    if (mode == LBAP) {
        si.radAzimuth = inp.getAzimuth();
        si.radElevation = juce::MathConstants<float>::halfPi - inp.getZenith();
    } else {
        si.azimuth = ((inp.getAzimuth() / juce::MathConstants<float>::twoPi) * 360.0f);
        if (si.azimuth > 180.0f) {
            si.azimuth = si.azimuth - 360.0f;
        }
        si.zenith = 90.0f - (inp.getZenith() / juce::MathConstants<float>::twoPi) * 360.0f;
    }
    si.radius = inp.getRadius();

    si.azimuthSpan = inp.getAzimuthSpan() * 0.5f;
    si.zenithSpan = inp.getZenithSpan() * 2.0f;

    if (mode == VBAP || mode == VBAP_HRTF) {
        jackClient->getVbapSourcesToUpdate()[inInput] = 1;
    }
}

//==============================================================================
void MainContentComponent::setListTripletFromVbap()
{
    clearListTriplet();
    for (auto const & it : jackClient->getVbapTriplets()) {
        Triplet tri;
        tri.id1 = it[0];
        tri.id2 = it[1];
        tri.id3 = it[2];
        mTriplets.push_back(tri);
    }
}

//==============================================================================
Speaker const * MainContentComponent::getSpeakerFromOutputPatch(int out) const
{
    for (auto const * it : mSpeakers) {
        if (it->getOutputPatch() == out && !it->isDirectOut()) {
            return it;
        }
    }
    return nullptr;
}

//==============================================================================
Speaker * MainContentComponent::getSpeakerFromOutputPatch(int out)
{
    for (auto it : mSpeakers) {
        if (it->getOutputPatch() == out && !it->isDirectOut()) {
            return it;
        }
    }
    return nullptr;
}

//==============================================================================
static void Linkwitz_Riley_compute_variables(double freq, double sr, double ** coeffs, int length)
{
    double wc = 2 * juce::MathConstants<float>::pi * freq;
    double wc2 = wc * wc;
    double wc3 = wc2 * wc;
    double wc4 = wc2 * wc2;
    double k = wc / tan(juce::MathConstants<float>::pi * freq / sr);
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

    (*coeffs)[0] = b1;
    (*coeffs)[1] = b2;
    (*coeffs)[2] = b3;
    (*coeffs)[3] = b4;
    (*coeffs)[4] = ha0;
    (*coeffs)[5] = ha1;
    (*coeffs)[6] = ha2;
}

//==============================================================================
float MainContentComponent::getLevelsAlpha(int indexLevel) const
{
    float level = jackClient->getLevelsIn(indexLevel);
    if (level > 0.0001) {
        // -80 dB
        return 1.0;
    } else {
        return sqrtf(level * 10000.0f);
    }
}

//==============================================================================
float MainContentComponent::getSpeakerLevelsAlpha(int indexLevel) const
{
    float level = jackClient->getLevelsOut(indexLevel);
    float alpha = 1.0;
    if (level > 0.001) {
        // -60 dB
        alpha = 1.0;
    } else {
        alpha = sqrtf(level * 1000.0f);
    }
    if (alpha < 0.6) {
        alpha = 0.6;
    }
    return alpha;
}

//==============================================================================
bool MainContentComponent::updateLevelComp()
{
    unsigned int dimensions = 2, directOutSpeakers = 0;

    if (mSpeakers.size() == 0)
        return false;

    // Test for a 2-D or 3-D configuration.
    float zenith = -1.0f;
    for (auto && it : mSpeakers) {
        if (it->isDirectOut()) {
            directOutSpeakers++;
            continue;
        }
        if (zenith == -1.0f) {
            zenith = it->getAziZenRad().y;
        } else if (it->getAziZenRad().y < (zenith - 4.9) || it->getAziZenRad().y > (zenith + 4.9)) {
            dimensions = 3;
        }
    }

    // Too few speakers...
    if ((mSpeakers.size() - directOutSpeakers) < dimensions) {
        juce::AlertWindow alert("Not enough speakers!    ",
                                "Do you want to reload previous config?    ",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("No", 0);
        alert.addButton("Yes", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            openXmlFileSpeaker(mPathCurrentFileSpeaker);
        }
        return false;
    }

    // Test for duplicated output patch.
    std::vector<int> tempout;
    for (unsigned int i = 0; i < mSpeakers.size(); i++) {
        if (!mSpeakers[i]->isDirectOut()) {
            tempout.push_back(mSpeakers[i]->getOutputPatch());
        }
    }

    std::sort(tempout.begin(), tempout.end());
    for (unsigned int i = 0; i < tempout.size() - 1; i++) {
        if (tempout[i] == tempout[i + 1]) {
            juce::AlertWindow alert("Duplicated Output Numbers!    ",
                                    "Some output numbers are used more than once. Do you want to continue anyway?    "
                                    "\nIf you continue, you may have to fix your speaker setup before using it!   ",
                                    juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("No", 0);
            alert.addButton("Yes", 1);
            if (alert.runModalLoop() == 0) {
                if (mPathCurrentFileSpeaker.compare(mPathLastVbapSpeakerSetup) == 0) {
                    openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
                } else {
                    openXmlFileSpeaker(mPathLastVbapSpeakerSetup);
                }
            }
            return false;
        }
    }

    jackClient->setProcessBlockOn(false);
    jackClient->setMaxOutputPatch(0);

    // Save mute/solo/directOut states
    bool inputsIsMuted[MAX_INPUTS];
    bool inputsIsSolo[MAX_INPUTS];
    auto const soloIn = jackClient->getSoloIn();
    int directOuts[MAX_INPUTS];
    auto const & sourcesIn{ jackClient->getSourcesIn() };
    for (unsigned int i = 0; i < MAX_INPUTS; i++) {
        inputsIsMuted[i] = sourcesIn[i].isMuted;
        inputsIsSolo[i] = sourcesIn[i].isSolo;
        directOuts[i] = sourcesIn[i].directOut;
    }

    bool outputsIsMuted[MAX_INPUTS];
    bool outputsIsSolo[MAX_INPUTS];
    bool soloOut = jackClient->getSoloOut();
    auto const & speakersOut{ jackClient->getSpeakersOut() };
    for (unsigned int i = 0; i < MAX_OUTPUTS; i++) {
        outputsIsMuted[i] = speakersOut[i].isMuted;
        outputsIsSolo[i] = speakersOut[i].isSolo;
    }

    // Cleanup speakers output patch
    for (auto & it : jackClient->getSpeakersOut()) {
        it.outputPatch = 0;
    }

    // Create outputs.
    int i = 0, x = 2;
    auto const mode{ jackClient->getMode() };
    for (auto && it : mSpeakers) {
        juce::Rectangle<int> level(x, 4, VU_METER_WIDTH_IN_PIXELS, 200);
        it->getVuMeter()->setBounds(level);
        it->getVuMeter()->resetClipping();
        mOutputsUiBox->getContent()->addAndMakeVisible(it->getVuMeter());
        it->getVuMeter()->repaint();

        x += VU_METER_WIDTH_IN_PIXELS;

        if (mode == VBAP || mode == VBAP_HRTF) {
            it->normalizeRadius();
        }

        SpeakerOut so;
        so.id = it->getOutputPatch();
        so.x = it->getCoordinate().x;
        so.y = it->getCoordinate().y;
        so.z = it->getCoordinate().z;
        so.azimuth = it->getAziZenRad().x;
        so.zenith = it->getAziZenRad().y;
        so.radius = it->getAziZenRad().z;
        so.outputPatch = it->getOutputPatch();
        so.directOut = it->isDirectOut();

        jackClient->getSpeakersOut()[i++] = so;

        if (static_cast<unsigned int>(it->getOutputPatch()) > jackClient->getMaxOutputPatch())
            jackClient->setMaxOutputPatch(it->getOutputPatch());
    }

    // Set user gain and highpass filter cutoff frequency for each speaker.
    for (auto const * speaker : mSpeakers) {
        jackClient->getSpeakersOut()[speaker->getOutputPatch() - 1].gain = std::pow(10.0f, speaker->getGain() * 0.05f);
        if (speaker->getHighPassCutoff() > 0.0f) {
            double * coeffs;
            Linkwitz_Riley_compute_variables(static_cast<double>(speaker->getHighPassCutoff()),
                                             (double)mSamplingRate,
                                             &coeffs,
                                             7);
            auto & speakerOut{ jackClient->getSpeakersOut()[speaker->getOutputPatch() - 1] };
            speakerOut.b1 = coeffs[0];
            speakerOut.b2 = coeffs[1];
            speakerOut.b3 = coeffs[2];
            speakerOut.b4 = coeffs[3];
            speakerOut.ha0 = coeffs[4];
            speakerOut.ha1 = coeffs[5];
            speakerOut.ha2 = coeffs[6];
            speakerOut.hpActive = true;
            free(coeffs);
        }
    }

    i = 0;
    x = 2;
    mInputLocks.lock();
    for (auto * input : mSourceInputs) {
        juce::Rectangle<int> level(x, 4, VU_METER_WIDTH_IN_PIXELS, 200);
        input->getVuMeter()->setBounds(level);
        input->getVuMeter()->updateDirectOutMenu(mSpeakers);
        input->getVuMeter()->resetClipping();
        mInputsUiBox->getContent()->addAndMakeVisible(input->getVuMeter());
        input->getVuMeter()->repaint();

        x += VU_METER_WIDTH_IN_PIXELS;

        SourceIn si;
        si.id = input->getId();
        si.radAzimuth = input->getAzimuth();
        si.radElevation = juce::MathConstants<float>::halfPi - input->getZenith();
        si.azimuth = input->getAzimuth();
        si.zenith = input->getZenith();
        si.radius = input->getRadius();
        si.gain = 0.0f;
        jackClient->getSourcesIn()[i++] = si;
    }

    mInputLocks.unlock();

    if (mEditSpeakersWindow != nullptr) {
        mEditSpeakersWindow->updateWinContent();
    }

    mOutputsUiBox->repaint();
    resized();

    // Temporarily remove direct out speakers to construct vbap or lbap algorithm.
    i = 0;
    std::vector<Speaker *> tempListSpeaker;
    tempListSpeaker.resize(mSpeakers.size());
    for (auto && it : mSpeakers) {
        if (!it->isDirectOut()) {
            tempListSpeaker[i++] = it;
        }
    }
    tempListSpeaker.resize(i);

    bool retval = false;
    if (mode == VBAP || mode == VBAP_HRTF) {
        jackClient->setVbapDimensions(dimensions);
        if (dimensions == 2) {
            setShowTriplets(false);
        }
        retval = jackClient->initSpeakersTriplet(tempListSpeaker, dimensions, mNeedToComputeVbap);

        if (retval) {
            setListTripletFromVbap();
            mNeedToComputeVbap = false;
        } else {
            juce::AlertWindow alert("Not a valid DOME 3-D configuration!    ",
                                    "Maybe you want to open it in CUBE mode? Reload the default speaker setup...    ",
                                    juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
            alert.runModalLoop();
            openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
            return false;
        }
    } else if (mode == LBAP) {
        setShowTriplets(false);
        retval = jackClient->lbapSetupSpeakerField(tempListSpeaker);
    }

    // Restore mute/solo/directOut states
    jackClient->setSoloIn(soloIn);
    for (unsigned int i = 0; i < MAX_INPUTS; i++) {
        auto & sourceIn{ jackClient->getSourcesIn()[i] };
        sourceIn.isMuted = inputsIsMuted[i];
        sourceIn.isSolo = inputsIsSolo[i];
        sourceIn.directOut = directOuts[i];
    }
    mInputLocks.lock();
    for (unsigned int i = 0; i < mSourceInputs.size(); i++) {
        mSourceInputs[i]->setDirectOutChannel(directOuts[i]);
    }
    mInputLocks.unlock();

    jackClient->setSoloOut(soloOut);
    for (unsigned int i = 0; i < MAX_OUTPUTS; i++) {
        auto & speakerOut{ jackClient->getSpeakersOut()[i] };
        speakerOut.isMuted = outputsIsMuted[i];
        speakerOut.isSolo = outputsIsSolo[i];
    }

    jackClient->setProcessBlockOn(true);

    return retval;
}

//==============================================================================
void MainContentComponent::setNameConfig()
{
    mNameConfig = mPathCurrentFileSpeaker.fromLastOccurrenceOf("/", false, false);
    mSpeakerViewComponent->setNameConfig(mNameConfig);
}

//==============================================================================
void MainContentComponent::muteInput(int const id, bool const mute)
{
    auto const index{ id - 1 };
    jackClient->getSourcesIn()[index].isMuted = mute;
}

//==============================================================================
void MainContentComponent::muteOutput(int const id, bool const mute)
{
    auto const index{ id - 1 };
    jackClient->getSpeakersOut()[index].isMuted = mute;
}

//==============================================================================
void MainContentComponent::soloInput(int const id, bool const solo)
{
    auto & sourcesIn{ jackClient->getSourcesIn() };
    auto const index{ id - 1 };
    sourcesIn[index].isSolo = solo;

    jackClient->setSoloIn(false);
    for (unsigned int i = 0; i < MAX_INPUTS; i++) {
        if (sourcesIn[i].isSolo) {
            jackClient->setSoloIn(true);
            break;
        }
    }
}

//==============================================================================
void MainContentComponent::soloOutput(int const id, bool const solo)
{
    auto const index{ id - 1 };
    auto & speakersOut{ jackClient->getSpeakersOut() };
    speakersOut[index].isSolo = solo;

    jackClient->setSoloOut(false);
    for (unsigned int i = 0; i < MAX_OUTPUTS; i++) {
        if (speakersOut[i].isSolo) {
            jackClient->setSoloOut(true);
            break;
        }
    }
}

//==============================================================================
void MainContentComponent::setDirectOut(int const id, int const chn)
{
    auto const index{ id - 1 };
    jackClient->getSourcesIn()[index].directOut = chn;
}

//==============================================================================
void MainContentComponent::reloadXmlFileSpeaker()
{
    if (juce::File(mPathCurrentFileSpeaker).existsAsFile()) {
        openXmlFileSpeaker(mPathCurrentFileSpeaker);
    }
}

//==============================================================================
void MainContentComponent::openXmlFileSpeaker(juce::String const & path)
{
    auto const oldPath{ mPathCurrentFileSpeaker };
    auto const isNewSameAsOld{ oldPath.compare(path) == 0 };
    auto const isNewSameAsLastSetup{ mPathLastVbapSpeakerSetup.compare(path) == 0 };
    auto ok{ false };
    if (!juce::File(path.toStdString()).existsAsFile()) {
        juce::AlertWindow alert("Error in Load Speaker Setup !",
                                "Can't found file " + path + ", the current setup will be kept.",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
    } else {
        mPathCurrentFileSpeaker = path;
        juce::XmlDocument xmlDoc{ juce::File{ mPathCurrentFileSpeaker } };
        auto const mainXmlElem(xmlDoc.getDocumentElement());
        if (!mainXmlElem) {
            juce::AlertWindow alert{ "Error in Load Speaker Setup !",
                                     "Your file is corrupted !\n" + xmlDoc.getLastParseError(),
                                     juce::AlertWindow::WarningIcon };
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("Ok", 0);
            alert.runModalLoop();
        } else {
            if (mainXmlElem->hasTagName("SpeakerSetup")) {
                mSpeakerLocks.lock();
                mSpeakers.clear();
                mSpeakerLocks.unlock();
                if (path.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) == 0) {
                    jackClient->setMode(VBAP_HRTF);
                    mModeSpatCombo->setSelectedId(VBAP_HRTF + 1, juce::NotificationType::dontSendNotification);
                } else if (path.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) == 0) {
                    jackClient->setMode(STEREO);
                    mModeSpatCombo->setSelectedId(STEREO + 1, juce::NotificationType::dontSendNotification);
                } else if (!isNewSameAsOld && oldPath.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
                           && oldPath.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
                    auto const spatMode = mainXmlElem->getIntAttribute("SpatMode");
                    jackClient->setMode(static_cast<ModeSpatEnum>(spatMode));
                    mModeSpatCombo->setSelectedId(spatMode + 1, juce::NotificationType::dontSendNotification);
                } else if (!isNewSameAsLastSetup) {
                    auto const spatMode = mainXmlElem->getIntAttribute("SpatMode");
                    jackClient->setMode(static_cast<ModeSpatEnum>(spatMode));
                    mModeSpatCombo->setSelectedId(spatMode + 1, juce::NotificationType::dontSendNotification);
                }

                auto const loadSetupFromXyz{ isNewSameAsOld && jackClient->getMode() == LBAP };

                setNameConfig();
                jackClient->setProcessBlockOn(false);
                jackClient->clearOutput();
                jackClient->setMaxOutputPatch(0);
                juce::Array<int> layoutIndexes{};
                int maxLayoutIndex{};
                forEachXmlChildElement(*mainXmlElem, ring)
                {
                    if (ring->hasTagName("Ring")) {
                        forEachXmlChildElement(*ring, spk)
                        {
                            if (spk->hasTagName("Speaker")) {
                                // Safety against layoutIndex doubles in the speaker setup.
                                auto layoutIndex = spk->getIntAttribute("LayoutIndex");
                                if (layoutIndexes.contains(layoutIndex)) {
                                    layoutIndex = ++maxLayoutIndex;
                                }
                                layoutIndexes.add(layoutIndex);
                                if (layoutIndex > maxLayoutIndex) {
                                    maxLayoutIndex = layoutIndex;
                                }

                                mSpeakers.add(new Speaker{ this,
                                                           layoutIndex,
                                                           spk->getIntAttribute("OutputPatch"),
                                                           static_cast<float>(spk->getDoubleAttribute("Azimuth")),
                                                           static_cast<float>(spk->getDoubleAttribute("Zenith")),
                                                           static_cast<float>(spk->getDoubleAttribute("Radius")) });
                                if (loadSetupFromXyz) {
                                    mSpeakers.getLast()->setCoordinate(
                                        glm::vec3(static_cast<float>(spk->getDoubleAttribute("PositionX")),
                                                  static_cast<float>(spk->getDoubleAttribute("PositionZ")),
                                                  static_cast<float>(spk->getDoubleAttribute("PositionY"))));
                                }
                                if (spk->hasAttribute("Gain")) {
                                    mSpeakers.getLast()->setGain(static_cast<float>(spk->getDoubleAttribute("Gain")));
                                }
                                if (spk->hasAttribute("HighPassCutoff")) {
                                    mSpeakers.getLast()->setHighPassCutoff(
                                        static_cast<float>(spk->getDoubleAttribute("HighPassCutoff")));
                                }
                                if (spk->hasAttribute("DirectOut")) {
                                    mSpeakers.getLast()->setDirectOut(spk->getBoolAttribute("DirectOut"));
                                }
                                jackClient->addOutput(static_cast<unsigned int>(spk->getIntAttribute("OutputPatch")));
                            }
                        }
                    }
                    if (ring->hasTagName("triplet")) {
                        Triplet tri;
                        tri.id1 = ring->getIntAttribute("id1");
                        tri.id2 = ring->getIntAttribute("id2");
                        tri.id3 = ring->getIntAttribute("id3");
                        mTriplets.push_back(tri);
                    }
                }
                jackClient->setProcessBlockOn(true);
                ok = true;
            } else {
                juce::String msg;
                if (mainXmlElem->hasTagName("ServerGRIS_Preset")) {
                    msg = "You are trying to open a Server document, and not a Speaker Setup !";
                } else {
                    msg = "Your file is corrupted !\n" + xmlDoc.getLastParseError();
                }
                juce::AlertWindow alert("Error in Load Speaker Setup !", msg, juce::AlertWindow::WarningIcon);
                alert.setLookAndFeel(&mLookAndFeel);
                alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
                alert.runModalLoop();
            }
        }
    }
    if (ok) {
        if (mPathCurrentFileSpeaker.endsWith("default_preset/default_speaker_setup.xml")) {
            mApplicationProperties.getUserSettings()->setValue(
                "lastSpeakerSetupDirectory",
                juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName());
        } else {
            mApplicationProperties.getUserSettings()->setValue(
                "lastSpeakerSetupDirectory",
                juce::File(mPathCurrentFileSpeaker).getParentDirectory().getFullPathName());
        }
        mNeedToComputeVbap = true;
        updateLevelComp();
        auto const mode{ jackClient->getMode() };
        if (mode != VBAP_HRTF && mode != STEREO) {
            if (mPathCurrentFileSpeaker.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
                && mPathCurrentFileSpeaker.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
                mPathLastVbapSpeakerSetup = mPathCurrentFileSpeaker;
            }
        }
    } else {
        if (isNewSameAsOld) {
            openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
        } else {
            openXmlFileSpeaker(oldPath);
        }
    }
}

//==============================================================================
void MainContentComponent::setTitle()
{
    juce::String version = STRING(JUCE_APP_VERSION);
    version = "SpatGRIS v" + version + " - ";
    mMainWindow.setName(version + juce::File(mPathCurrentPreset).getFileName());
}

//==============================================================================
void MainContentComponent::handleTimer(bool state)
{
    if (state) {
        startTimerHz(24);
    } else {
        stopTimer();
    }
}

//==============================================================================
void MainContentComponent::closeSpeakersConfigurationWindow()
{
    mEditSpeakersWindow.reset();
    jackClient->setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::openPreset(juce::String path)
{
    juce::String msg;
    jackClient->setProcessBlockOn(false);
    juce::File xmlFile = juce::File(path.toStdString());
    juce::XmlDocument xmlDoc(xmlFile);
    std::unique_ptr<juce::XmlElement> mainXmlElem(xmlDoc.getDocumentElement());
    if (mainXmlElem == nullptr) {
        juce::AlertWindow alert("Error in Open Preset !",
                                "Your file is corrupted !\n" + path.toStdString() + "\n"
                                    + xmlDoc.getLastParseError().toStdString(),
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
    } else {
        if (mainXmlElem->hasTagName("SpatServerGRIS_Preset") || mainXmlElem->hasTagName("ServerGRIS_Preset")) {
            mPathCurrentPreset = path;
            mOscInputPort
                = mainXmlElem->getIntAttribute("OSC_Input_Port"); // TODO: app preferences instead of project settings ?
            mAddInputsTextEditor->setText(mainXmlElem->getStringAttribute("Number_Of_Inputs"));
            mMasterGainOutSlider->setValue(mainXmlElem->getDoubleAttribute("Master_Gain_Out", 0.0),
                                           juce::sendNotification);
            mInterpolationSlider->setValue(mainXmlElem->getDoubleAttribute("Master_Interpolation", 0.1),
                                           juce::sendNotification);
            setShowNumbers(mainXmlElem->getBoolAttribute("Show_Numbers"));
            if (mainXmlElem->hasAttribute("Show_Speakers")) {
                setShowSpeakers(mainXmlElem->getBoolAttribute("Show_Speakers"));
            } else {
                setShowSpeakers(true);
            }
            if (mainXmlElem->hasAttribute("Show_Triplets")) {
                setShowTriplets(mainXmlElem->getBoolAttribute("Show_Triplets"));
            } else {
                setShowTriplets(false);
            }
            if (mainXmlElem->hasAttribute("Use_Alpha")) {
                mIsSourceLevelShown = mainXmlElem->getBoolAttribute("Use_Alpha");
            } else {
                mIsSourceLevelShown = false;
            }
            if (mainXmlElem->hasAttribute("Use_Alpha")) {
                mIsSourceLevelShown = mainXmlElem->getBoolAttribute("Use_Alpha");
            } else {
                mIsSourceLevelShown = false;
            }
            if (mainXmlElem->hasAttribute("Show_Speaker_Level")) {
                mIsSpeakerLevelShown = mainXmlElem->getBoolAttribute("Show_Speaker_Level");
            } else {
                mIsSpeakerLevelShown = false;
            }
            if (mainXmlElem->hasAttribute("Show_Sphere")) {
                mIsSphereShown = mainXmlElem->getBoolAttribute("Show_Sphere");
            } else {
                mIsSphereShown = false;
            }
            mSpeakerViewComponent->setShowSphere(mIsSphereShown);

            if (mainXmlElem->hasAttribute("CamAngleX")) {
                float angleX = mainXmlElem->getDoubleAttribute("CamAngleX");
                float angleY = mainXmlElem->getDoubleAttribute("CamAngleY");
                float distance = mainXmlElem->getDoubleAttribute("CamDistance");
                mSpeakerViewComponent->setCamPosition(angleX, angleY, distance);
            } else {
                mSpeakerViewComponent->setCamPosition(80.0f, 25.0f, 22.0f); // TODO: named constants ?
            }

            // Update
            textEditorReturnKeyPressed(*mAddInputsTextEditor);
            sliderValueChanged(mMasterGainOutSlider.get());
            sliderValueChanged(mInterpolationSlider.get());

            juce::File speakerSetup = juce::File(mPathCurrentFileSpeaker.toStdString());
            if (!mPathCurrentFileSpeaker.startsWith("/")) {
                mPathCurrentFileSpeaker
                    = DEFAULT_PRESET_DIRECTORY.getChildFile(mPathCurrentFileSpeaker).getFullPathName();
            }

            forEachXmlChildElement(*mainXmlElem, input)
            {
                if (input->hasTagName("Input")) {
                    for (auto && it : mSourceInputs) {
                        if (it->getId() == input->getIntAttribute("Index")) {
                            it->setColor(juce::Colour::fromFloatRGBA((float)input->getDoubleAttribute("R"),
                                                                     (float)input->getDoubleAttribute("G"),
                                                                     (float)input->getDoubleAttribute("B"),
                                                                     1.0f),
                                         true);
                            if (input->hasAttribute("DirectOut")) {
                                it->setDirectOutChannel(input->getIntAttribute("DirectOut"));
                                setDirectOut(it->getId(), input->getIntAttribute("DirectOut"));
                            } else {
                                it->setDirectOutChannel(0);
                                setDirectOut(it->getId(), 0);
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
            juce::AlertWindow alert("Error in Open Preset !", msg, juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
            alert.runModalLoop();
        }
    }

    jackClient->setPinkNoiseActive(false);
    jackClient->setProcessBlockOn(true);

    if (mPathCurrentPreset.endsWith("default_preset/default_preset.xml")) {
        mApplicationProperties.getUserSettings()->setValue(
            "lastPresetDirectory",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName());
    } else {
        mApplicationProperties.getUserSettings()->setValue(
            "lastPresetDirectory",
            juce::File(mPathCurrentPreset).getParentDirectory().getFullPathName());
    }
    setTitle();
}

//==============================================================================
void MainContentComponent::getPresetData(juce::XmlElement * xml) const
{
    xml->setAttribute("OSC_Input_Port", juce::String(mOscInputPort));
    xml->setAttribute("Number_Of_Inputs", mAddInputsTextEditor->getTextValue().toString());
    xml->setAttribute("Master_Gain_Out", mMasterGainOutSlider->getValue());
    xml->setAttribute("Master_Interpolation", mInterpolationSlider->getValue());
    xml->setAttribute("Show_Numbers", mIsNumbersShown);
    xml->setAttribute("Show_Speakers", mIsSpeakersShown);
    xml->setAttribute("Show_Triplets", mIsTripletsShown);
    xml->setAttribute("Use_Alpha", mIsSourceLevelShown);
    xml->setAttribute("Show_Speaker_Level", mIsSpeakerLevelShown);
    xml->setAttribute("Show_Sphere", mIsSphereShown);
    xml->setAttribute("CamAngleX", mSpeakerViewComponent->getCamAngleX());
    xml->setAttribute("CamAngleY", mSpeakerViewComponent->getCamAngleY());
    xml->setAttribute("CamDistance", mSpeakerViewComponent->getCamDistance());

    for (auto const it : mSourceInputs) {
        juce::XmlElement * xmlInput = new juce::XmlElement("Input");
        xmlInput->setAttribute("Index", it->getId());
        xmlInput->setAttribute("R", it->getColor().x);
        xmlInput->setAttribute("G", it->getColor().y);
        xmlInput->setAttribute("B", it->getColor().z);
        xmlInput->setAttribute("DirectOut", juce::String(it->getDirectOutChannel()));
        xml->addChildElement(xmlInput);
    }
}

//==============================================================================
void MainContentComponent::savePreset(juce::String path)
{
    juce::File xmlFile = juce::File(path.toStdString());
    auto xml = std::make_unique<juce::XmlElement>("ServerGRIS_Preset");
    getPresetData(xml.get());
    xml->writeTo(xmlFile);
    xmlFile.create();
    mPathCurrentPreset = path;
    mApplicationProperties.getUserSettings()->setValue(
        "lastPresetDirectory",
        juce::File(mPathCurrentPreset).getParentDirectory().getFullPathName());
    setTitle();
}

//==============================================================================
void MainContentComponent::saveSpeakerSetup(juce::String path)
{
    mPathCurrentFileSpeaker = path;
    juce::File xmlFile = juce::File(path.toStdString());
    juce::XmlElement xml{ "SpeakerSetup" };

    xml.setAttribute("Name", mNameConfig);
    xml.setAttribute("Dimension", 3);
    xml.setAttribute("SpatMode", getModeSelected());

    juce::XmlElement * xmlRing{ new juce::XmlElement("Ring") };

    for (auto const & it : mSpeakers) {
        juce::XmlElement * xmlInput{ new juce::XmlElement{ "Speaker" } };
        xmlInput->setAttribute("PositionY", it->getCoordinate().z);
        xmlInput->setAttribute("PositionX", it->getCoordinate().x);
        xmlInput->setAttribute("PositionZ", it->getCoordinate().y);
        xmlInput->setAttribute("Azimuth", it->getAziZenRad().x);
        xmlInput->setAttribute("Zenith", it->getAziZenRad().y);
        xmlInput->setAttribute("Radius", it->getAziZenRad().z);
        xmlInput->setAttribute("LayoutIndex", it->getIdSpeaker());
        xmlInput->setAttribute("OutputPatch", it->getOutputPatch());
        xmlInput->setAttribute("Gain", it->getGain());
        xmlInput->setAttribute("HighPassCutoff", it->getHighPassCutoff());
        xmlInput->setAttribute("DirectOut", it->isDirectOut());
        xmlRing->addChildElement(xmlInput);
    }
    xml.addChildElement(xmlRing);

    for (auto const & it : mTriplets) {
        juce::XmlElement * xmlInput{ new juce::XmlElement{ "triplet" } };
        xmlInput->setAttribute("id1", it.id1);
        xmlInput->setAttribute("id2", it.id2);
        xmlInput->setAttribute("id3", it.id3);
        xml.addChildElement(xmlInput);
    }

    xml.writeTo(xmlFile);
    xmlFile.create();

    mApplicationProperties.getUserSettings()->setValue(
        "lastSpeakerSetupDirectory",
        juce::File(mPathCurrentFileSpeaker).getParentDirectory().getFullPathName());

    mNeedToSaveSpeakerSetup = false;

    auto const mode{ jackClient->getMode() };
    if (mode != VBAP_HRTF && mode != STEREO) {
        if (mPathCurrentFileSpeaker.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
            && mPathCurrentFileSpeaker.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
            mPathLastVbapSpeakerSetup = mPathCurrentFileSpeaker;
        }
    }

    setNameConfig();
}

//==============================================================================
void MainContentComponent::saveProperties(juce::String device,
                                          int rate,
                                          int buff,
                                          int fileformat,
                                          int fileconfig,
                                          int attenuationDB,
                                          int attenuationHz,
                                          int oscPort)
{
    juce::PropertiesFile * props = mApplicationProperties.getUserSettings();

    juce::String DeviceValue = props->getValue("AlsaOutputDevice", "");
    int BufferValue = props->getIntValue("BufferValue", 1024);
    int RateValue = props->getIntValue("RateValue", 48000);
    int OscInputPort = props->getIntValue("OscInputPort", 18032);

    if (std::isnan(float(BufferValue)) || BufferValue == 0) {
        BufferValue = 1024;
    }
    if (std::isnan(float(RateValue)) || RateValue == 0) {
        RateValue = 48000;
    }

    if (device.compare(DeviceValue) != 0 || rate != RateValue || buff != BufferValue) {
        juce::AlertWindow alert("You Need to Restart SpatGRIS!",
                                "New settings will be effective on next launch of the SpatGRIS.",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Cancel", 0);
        alert.addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            props->setValue("AlsaOutputDevice", device);
            props->setValue("BufferValue", (int)buff);
            props->setValue("RateValue", (int)rate);
        }
    }

    // Handle OSC Input Port
    if (oscPort < 0 || oscPort > 65535) {
        oscPort = 18032;
    }
    if (oscPort != OscInputPort) {
        mOscInputPort = oscPort;
        props->setValue("OscInputPort", oscPort);
        mOscReceiver->closeConnection();
        if (mOscReceiver->startConnection(mOscInputPort)) {
            std::cout << "OSC receiver connected to port " << oscPort << '\n';
        } else {
            std::cout << "OSC receiver connection to port " << oscPort << " failed... Should popup an alert window."
                      << '\n';
        }
    }

    // Handle recording settings
    jackClient->setRecordFormat(fileformat);
    props->setValue("FileFormat", fileformat);

    jackClient->setRecordFileConfig(fileconfig);
    props->setValue("FileConfig", fileconfig);

    // Handle CUBE distance attenuation
    float linGain = powf(10.0f, ATTENUATION_DB[attenuationDB].getFloatValue() * 0.05f);
    jackClient->setAttenuationDb(linGain);
    props->setValue("AttenuationDB", attenuationDB);

    float coeff = expf(-juce::MathConstants<float>::twoPi * ATTENUATION_CUTOFFS[attenuationHz].getFloatValue()
                       / jackClient->getSampleRate());
    jackClient->setAttenuationHz(coeff);
    props->setValue("AttenuationHz", attenuationHz);

    mApplicationProperties.saveIfNeeded();
}

//==============================================================================
void MainContentComponent::timerCallback()
{
    mJackLoadLabel->setText(juce::String(jackClient->getCpuUsed() * 100.0f, 4) + " %", juce::dontSendNotification);
    int seconds = jackClient->getIndexRecord() / jackClient->getSampleRate();
    int minute = int(seconds / 60) % 60;
    seconds = int(seconds % 60);
    juce::String timeRecorded = ((minute < 10) ? "0" + juce::String(minute) : juce::String(minute)) + " : "
                                + ((seconds < 10) ? "0" + juce::String(seconds) : juce::String(seconds));
    mTimeRecordedLabel->setText(timeRecorded, juce::dontSendNotification);

    if (mStartRecordButton->getToggleState()) {
        mStartRecordButton->setToggleState(false, juce::dontSendNotification);
    }

    if (jackClient->isSavingRun()) {
        mStartRecordButton->setButtonText("Stop");
    } else if (jackClient->getRecordingPath() == "") {
        mStartRecordButton->setButtonText("Record");
    } else {
        mStartRecordButton->setButtonText("Record");
    }

    if (mIsRecording && !jackClient->isRecording()) {
        bool isReadyToMerge = true;
        for (unsigned int i = 0; i < MAX_OUTPUTS; i++) {
            if (jackClient->getRecorders()[i].backgroundThread.isThreadRunning()) {
                isReadyToMerge = false;
            }
        }
        if (isReadyToMerge) {
            mIsRecording = false;
            if (jackClient->getRecordFileConfig()) {
                jackClient->setProcessBlockOn(false);
                AudioRenderer * renderer = new AudioRenderer();
                renderer->prepareRecording(jackClient->getRecordingPath(),
                                           jackClient->getOutputFileNames(),
                                           jackClient->getSampleRate());
                renderer->runThread();
                jackClient->setProcessBlockOn(true);
            }
        }
    }

    if (jackClient->isOverloaded()) {
        mJackLoadLabel->setColour(juce::Label::backgroundColourId, juce::Colours::darkred);
    } else {
        mJackLoadLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    }

    for (auto && it : mSourceInputs) {
        it->getVuMeter()->update();
    }

    for (auto && it : mSpeakers) {
        it->getVuMeter()->update();
    }

    mJackClientListComponent->updateContentCli();

    if (mIsProcessForeground != juce::Process::isForegroundProcess()) {
        mIsProcessForeground = juce::Process::isForegroundProcess();
        if (mEditSpeakersWindow != nullptr && mIsProcessForeground) {
            mEditSpeakersWindow->setVisible(true);
            mEditSpeakersWindow->setAlwaysOnTop(true);
        } else if (mEditSpeakersWindow != nullptr && !mIsProcessForeground) {
            mEditSpeakersWindow->setVisible(false);
            mEditSpeakersWindow->setAlwaysOnTop(false);
        }
        if (mFlatViewWindow != nullptr && mIsProcessForeground) {
            mFlatViewWindow->toFront(false);
            toFront(true);
        }
    }
}

//==============================================================================
void MainContentComponent::paint(juce::Graphics & g)
{
    g.fillAll(mLookAndFeel.getWinBackgroundColour());
}

//==============================================================================
void MainContentComponent::textEditorFocusLost(juce::TextEditor & textEditor)
{
    textEditorReturnKeyPressed(textEditor);
}

//==============================================================================
void MainContentComponent::textEditorReturnKeyPressed(juce::TextEditor & textEditor)
{
    if (&textEditor == mAddInputsTextEditor.get()) {
        unsigned int num_of_inputs = (unsigned int)mAddInputsTextEditor->getTextValue().toString().getIntValue();
        if (num_of_inputs < 1) {
            mAddInputsTextEditor->setText("1");
        }
        if (num_of_inputs > MAX_INPUTS) {
            mAddInputsTextEditor->setText(juce::String(MAX_INPUTS));
        }

        if (jackClient->getInputPorts().size() != num_of_inputs) {
            jackClient->setProcessBlockOn(false);
            jackClient->addRemoveInput(num_of_inputs);
            jackClient->setProcessBlockOn(true);

            mInputLocks.lock();
            bool addInput = false;
            for (unsigned int i = 0; i < jackClient->getInputPorts().size(); i++) {
                if (i >= mSourceInputs.size()) {
                    mSourceInputs.add(new Input(*this, mSmallLookAndFeel, i + 1));
                    addInput = true;
                }
            }
            if (!addInput) {
                auto const listSourceInputSize{ mSourceInputs.size() };
                auto const jackClientInputPortSize{ jackClient->getInputPorts().size() };
                if (listSourceInputSize > jackClientInputPortSize) {
                    mSourceInputs.removeRange(jackClientInputPortSize, listSourceInputSize - jackClientInputPortSize);
                }
            }
            mInputLocks.unlock();
        }
        unfocusAllComponents();
        updateLevelComp();
    }
}

//==============================================================================
void MainContentComponent::buttonClicked(juce::Button * button)
{
    if (button == mStartRecordButton.get()) {
        if (jackClient->isRecording()) {
            jackClient->stopRecord();
            mStartRecordButton->setEnabled(false);
            mTimeRecordedLabel->setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
        } else {
            mIsRecording = true;
            jackClient->startRecord();
            mTimeRecordedLabel->setColour(juce::Label::textColourId, mLookAndFeel.getRedColour());
        }
        mStartRecordButton->setToggleState(jackClient->isRecording(), juce::dontSendNotification);
    } else if (button == mInitRecordButton.get()) {
        chooseRecordingPath();
        mStartRecordButton->setEnabled(true);
    }
}

//==============================================================================
void MainContentComponent::sliderValueChanged(juce::Slider * slider)
{
    if (slider == mMasterGainOutSlider.get()) {
        jackClient->setMasterGainOut(pow(10.0, mMasterGainOutSlider->getValue() * 0.05));
    }

    else if (slider == mInterpolationSlider.get()) {
        jackClient->setInterMaster(mInterpolationSlider->getValue());
    }
}

//==============================================================================
void MainContentComponent::comboBoxChanged(juce::ComboBox * comboBox)
{
    if (mEditSpeakersWindow != nullptr && mNeedToSaveSpeakerSetup) {
        juce::AlertWindow alert(
            "The speaker configuration has changed!    ",
            "Save your changes or close the speaker configuration window before switching mode...    ",
            juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        mModeSpatCombo->setSelectedId(jackClient->getMode() + 1, juce::NotificationType::dontSendNotification);
        return;
    }

    if (mModeSpatCombo.get() == comboBox) {
        jackClient->setProcessBlockOn(false);
        jackClient->setMode((ModeSpatEnum)(mModeSpatCombo->getSelectedId() - 1));
        switch (jackClient->getMode()) {
        case VBAP:
            openXmlFileSpeaker(mPathLastVbapSpeakerSetup);
            mNeedToSaveSpeakerSetup = false;
            mIsSpanShown = true;
            break;
        case LBAP:
            openXmlFileSpeaker(mPathLastVbapSpeakerSetup);
            mNeedToSaveSpeakerSetup = false;
            mIsSpanShown = true;
            break;
        case VBAP_HRTF:
            openXmlFileSpeaker(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName());
            mNeedToSaveSpeakerSetup = false;
            jackClient->resetHrtf();
            mIsSpanShown = false;
            break;
        case STEREO:
            openXmlFileSpeaker(STEREO_SPEAKER_SETUP_FILE.getFullPathName());
            mNeedToSaveSpeakerSetup = false;
            mIsSpanShown = false;
            break;
        default:
            break;
        }
        jackClient->setProcessBlockOn(true);

        if (mEditSpeakersWindow != nullptr) {
            juce::String windowName = juce::String("Speakers Setup Edition - ")
                                      + juce::String(MODE_SPAT_STRING[jackClient->getMode()]) + juce::String(" - ")
                                      + juce::File(mPathCurrentFileSpeaker).getFileName();
            mEditSpeakersWindow->setName(windowName);
        }
    }
}

//==============================================================================
juce::StringArray MainContentComponent::getMenuBarNames()
{
    char const * names[] = { "juce::File", "View", "Help", nullptr };
    return juce::StringArray{ names };
}

//==============================================================================
void MainContentComponent::setOscLogging(const juce::OSCMessage & message)
{
    if (mOscLogWindow != nullptr) {
        juce::String address = message.getAddressPattern().toString();
        mOscLogWindow->addToLog(address + "\n");
        juce::String msg;
        for (int i = 0; i < message.size(); i++) {
            if (message[i].isInt32()) {
                msg = msg + juce::String(message[i].getInt32()) + " ";
            } else if (message[i].isFloat32()) {
                msg = msg + juce::String(message[i].getFloat32()) + " ";
            } else if (message[i].isString()) {
                msg = msg + message[i].getString() + " ";
            }
        }
        mOscLogWindow->addToLog(msg + "\n");
    }
}

//==============================================================================
void MainContentComponent::chooseRecordingPath()
{
    juce::String dir = mApplicationProperties.getUserSettings()->getValue("lastRecordingDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String extF;
    juce::String extChoice;
    if (jackClient->getRecordFormat() == 0) {
        extF = ".wav";
        extChoice = "*.wav,*.aif";
    } else {
        extF = ".aif";
        extChoice = "*.aif,*.wav";
    }

    juce::FileChooser fc("Choose a file to save...", dir + "/recording" + extF, extChoice, USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToSave(true)) {
        juce::String filePath = fc.getResults().getReference(0).getFullPathName();
        mApplicationProperties.getUserSettings()->setValue("lastRecordingDirectory",
                                                           juce::File(filePath).getParentDirectory().getFullPathName());
        jackClient->setRecordingPath(filePath);
    }
    jackClient->prepareToRecord();
}

//==============================================================================
void MainContentComponent::resized()
{
    juce::Rectangle<int> r(getLocalBounds().reduced(2));

    mMenuBar->setBounds(0, 0, getWidth(), 20);
    r.removeFromTop(20);

    // Lay out the speaker view and the vertical divider.
    Component * vcomps[] = { mSpeakerViewComponent.get(), mVerticalDividerBar.get(), nullptr };

    // Lay out side-by-side and resize the components' heights as well as widths.
    mVerticalLayout.layOutComponents(vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);

    mMainUiBox->setBounds(mSpeakerViewComponent->getWidth() + 6,
                          20,
                          getWidth() - (mSpeakerViewComponent->getWidth() + 10),
                          getHeight());
    mMainUiBox->correctSize(getWidth() - mSpeakerViewComponent->getWidth() - 6, 610);

    mInputsUiBox->setBounds(0, 2, getWidth() - (mSpeakerViewComponent->getWidth() + 10), 231);
    mInputsUiBox->correctSize(((unsigned int)mSourceInputs.size() * (VU_METER_WIDTH_IN_PIXELS)) + 4, 200);

    mOutputsUiBox->setBounds(0, 233, getWidth() - (mSpeakerViewComponent->getWidth() + 10), 210);
    mOutputsUiBox->correctSize(((unsigned int)mSpeakers.size() * (VU_METER_WIDTH_IN_PIXELS)) + 4, 180);

    mControlUiBox->setBounds(0, 443, getWidth() - (mSpeakerViewComponent->getWidth() + 10), 145);
    mControlUiBox->correctSize(720, 145);
}
