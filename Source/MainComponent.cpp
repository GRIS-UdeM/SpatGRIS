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

#include "LevelComponent.h"
#include "MainWindow.h"

//==============================================================================
// Audio recorder class used to write an interleaved multi-channel soundfile on disk.
class AudioRenderer : public juce::ThreadWithProgressWindow
{
public:
    AudioRenderer()
        : juce::ThreadWithProgressWindow("Merging recorded mono files into an interleaved multi-channel file...",
                                         true,
                                         false)
    {
        setStatusMessage("Initializing...");
    }
    ~AudioRenderer() override = default;
    //==============================================================================
    void prepareRecording(juce::File const & file,
                          juce::Array<juce::File> const & filenames,
                          unsigned int const sampleRate)
    {
        this->fileToRecord = file;
        this->filenames = filenames;
        this->sampleRate = sampleRate;
    }
    //==============================================================================
    void run() override
    {
        unsigned int numberOfPasses{};
        unsigned int const blockSize{ 2048 };
        float const factor{ powf(2.0f, 31.0f) };
        int const numberOfChannels{ this->filenames.size() };
        juce::String const extF = this->filenames[0].getFileExtension();

        int ** data = new int *[2];
        data[0] = new int[blockSize];
        data[1] = 0;

        float ** buffer = new float *[numberOfChannels];
        for (int i = 0; i < numberOfChannels; i++) {
            buffer[i] = new float[blockSize];
        }

        formatManager.registerBasicFormats();
        juce::AudioFormatReader * readers[MaxOutputs];
        for (int i = 0; i < numberOfChannels; i++) {
            readers[i] = formatManager.createReaderFor(filenames[i]);
        }

        unsigned int const duration = (unsigned int)readers[0]->lengthInSamples;
        unsigned int const howmanyPasses = duration / blockSize;

        // Create an OutputStream to write to our destination file.
        this->fileToRecord.deleteFile();
        std::unique_ptr<juce::FileOutputStream> fileStream(this->fileToRecord.createOutputStream());

        juce::AudioFormatWriter * writer;

        if (fileStream != nullptr) {
            // Now create a writer object that writes to our output stream...
            if (extF == ".wav") {
                juce::WavAudioFormat wavFormat;
                writer = wavFormat.createWriterFor(fileStream.get(), this->sampleRate, numberOfChannels, 24, NULL, 0);
            } else {
                juce::AiffAudioFormat aiffFormat;
                writer = aiffFormat.createWriterFor(fileStream.get(), this->sampleRate, numberOfChannels, 24, NULL, 0);
            }

            if (writer != nullptr) {
                fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now
                                      // using it)
            }
        }

        while ((numberOfPasses * blockSize) < duration) {
            // this will update the progress bar on the dialog box
            setProgress(numberOfPasses / (double)howmanyPasses);

            setStatusMessage("Processing...");

            for (int i = 0; i < numberOfChannels; i++) {
                readers[i]->read(data, 1, numberOfPasses * blockSize, blockSize, false);
                for (unsigned int j = 0; j < blockSize; j++) {
                    buffer[i][j] = data[0][j] / factor;
                }
            }
            writer->writeFromFloatArrays(buffer, numberOfChannels, blockSize);
            ++numberOfPasses;
            wait(1);
        }

        if (writer != nullptr)
            delete writer;

        setProgress(-1.0); // setting a value beyond the range 0 -> 1 will show a spinning bar.
        setStatusMessage("Finishing the creation of the multi-channel file!");

        // Delete the monophonic files.
        for (auto && it : this->filenames) {
            it.deleteFile();
            wait(50);
        }
        wait(1000);
    }

    // This method gets called on the message thread once our thread has finished..
    void threadComplete(bool userPressedCancel) override
    {
        // thread finished normally.
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Multi-channel processing window",
                                               "Merging files finished!");

        // Clean up by deleting our thread object.
        delete this;
    }

private:
    //==============================================================================
    juce::AudioFormatManager formatManager;
    juce::File fileToRecord;
    juce::Array<juce::File> filenames;
    unsigned int sampleRate;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRenderer);
};

//==============================================================================
MainContentComponent::MainContentComponent(MainWindow & parent) : parent(parent)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);

    // Create the menubar.
    menuBar.reset(new juce::MenuBarComponent(this));
    this->addAndMakeVisible(menuBar.get());

    // Start the Splash screen.
    juce::File fs = juce::File(SPLASH_SCREEN_FILE);
    if (fs.exists()) {
        this->splash.reset(new juce::SplashScreen("SpatGRIS2", juce::ImageFileFormat::loadFrom(fs), true));
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
    this->applicationProperties.setStorageParameters(options);

    juce::PropertiesFile * props = this->applicationProperties.getUserSettings();

    // Initialize class variables.
    this->isProcessForeground = true;
    this->isNumbersShown = false;
    this->isSpeakersShown = true;
    this->isTripletsShown = false;
    this->isSourceLevelShown = false;
    this->isSphereShown = false;
    this->isSpanShown = true;
    this->isRecording = false;

    // Get a reference to the last opened VBAP speaker setup.
    juce::File lastVbap = juce::File(props->getValue("lastVbapSpeakerSetup", "./not_saved_yet"));
    if (!lastVbap.existsAsFile()) {
        this->pathLastVbapSpeakerSetup = DEFAULT_SPEAKER_SETUP_FILE.getFullPathName();
    } else {
        this->pathLastVbapSpeakerSetup = props->getValue("lastVbapSpeakerSetup");
    }

    this->winSpeakConfig = nullptr;
    this->windowProperties = nullptr;
    this->flatViewWindowSource = nullptr;
    this->aboutWindow = nullptr;
    this->oscLogWindow = nullptr;

    // SpeakerViewComponent 3D view
    this->speakerView.reset(new SpeakerViewComponent(this));
    addAndMakeVisible(this->speakerView.get());

    // Box Main
    this->boxMainUI.reset(new Box(mGrisFeel, "", true, false));
    addAndMakeVisible(this->boxMainUI.get());

    // Box Inputs
    this->boxInputsUI.reset(new Box(mGrisFeel, "Inputs"));
    addAndMakeVisible(this->boxInputsUI.get());

    // Box Outputs
    this->boxOutputsUI.reset(new Box(mGrisFeel, "Outputs"));
    addAndMakeVisible(this->boxOutputsUI.get());

    // Box Control
    this->boxControlUI.reset(new Box(mGrisFeel));
    addAndMakeVisible(this->boxControlUI.get());

    this->boxMainUI->getContent()->addAndMakeVisible(this->boxInputsUI.get());
    this->boxMainUI->getContent()->addAndMakeVisible(this->boxOutputsUI.get());
    this->boxMainUI->getContent()->addAndMakeVisible(this->boxControlUI.get());

    // Components in Box Control
    this->labelJackStatus.reset(
        addLabel("Jack Unknown", "Jack Status", 0, 0, 80, 28, this->boxControlUI->getContent()));
    this->labelJackLoad.reset(addLabel("0.000000 %", "Load Jack CPU", 80, 0, 80, 28, this->boxControlUI->getContent()));
    this->labelJackRate.reset(addLabel("00000 Hz", "Rate", 160, 0, 80, 28, this->boxControlUI->getContent()));
    this->labelJackBuffer.reset(addLabel("0000 spls", "Buffer Size", 240, 0, 80, 28, this->boxControlUI->getContent()));
    this->labelJackInfo.reset(
        addLabel("...", "Jack Inputs/Outputs system", 320, 0, 90, 28, this->boxControlUI->getContent()));

    this->labelJackStatus->setColour(juce::Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackLoad->setColour(juce::Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackRate->setColour(juce::Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackBuffer->setColour(juce::Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    this->labelJackInfo->setColour(juce::Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());

    addLabel("Gain", "Master Gain Outputs", 15, 30, 120, 20, this->boxControlUI->getContent());
    this->sliderMasterGainOut.reset(
        addSlider("Master Gain", "Master Gain Outputs", 5, 45, 60, 60, this->boxControlUI->getContent()));
    this->sliderMasterGainOut->setRange(-60.0, 12.0, 0.01);
    this->sliderMasterGainOut->setTextValueSuffix(" dB");

    addLabel("Interpolation", "Master Interpolation", 60, 30, 120, 20, this->boxControlUI->getContent());
    this->sliderInterpolation.reset(
        addSlider("Inter", "Interpolation", 70, 45, 60, 60, this->boxControlUI->getContent()));
    this->sliderInterpolation->setRange(0.0, 1.0, 0.001);

    addLabel("Mode :", "Mode of spatilization", 150, 30, 60, 20, this->boxControlUI->getContent());
    this->comBoxModeSpat.reset(
        addComboBox("", "Mode of spatilization", 155, 48, 90, 22, this->boxControlUI->getContent()));
    for (int i = 0; i < ModeSpatString.size(); i++) {
        this->comBoxModeSpat->addItem(ModeSpatString[i], i + 1);
    }

    this->tedAddInputs.reset(
        addTextEditor("Inputs :", "0", "Numbers of Inputs", 122, 83, 43, 22, this->boxControlUI->getContent()));
    this->tedAddInputs->setInputRestrictions(3, "0123456789");

    this->butInitRecord.reset(
        addButton("Init Recording", "Init Recording", 268, 48, 103, 24, this->boxControlUI->getContent()));

    this->butStartRecord.reset(
        addButton("Record", "Start/Stop Record", 268, 83, 60, 24, this->boxControlUI->getContent()));
    this->butStartRecord->setEnabled(false);

    this->labelTimeRecorded.reset(addLabel("00:00", "Record time", 327, 83, 50, 24, this->boxControlUI->getContent()));

    // Jack client box.
    this->jackClientListComponentJack.reset(new JackClientListComponent(this, &mGrisFeel));
    this->jackClientListComponentJack->setBounds(410, 0, 304, 138);
    this->boxControlUI->getContent()->addAndMakeVisible(this->jackClientListComponentJack.get());

    // Set up the layout and resizer bars.
    this->verticalLayout.setItemLayout(0,
                                       -0.2,
                                       -0.8,
                                       -0.435); // width of the speaker view must be between 20% and 80%, preferably 50%
    this->verticalLayout.setItemLayout(1, 8, 8, 8); // the vertical divider drag-bar thing is always 8 pixels wide
    this->verticalLayout.setItemLayout(
        2,
        150,
        -1.0,
        -0.565); // right panes must be at least 150 pixels wide, preferably 50% of the total width
    this->verticalDividerBar.reset(new juce::StretchableLayoutResizerBar(&verticalLayout, 1, true));
    this->addAndMakeVisible(verticalDividerBar.get());

    // Default application window size.
    this->setSize(1285, 610);

    // Jack Initialization parameters.
    unsigned int BufferValue = props->getIntValue("BufferValue", 1024);
    unsigned int RateValue = props->getIntValue("RateValue", 48000);

    if (std::isnan(float(BufferValue)) || BufferValue == 0 || std::isnan(float(RateValue)) || RateValue == 0) {
        BufferValue = 1024;
        RateValue = 48000;
    }

    this->samplingRate = RateValue;

    // Start Jack Server and client.
    int errorCode = 0;
    alsaOutputDevice = props->getValue("AlsaOutputDevice", "");
    this->jackServer.reset(new JackServerGris(RateValue, BufferValue, alsaOutputDevice, &errorCode));
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
    this->jackClient.reset(new JackClientGris());

    alsaAvailableOutputDevices = this->jackServer->getAvailableOutputDevices();

    unsigned int fileformat = props->getIntValue("FileFormat", 0);
    this->jackClient->setRecordFormat(fileformat);
    unsigned int fileconfig = props->getIntValue("FileConfig", 0);
    this->jackClient->setRecordFileConfig(fileconfig);

    if (!jackClient->isReady()) {
        this->labelJackStatus->setText("Jack ERROR", juce::dontSendNotification);
    } else {
        this->labelJackStatus->setText("Jack Run", juce::dontSendNotification);
    }

    this->labelJackRate->setText(juce::String(this->jackClient->sampleRate) + " Hz", juce::dontSendNotification);
    this->labelJackBuffer->setText(juce::String(this->jackClient->bufferSize) + " spls", juce::dontSendNotification);
    this->labelJackInfo->setText("I : " + juce::String(this->jackClient->numberOutputs)
                                     + " - O : " + juce::String(this->jackClient->numberInputs),
                                 juce::dontSendNotification);

    // Start the OSC Receiver.
    this->oscReceiver.reset(new OscInput(*this));
    this->oscReceiver->startConnection(this->oscInputPort);

    // Default widget values.
    this->sliderMasterGainOut->setValue(0.0);
    this->sliderInterpolation->setValue(0.1);
    this->comBoxModeSpat->setSelectedId(1);

    this->tedAddInputs->setText("16", juce::dontSendNotification);
    textEditorReturnKeyPressed(*this->tedAddInputs);

    // Open the default preset if lastOpenPreset is not a valid file.
    juce::File preset = juce::File(props->getValue("lastOpenPreset", "./not_saved_yet"));
    if (!preset.existsAsFile()) {
        this->openPreset(DEFAULT_PRESET_FILE.getFullPathName());
    } else {
        this->openPreset(props->getValue("lastOpenPreset"));
    }

    // Open the default speaker setup if lastOpenSpeakerSetup is not a valid file.
    juce::File setup = juce::File(props->getValue("lastOpenSpeakerSetup", "./not_saved_yet"));
    if (!setup.existsAsFile()) {
        this->openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
    } else {
        this->openXmlFileSpeaker(props->getValue("lastOpenSpeakerSetup"));
    }

    // End layout and start refresh timer.
    this->resized();
    startTimerHz(24);

    // End Splash screen.
    if (this->splash) {
        this->splash->deleteAfterDelay(juce::RelativeTime::seconds(4), false);
        this->splash.release();
    }

    // Initialize the command manager for the menubar items.
    juce::ApplicationCommandManager & commandManager = this->parent.getApplicationCommandManager();
    commandManager.registerAllCommandsForTarget(this);

    // Restore last vertical divider position and speaker view cam distance.
    if (props->containsKey("sashPosition")) {
        int trueSize = (int)round((this->getWidth() - 3) * abs(props->getDoubleValue("sashPosition")));
        this->verticalLayout.setItemPosition(1, trueSize);
    }
}

//==============================================================================
MainContentComponent::~MainContentComponent()
{
    juce::PropertiesFile * props = this->applicationProperties.getUserSettings();
    props->setValue("lastOpenPreset", this->pathCurrentPreset);
    props->setValue("lastOpenSpeakerSetup", this->pathCurrentFileSpeaker);
    props->setValue("lastVbapSpeakerSetup", this->pathLastVbapSpeakerSetup);
    props->setValue("sashPosition", this->verticalLayout.getItemCurrentRelativeSize(0));
    this->applicationProperties.saveIfNeeded();
    this->applicationProperties.closeFiles();

    this->lockSpeakers.lock();
    this->listSpeaker.clear();
    this->lockSpeakers.unlock();

    this->lockInputs.lock();
    this->listSourceInput.clear();
    this->lockInputs.unlock();
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
    lb->setFont(mGrisFeel.getFont());
    lb->setLookAndFeel(&mGrisFeel);
    lb->setColour(juce::Label::textColourId, mGrisFeel.getFontColour());
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
    tb->setColour(juce::ToggleButton::textColourId, mGrisFeel.getFontColour());
    tb->setLookAndFeel(&mGrisFeel);
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
    tb->setColour(juce::ToggleButton::textColourId, mGrisFeel.getFontColour());
    tb->setLookAndFeel(&mGrisFeel);
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
    te->setTextToShowWhenEmpty(emptyS, mGrisFeel.getOffColour());
    te->setColour(juce::ToggleButton::textColourId, mGrisFeel.getFontColour());
    te->setLookAndFeel(&mGrisFeel);

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
    sd->setRotaryParameters(M_PI * 1.3f, M_PI * 2.7f, true);
    sd->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    sd->setColour(juce::ToggleButton::textColourId, mGrisFeel.getFontColour());
    sd->setLookAndFeel(&mGrisFeel);
    sd->addListener(this);
    into->addAndMakeVisible(sd);
    return sd;
}

//==============================================================================
juce::ComboBox * MainContentComponent::addComboBox(const juce::String & s,
                                                   const juce::String & stooltip,
                                                   int x,
                                                   int y,
                                                   int w,
                                                   int h,
                                                   Component * into)
{
    juce::ComboBox * cb = new juce::ComboBox();
    cb->setTooltip(stooltip);
    cb->setSize(w, h);
    cb->setTopLeftPosition(x, y);
    cb->setLookAndFeel(&mGrisFeel);
    cb->addListener(this);
    into->addAndMakeVisible(cb);
    return cb;
}

//==============================================================================
// Menu item action handlers.
void MainContentComponent::handleNew()
{
    juce::AlertWindow alert("Closing current preset !", "Do you want to save ?", juce::AlertWindow::InfoIcon);
    alert.setLookAndFeel(&mGrisFeel);
    alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::deleteKey));
    alert.addButton("yes", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert.addButton("No", 2, juce::KeyPress(juce::KeyPress::escapeKey));

    int status = alert.runModalLoop();
    if (status == 1) {
        this->handleSavePreset();
    } else if (status == 0) {
        return;
    }

    this->openPreset(DEFAULT_PRESET_FILE.getFullPathName());
}

//==============================================================================
void MainContentComponent::handleOpenPreset()
{
    juce::String dir = this->applicationProperties.getUserSettings()->getValue("lastPresetDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(this->pathCurrentPreset).getFileName();

    juce::FileChooser fc("Choose a file to open...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    bool loaded = false;
    if (fc.browseForFileToOpen()) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        juce::AlertWindow alert("Open Project !",
                                "You want to load : " + chosen + "\nEverything not saved will be lost !",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mGrisFeel);
        alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert.addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            this->openPreset(chosen);
            loaded = true;
        }
    }

    if (loaded) { // Check for direct out OutputPatch mismatch.
        for (auto && it : listSourceInput) {
            if (it->getDirectOutChannel() != 0) {
                std::vector<int> directOutOutputPatches = this->jackClient->getDirectOutOutputPatches();
                if (std::find(directOutOutputPatches.begin(), directOutOutputPatches.end(), it->getDirectOutChannel())
                    == directOutOutputPatches.end()) {
                    juce::AlertWindow alert(
                        "Direct Out Mismatch!",
                        "Some of the direct out channels of this project don't exist in the current speaker setup.\n",
                        juce::AlertWindow::WarningIcon);
                    alert.setLookAndFeel(&mGrisFeel);
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
    if (!juce::File(this->pathCurrentPreset).existsAsFile()
        || this->pathCurrentPreset.endsWith("default_preset/default_preset.xml")) {
        this->handleSaveAsPreset();
    }
    this->savePreset(this->pathCurrentPreset);
}

//==============================================================================
void MainContentComponent::handleSaveAsPreset()
{
    juce::String dir = this->applicationProperties.getUserSettings()->getValue("lastPresetDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(this->pathCurrentPreset).getFileName();

    juce::FileChooser fc("Choose a file to save...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToSave(true)) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        this->savePreset(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleOpenSpeakerSetup()
{
    juce::String dir = this->applicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(this->pathCurrentFileSpeaker).getFileName();

    juce::FileChooser fc("Choose a file to open...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToOpen()) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        juce::AlertWindow alert("Load Speaker Setup !",
                                "You want to load : " + chosen + "\nEverything not saved will be lost !",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mGrisFeel);
        alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert.addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            alert.setVisible(false);
            this->openXmlFileSpeaker(chosen);
        }
    }
}

//==============================================================================
void MainContentComponent::handleSaveAsSpeakerSetup()
{
    juce::String dir = this->applicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory");
    if (!juce::File(dir).isDirectory() || dir.endsWith("/default_preset")) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(this->pathCurrentFileSpeaker).getFileName();

    juce::FileChooser fc("Choose a file to save...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToSave(true)) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        this->saveSpeakerSetup(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleShowSpeakerEditWindow()
{
    juce::Rectangle<int> result(this->getScreenX() + this->speakerView->getWidth() + 20,
                                this->getScreenY() + 20,
                                850,
                                600);
    if (this->winSpeakConfig == nullptr) {
        juce::String windowName = juce::String("Speakers Setup Edition - ")
                                  + juce::String(ModeSpatString[this->jackClient->modeSelected]) + juce::String(" - ")
                                  + juce::File(this->pathCurrentFileSpeaker).getFileName();
        this->winSpeakConfig.reset(new EditSpeakersWindow(windowName, this->mGrisFeel, *this, this->nameConfig));
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

//==============================================================================
void MainContentComponent::handleShowPreferences()
{
    juce::PropertiesFile * props = this->applicationProperties.getUserSettings();
    if (this->windowProperties == nullptr) {
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
        this->windowProperties.reset(new PropertiesWindow{ *this,
                                                           this->mGrisFeel,
                                                           alsaAvailableOutputDevices,
                                                           alsaOutputDevice,
                                                           RateValues.indexOf(juce::String(RateValue)),
                                                           BufferSizes.indexOf(juce::String(BufferValue)),
                                                           static_cast<int>(FileFormat),
                                                           static_cast<int>(FileConfig),
                                                           static_cast<int>(AttenuationDB),
                                                           static_cast<int>(AttenuationHz),
                                                           static_cast<int>(OscInputPort) });
    }
    int height = 450;
    if (alsaAvailableOutputDevices.isEmpty()) {
        height = 420;
    }
    juce::Rectangle<int> result(this->getScreenX() + (this->speakerView->getWidth() / 2) - 150,
                                this->getScreenY() + (this->speakerView->getHeight() / 2) - 75,
                                270,
                                height);
    this->windowProperties->setBounds(result);
    this->windowProperties->setResizable(false, false);
    this->windowProperties->setUsingNativeTitleBar(true);
    this->windowProperties->setVisible(true);
    this->windowProperties->repaint();
}

//==============================================================================
void MainContentComponent::handleShow2DView()
{
    if (this->flatViewWindowSource == nullptr) {
        this->flatViewWindowSource.reset(new FlatViewWindow{ *this, this->mGrisFeel });
    } else {
        this->flatViewWindowRect.setBounds(this->flatViewWindowSource->getScreenX(),
                                           this->flatViewWindowSource->getScreenY(),
                                           this->flatViewWindowSource->getWidth(),
                                           this->flatViewWindowSource->getHeight());
    }

    if (this->flatViewWindowRect.getWidth() == 0) {
        this->flatViewWindowRect.setBounds(this->getScreenX() + this->speakerView->getWidth() + 22,
                                           this->getScreenY() + 100,
                                           500,
                                           500);
    }

    this->flatViewWindowSource->setBounds(this->flatViewWindowRect);
    this->flatViewWindowSource->setResizable(true, true);
    this->flatViewWindowSource->setUsingNativeTitleBar(true);
    this->flatViewWindowSource->setVisible(true);
}

//==============================================================================
void MainContentComponent::handleShowOscLogView()
{
    if (this->oscLogWindow == nullptr) {
        this->oscLogWindow.reset(new OscLogWindow("OSC Logging Windows",
                                                  this->mGrisFeel.getWinBackgroundColour(),
                                                  juce::DocumentWindow::allButtons,
                                                  this,
                                                  &this->mGrisFeel));
    }
    this->oscLogWindow->centreWithSize(500, 500);
    this->oscLogWindow->setResizable(false, false);
    this->oscLogWindow->setUsingNativeTitleBar(true);
    this->oscLogWindow->setVisible(true);
    this->oscLogWindow->repaint();
}

//==============================================================================
void MainContentComponent::handleShowAbout()
{
    if (this->aboutWindow.get() == nullptr) {
        this->aboutWindow.reset(new AboutWindow{ "About SpatGRIS", this->mGrisFeel, *this });
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
    this->setShowNumbers(!this->isNumbersShown);
}

//==============================================================================
void MainContentComponent::setShowNumbers(bool state)
{
    this->isNumbersShown = state;
    this->speakerView->setShowNumber(state);
}

//==============================================================================
void MainContentComponent::handleShowSpeakers()
{
    this->setShowSpeakers(!this->isSpeakersShown);
}

//==============================================================================
void MainContentComponent::setShowSpeakers(bool state)
{
    this->isSpeakersShown = state;
    this->speakerView->setHideSpeaker(!state);
}

//==============================================================================
void MainContentComponent::handleShowTriplets()
{
    this->setShowTriplets(!this->isTripletsShown);
}

//==============================================================================
void MainContentComponent::setShowTriplets(bool state)
{
    if (this->getModeSelected() == LBAP && state == true) {
        juce::AlertWindow alert("Can't draw triplets !",
                                "Triplets are not effective with the CUBE mode.",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mGrisFeel);
        alert.addButton("Close", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        this->setShowTriplets(false);
    } else if (this->validateShowTriplets() || state == false) {
        this->isTripletsShown = state;
        this->speakerView->setShowTriplets(state);
    } else {
        juce::AlertWindow alert("Can't draw all triplets !",
                                "Maybe you didn't compute your current speaker setup ?",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mGrisFeel);
        alert.addButton("Close", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        this->setShowTriplets(false);
    }
}

//==============================================================================
bool MainContentComponent::validateShowTriplets() const
{
    int success = true;
    for (unsigned int i = 0; i < this->getListTriplet().size(); ++i) {
        Speaker const * spk1 = this->getSpeakerFromOutputPatch(this->getListTriplet()[i].id1);
        Speaker const * spk2 = this->getSpeakerFromOutputPatch(this->getListTriplet()[i].id2);
        Speaker const * spk3 = this->getSpeakerFromOutputPatch(this->getListTriplet()[i].id3);

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
    this->isSourceLevelShown = !this->isSourceLevelShown;
}

//==============================================================================
void MainContentComponent::handleShowSpeakerLevel()
{
    this->isSpeakerLevelShown = !this->isSpeakerLevelShown;
}

//==============================================================================
void MainContentComponent::handleShowSphere()
{
    this->isSphereShown = !this->isSphereShown;
    this->speakerView->setShowSphere(this->isSphereShown);
}

//==============================================================================
void MainContentComponent::handleResetInputPositions()
{
    for (auto && it : this->listSourceInput) {
        it->resetPosition();
    }
}

//==============================================================================
void MainContentComponent::handleResetMeterClipping()
{
    for (auto && it : this->listSourceInput) {
        it->getVuMeter()->resetClipping();
    }
    for (auto && it : this->listSpeaker) {
        it->getVuMeter()->resetClipping();
    }
}

//==============================================================================
void MainContentComponent::handleInputColours()
{
    float hue = 0.0f;
    float inc = 1.0 / (this->listSourceInput.size() + 1);
    for (auto && it : this->listSourceInput) {
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
        result.setTicked(this->isNumbersShown);
        break;
    case MainWindow::ShowSpeakersID:
        result.setInfo("Show Speakers", "Show speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::altModifier);
        result.setTicked(this->isSpeakersShown);
        break;
    case MainWindow::ShowTripletsID:
        result.setInfo("Show Speaker Triplets", "Show speaker triplets on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('T', juce::ModifierKeys::altModifier);
        result.setTicked(this->isTripletsShown);
        break;
    case MainWindow::ShowSourceLevelID:
        result.setInfo("Show Source Activity", "Activate brightness on sources on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('A', juce::ModifierKeys::altModifier);
        result.setTicked(this->isSourceLevelShown);
        break;
    case MainWindow::ShowSpeakerLevelID:
        result.setInfo("Show Speaker Level", "Activate brightness on speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('L', juce::ModifierKeys::altModifier);
        result.setTicked(this->isSpeakerLevelShown);
        break;
    case MainWindow::ShowSphereID:
        result.setInfo("Show Sphere/Cube", "Show the sphere on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::altModifier);
        result.setTicked(this->isSphereShown);
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
            this->handleNew();
            break;
        case MainWindow::OpenPresetID:
            this->handleOpenPreset();
            break;
        case MainWindow::SavePresetID:
            this->handleSavePreset();
            break;
        case MainWindow::SaveAsPresetID:
            this->handleSaveAsPreset();
            break;
        case MainWindow::OpenSpeakerSetupID:
            this->handleOpenSpeakerSetup();
            break;
        case MainWindow::ShowSpeakerEditID:
            this->handleShowSpeakerEditWindow();
            break;
        case MainWindow::Show2DViewID:
            this->handleShow2DView();
            break;
        case MainWindow::ShowNumbersID:
            this->handleShowNumbers();
            break;
        case MainWindow::ShowSpeakersID:
            this->handleShowSpeakers();
            break;
        case MainWindow::ShowTripletsID:
            this->handleShowTriplets();
            break;
        case MainWindow::ShowSourceLevelID:
            this->handleShowSourceLevel();
            break;
        case MainWindow::ShowSpeakerLevelID:
            this->handleShowSpeakerLevel();
            break;
        case MainWindow::ShowSphereID:
            this->handleShowSphere();
            break;
        case MainWindow::ColorizeInputsID:
            this->handleInputColours();
            break;
        case MainWindow::ResetInputPosID:
            this->handleResetInputPositions();
            break;
        case MainWindow::ResetMeterClipping:
            this->handleResetMeterClipping();
            break;
        case MainWindow::ShowOscLogView:
            this->handleShowOscLogView();
            break;
        case MainWindow::PrefsID:
            this->handleShowPreferences();
            break;
        case MainWindow::QuitID:
            dynamic_cast<MainWindow *>(&this->parent)->closeButtonPressed();
            break;
        case MainWindow::AboutID:
            this->handleShowAbout();
            break;
        case MainWindow::OpenManualID:
            this->handleOpenManual();
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
    juce::ApplicationCommandManager * commandManager = &this->parent.getApplicationCommandManager();

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
        if (this->jackClient->vbapDimensions == 3) {
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
    juce::File xmlFile = juce::File(this->pathCurrentPreset.toStdString());
    juce::XmlDocument xmlDoc(xmlFile);
    std::unique_ptr<juce::XmlElement> savedState(xmlDoc.getDocumentElement());
    if (savedState == nullptr) {
        return true;
    }

    auto currentState = std::make_unique<juce::XmlElement>("ServerGRIS_Preset");
    this->getPresetData(currentState.get());

    if (!savedState->isEquivalentTo(currentState.get(), true)) {
        return true;
    }

    return false;
}

//==============================================================================
bool MainContentComponent::exitApp()
{
    int exitV = 2;

    if (this->isPresetModified()) {
        juce::AlertWindow alert("Exit SpatGRIS !",
                                "Do you want to save the current project ?",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mGrisFeel);
        alert.addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert.addButton("Exit", 2, juce::KeyPress(juce::KeyPress::deleteKey));
        exitV = alert.runModalLoop();
        if (exitV == 1) {
            alert.setVisible(false);
            juce::ModalComponentManager::getInstance()->cancelAllModalComponents();
            juce::String dir = this->applicationProperties.getUserSettings()->getValue("lastPresetDirectory");
            if (!juce::File(dir).isDirectory()) {
                dir = juce::File("~").getFullPathName();
            }
            juce::String filename = juce::File(this->pathCurrentPreset).getFileName();

            juce::FileChooser fc("Choose a file to save...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

            if (fc.browseForFileToSave(true)) {
                juce::String chosen = fc.getResults().getReference(0).getFullPathName();
                this->savePreset(chosen);
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
    for (auto && cli : this->jackClient->listClient) {
        if (cli.portEnd > maxport) {
            maxport = cli.portEnd;
        }
    }
    if (maxport > (unsigned int)this->tedAddInputs->getTextValue().toString().getIntValue()) {
        this->tedAddInputs->setText(juce::String(maxport), juce::dontSendNotification);
        textEditorReturnKeyPressed(*this->tedAddInputs);
    }
    this->jackClient->processBlockOn = false;
    this->jackClient->connectionClient(nameCli, conn);
    this->jackClient->processBlockOn = true;
}

//==============================================================================
void MainContentComponent::selectSpeaker(unsigned int idS)
{
    for (unsigned int i = 0; i < this->listSpeaker.size(); ++i) {
        if (i != idS) {
            this->listSpeaker[i]->unSelectSpeaker();
        } else {
            this->listSpeaker[i]->selectSpeaker();
        }
    }
    if (this->winSpeakConfig != nullptr) {
        this->winSpeakConfig->selectedRow(idS);
    }
}

//==============================================================================
void MainContentComponent::selectTripletSpeaker(int idS)
{
    int countS = 0;
    for (unsigned int i = 0; i < this->listSpeaker.size(); ++i) {
        if (this->listSpeaker[i]->isSelected()) {
            countS += 1;
        }
    }

    if (!this->listSpeaker[idS]->isSelected() && countS < 3) {
        this->listSpeaker[idS]->selectSpeaker();
        countS += 1;
    } else {
        this->listSpeaker[idS]->unSelectSpeaker();
    }

    if (countS == 3) {
        int i1 = -1, i2 = -1, i3 = -1;
        for (unsigned int i = 0; i < this->listSpeaker.size(); ++i) {
            if (this->listSpeaker[i]->isSelected()) {
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
            if (this->tripletExist(tri, posDel)) {
                this->listTriplet.erase(this->listTriplet.begin() + posDel);
            } else {
                this->listTriplet.push_back(tri);
            }
        }
    }
}

//==============================================================================
bool MainContentComponent::tripletExist(Triplet tri, int & pos) const
{
    pos = 0;
    for (auto const & ti : this->listTriplet) {
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
    for (auto && it : this->listSpeaker) {
        it->setSpeakerId(id++);
    }
}

//==============================================================================
void MainContentComponent::reorderSpeakers(std::vector<int> const & newOrder)
{
    auto const size = this->listSpeaker.size();

    juce::Array<Speaker *> tempListSpeaker{};
    tempListSpeaker.resize(size);

    for (int i = 0; i < size; i++) {
        for (auto && it : this->listSpeaker) {
            if (it->getIdSpeaker() == newOrder[i]) {
                tempListSpeaker.setUnchecked(i, it);
                break;
            }
        }
    }

    this->lockSpeakers.lock();
    this->listSpeaker.clearQuick(false);
    this->listSpeaker.addArray(tempListSpeaker);
    this->lockSpeakers.unlock();
}

//==============================================================================
int MainContentComponent::getMaxSpeakerId() const
{
    int maxId = 0;
    for (auto && it : this->listSpeaker) {
        if (it->getIdSpeaker() > maxId)
            maxId = it->getIdSpeaker();
    }
    return maxId;
}

//==============================================================================
int MainContentComponent::getMaxSpeakerOutputPatch() const
{
    int maxOut = 0;
    for (auto && it : this->listSpeaker) {
        if (it->getOutputPatch() > maxOut)
            maxOut = it->getOutputPatch();
    }
    return maxOut;
}

//==============================================================================
void MainContentComponent::addSpeaker(int sortColumnId, bool isSortedForwards)
{
    int newId = this->getMaxSpeakerId() + 1;

    this->lockSpeakers.lock();
    this->listSpeaker.add(new Speaker(this, newId, newId, 0.0f, 0.0f, 1.0f));

    if (sortColumnId == 1 && isSortedForwards) {
        for (unsigned int i = 0; i < this->listSpeaker.size(); i++) {
            this->listSpeaker[i]->setSpeakerId(i + 1);
        }
    } else if (sortColumnId == 1 && !isSortedForwards) {
        for (unsigned int i = 0; i < this->listSpeaker.size(); i++) {
            this->listSpeaker[i]->setSpeakerId((int)this->listSpeaker.size() - i);
        }
    }
    this->lockSpeakers.unlock();

    this->jackClient->processBlockOn = false;
    this->jackClient->addOutput(this->listSpeaker.getLast()->getOutputPatch());
    this->jackClient->processBlockOn = true;
}

//==============================================================================
void MainContentComponent::insertSpeaker(int position, int sortColumnId, bool isSortedForwards)
{
    int newPosition = position + 1;
    int newId = this->getMaxSpeakerId() + 1;
    int newOut = this->getMaxSpeakerOutputPatch() + 1;

    this->lockSpeakers.lock();
    if (sortColumnId == 1 && isSortedForwards) {
        newId = this->listSpeaker[position]->getIdSpeaker() + 1;
        this->listSpeaker.insert(newPosition, new Speaker(this, newId, newOut, 0.0f, 0.0f, 1.0f));
        for (unsigned int i = 0; i < this->listSpeaker.size(); i++) {
            this->listSpeaker.getUnchecked(i)->setSpeakerId(i + 1);
        }
    } else if (sortColumnId == 1 && !isSortedForwards) {
        newId = this->listSpeaker[position]->getIdSpeaker() - 1;
        ;
        this->listSpeaker.insert(newPosition, new Speaker(this, newId, newOut, 0.0f, 0.0f, 1.0f));
        for (unsigned int i = 0; i < this->listSpeaker.size(); i++) {
            this->listSpeaker.getUnchecked(i)->setSpeakerId(this->listSpeaker.size() - i);
        }
    } else {
        this->listSpeaker.insert(newPosition, new Speaker(this, newId, newOut, 0.0f, 0.0f, 1.0f));
    }
    this->lockSpeakers.unlock();

    this->jackClient->processBlockOn = false;
    this->jackClient->clearOutput();
    for (auto && it : this->listSpeaker) {
        this->jackClient->addOutput(it->getOutputPatch());
    }
    this->jackClient->processBlockOn = true;
}

//==============================================================================
void MainContentComponent::removeSpeaker(int idSpeaker)
{
    this->jackClient->removeOutput(idSpeaker);
    this->lockSpeakers.lock();
    this->listSpeaker.remove(idSpeaker, true);
    this->lockSpeakers.unlock();
}

//==============================================================================
bool MainContentComponent::isRadiusNormalized() const
{
    if (this->jackClient->modeSelected == VBAP || this->jackClient->modeSelected == VBAP_HRTF)
        return true;
    else
        return false;
}

//==============================================================================
void MainContentComponent::updateInputJack(int inInput, Input & inp)
{
    SourceIn * si = &this->jackClient->listSourceIn[inInput];

    if (this->jackClient->modeSelected == LBAP) {
        si->radazi = inp.getAziMuth();
        si->radele = M_PI2 - inp.getZenith();
    } else {
        si->azimuth = ((inp.getAziMuth() / M2_PI) * 360.0f);
        if (si->azimuth > 180.0f) {
            si->azimuth = si->azimuth - 360.0f;
        }
        si->zenith = 90.0f - (inp.getZenith() / M2_PI) * 360.0f;
    }
    si->radius = inp.getRadius();

    si->aziSpan = inp.getAziMuthSpan() * 0.5f;
    si->zenSpan = inp.getZenithSpan() * 2.0f;

    if (this->jackClient->modeSelected == VBAP || this->jackClient->modeSelected == VBAP_HRTF) {
        this->jackClient->vbapSourcesToUpdate[inInput] = 1;
    }
}

//==============================================================================
void MainContentComponent::setListTripletFromVbap()
{
    this->clearListTriplet();
    for (unsigned int i = 0; i < this->jackClient->vbap_triplets.size(); i++) {
        Triplet tri;
        tri.id1 = this->jackClient->vbap_triplets[i][0];
        tri.id2 = this->jackClient->vbap_triplets[i][1];
        tri.id3 = this->jackClient->vbap_triplets[i][2];
        this->listTriplet.push_back(tri);
    }
}

//==============================================================================
Speaker const * MainContentComponent::getSpeakerFromOutputPatch(int out) const
{
    for (auto const it : this->listSpeaker) {
        if (it->getOutputPatch() == out && !it->isDirectOut()) {
            return it;
        }
    }
    return nullptr;
}

//==============================================================================
Speaker * MainContentComponent::getSpeakerFromOutputPatch(int out)
{
    for (auto it : this->listSpeaker) {
        if (it->getOutputPatch() == out && !it->isDirectOut()) {
            return it;
        }
    }
    return nullptr;
}

//==============================================================================
static void Linkwitz_Riley_compute_variables(double freq, double sr, double ** coeffs, int length)
{
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

    (*coeffs)[0] = b1;
    (*coeffs)[1] = b2;
    (*coeffs)[2] = b3;
    (*coeffs)[3] = b4;
    (*coeffs)[4] = ha0;
    (*coeffs)[5] = ha1;
    (*coeffs)[6] = ha2;
}

//==============================================================================
bool MainContentComponent::updateLevelComp()
{
    unsigned int dimensions = 2, directOutSpeakers = 0;

    if (this->listSpeaker.size() == 0)
        return false;

    // Test for a 2-D or 3-D configuration.
    float zenith = -1.0f;
    for (auto && it : this->listSpeaker) {
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
    if ((this->listSpeaker.size() - directOutSpeakers) < dimensions) {
        juce::AlertWindow alert("Not enough speakers!    ",
                                "Do you want to reload previous config?    ",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mGrisFeel);
        alert.addButton("No", 0);
        alert.addButton("Yes", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            this->openXmlFileSpeaker(this->pathCurrentFileSpeaker);
        }
        return false;
    }

    // Test for duplicated output patch.
    std::vector<int> tempout;
    for (unsigned int i = 0; i < this->listSpeaker.size(); i++) {
        if (!this->listSpeaker[i]->isDirectOut()) {
            tempout.push_back(this->listSpeaker[i]->getOutputPatch());
        }
    }

    std::sort(tempout.begin(), tempout.end());
    for (unsigned int i = 0; i < tempout.size() - 1; i++) {
        if (tempout[i] == tempout[i + 1]) {
            juce::AlertWindow alert("Duplicated Output Numbers!    ",
                                    "Some output numbers are used more than once. Do you want to continue anyway?    "
                                    "\nIf you continue, you may have to fix your speaker setup before using it!   ",
                                    juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mGrisFeel);
            alert.addButton("No", 0);
            alert.addButton("Yes", 1);
            if (alert.runModalLoop() == 0) {
                if (this->pathCurrentFileSpeaker.compare(this->pathLastVbapSpeakerSetup) == 0) {
                    this->openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
                } else {
                    this->openXmlFileSpeaker(this->pathLastVbapSpeakerSetup);
                }
            }
            return false;
        }
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
    for (unsigned int i = 0; i < MaxOutputs; i++) {
        outputsIsMuted[i] = (&this->jackClient->listSpeakerOut[i])->isMuted;
        outputsIsSolo[i] = (&this->jackClient->listSpeakerOut[i])->isSolo;
    }

    // Cleanup speakers output patch
    for (auto && it : this->jackClient->listSpeakerOut) {
        it.outputPatch = 0;
    }

    // Create outputs.
    int i = 0, x = 2;
    for (auto && it : this->listSpeaker) {
        juce::Rectangle<int> level(x, 4, VuMeterWidthInPixels, 200);
        it->getVuMeter()->setBounds(level);
        it->getVuMeter()->resetClipping();
        this->boxOutputsUI->getContent()->addAndMakeVisible(it->getVuMeter());
        it->getVuMeter()->repaint();

        x += VuMeterWidthInPixels;

        if (this->jackClient->modeSelected == VBAP || this->jackClient->modeSelected == VBAP_HRTF) {
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

        this->jackClient->listSpeakerOut[i++] = so;

        if ((unsigned int)it->getOutputPatch() > this->jackClient->maxOutputPatch)
            this->jackClient->maxOutputPatch = it->getOutputPatch();
    }

    // Set user gain and highpass filter cutoff frequency for each speaker.
    for (auto && it : this->listSpeaker) {
        this->jackClient->listSpeakerOut[it->outputPatch - 1].gain = pow(10.0, it->getGain() * 0.05);
        if (it->getHighPassCutoff() > 0.0f) {
            double * coeffs;
            Linkwitz_Riley_compute_variables((double)it->getHighPassCutoff(), (double)this->samplingRate, &coeffs, 7);
            this->jackClient->listSpeakerOut[it->outputPatch - 1].b1 = coeffs[0];
            this->jackClient->listSpeakerOut[it->outputPatch - 1].b2 = coeffs[1];
            this->jackClient->listSpeakerOut[it->outputPatch - 1].b3 = coeffs[2];
            this->jackClient->listSpeakerOut[it->outputPatch - 1].b4 = coeffs[3];
            this->jackClient->listSpeakerOut[it->outputPatch - 1].ha0 = coeffs[4];
            this->jackClient->listSpeakerOut[it->outputPatch - 1].ha1 = coeffs[5];
            this->jackClient->listSpeakerOut[it->outputPatch - 1].ha2 = coeffs[6];
            this->jackClient->listSpeakerOut[it->outputPatch - 1].hpActive = true;
            free(coeffs);
        }
    }

    i = 0;
    x = 2;
    this->lockInputs.lock();
    for (auto && it : this->listSourceInput) {
        juce::Rectangle<int> level(x, 4, VuMeterWidthInPixels, 200);
        it->getVuMeter()->setBounds(level);
        it->getVuMeter()->updateDirectOutMenu(this->listSpeaker);
        it->getVuMeter()->resetClipping();
        this->boxInputsUI->getContent()->addAndMakeVisible(it->getVuMeter());
        it->getVuMeter()->repaint();

        x += VuMeterWidthInPixels;

        SourceIn si;
        si.id = it->getId();
        si.radazi = it->getAziMuth();
        si.radele = M_PI2 - it->getZenith();
        si.azimuth = it->getAziMuth();
        si.zenith = it->getZenith();
        si.radius = it->getRadius();
        si.gain = 0.0f;
        this->jackClient->listSourceIn[i++] = si;
    }

    this->lockInputs.unlock();

    if (this->winSpeakConfig != nullptr) {
        this->winSpeakConfig->updateWinContent();
    }

    this->boxOutputsUI->repaint();
    this->resized();

    // Temporarily remove direct out speakers to construct vbap or lbap algorithm.
    i = 0;
    std::vector<Speaker *> tempListSpeaker;
    tempListSpeaker.resize(this->listSpeaker.size());
    for (auto && it : this->listSpeaker) {
        if (!it->isDirectOut()) {
            tempListSpeaker[i++] = it;
        }
    }
    tempListSpeaker.resize(i);

    bool retval = false;
    if (this->jackClient->modeSelected == VBAP || this->jackClient->modeSelected == VBAP_HRTF) {
        this->jackClient->vbapDimensions = dimensions;
        if (dimensions == 2) {
            this->setShowTriplets(false);
        }
        retval = this->jackClient->initSpeakersTripplet(tempListSpeaker, dimensions, this->needToComputeVbap);

        if (retval) {
            this->setListTripletFromVbap();
            this->needToComputeVbap = false;
        } else {
            juce::AlertWindow alert("Not a valid DOME 3-D configuration!    ",
                                    "Maybe you want to open it in CUBE mode? Reload the default speaker setup...    ",
                                    juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mGrisFeel);
            alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
            alert.runModalLoop();
            this->openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
            return false;
        }
    } else if (this->jackClient->modeSelected == LBAP) {
        this->setShowTriplets(false);
        retval = this->jackClient->lbapSetupSpeakerField(tempListSpeaker);
    }

    // Restore mute/solo/directout states
    this->jackClient->soloIn = soloIn;
    for (unsigned int i = 0; i < MaxInputs; i++) {
        (&this->jackClient->listSourceIn[i])->isMuted = inputsIsMuted[i];
        (&this->jackClient->listSourceIn[i])->isSolo = inputsIsSolo[i];
        (&this->jackClient->listSourceIn[i])->directOut = directOuts[i];
    }
    this->lockInputs.lock();
    for (unsigned int i = 0; i < this->listSourceInput.size(); i++) {
        this->listSourceInput[i]->setDirectOutChannel(directOuts[i]);
    }
    this->lockInputs.unlock();

    this->jackClient->soloOut = soloOut;
    for (unsigned int i = 0; i < MaxOutputs; i++) {
        (&this->jackClient->listSpeakerOut[i])->isMuted = outputsIsMuted[i];
        (&this->jackClient->listSpeakerOut[i])->isSolo = outputsIsSolo[i];
    }

    this->jackClient->processBlockOn = true;

    return retval;
}

//==============================================================================
void MainContentComponent::setNameConfig()
{
    this->nameConfig = this->pathCurrentFileSpeaker.fromLastOccurrenceOf("/", false, false);
    this->speakerView->setNameConfig(this->nameConfig);
}

//==============================================================================
void MainContentComponent::muteInput(int id, bool mute)
{
    (&this->jackClient->listSourceIn[id - 1])->isMuted = mute;
}

//==============================================================================
void MainContentComponent::muteOutput(int id, bool mute)
{
    (&this->jackClient->listSpeakerOut[id - 1])->isMuted = mute;
}

//==============================================================================
void MainContentComponent::soloInput(int id, bool solo)
{
    (&this->jackClient->listSourceIn[id - 1])->isSolo = solo;

    this->jackClient->soloIn = false;
    for (unsigned int i = 0; i < MaxInputs; i++) {
        if ((&this->jackClient->listSourceIn[i])->isSolo) {
            this->jackClient->soloIn = true;
            break;
        }
    }
}

//==============================================================================
void MainContentComponent::soloOutput(int id, bool solo)
{
    (&this->jackClient->listSpeakerOut[id - 1])->isSolo = solo;

    this->jackClient->soloOut = false;
    for (unsigned int i = 0; i < MaxOutputs; i++) {
        if ((&this->jackClient->listSpeakerOut[i])->isSolo) {
            this->jackClient->soloOut = true;
            break;
        }
    }
}

//==============================================================================
void MainContentComponent::setDirectOut(int id, int chn)
{
    (&this->jackClient->listSourceIn[id - 1])->directOut = chn;
}

//==============================================================================
void MainContentComponent::reloadXmlFileSpeaker()
{
    if (juce::File(this->pathCurrentFileSpeaker).existsAsFile()) {
        this->openXmlFileSpeaker(this->pathCurrentFileSpeaker);
    }
}

//==============================================================================
void MainContentComponent::openXmlFileSpeaker(juce::String path)
{
    juce::String msg;
    juce::String oldPath = this->pathCurrentFileSpeaker;
    bool isNewSameAsOld = oldPath.compare(path) == 0;
    bool isNewSameAsLastSetup = this->pathLastVbapSpeakerSetup.compare(path) == 0;
    bool ok = false;
    if (!juce::File(path.toStdString()).existsAsFile()) {
        juce::AlertWindow alert("Error in Load Speaker Setup !",
                                "Can't found file " + path.toStdString() + ", the current setup will be kept.",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mGrisFeel);
        alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
    } else {
        this->pathCurrentFileSpeaker = path.toStdString();
        juce::XmlDocument xmlDoc(juce::File(this->pathCurrentFileSpeaker));
        std::unique_ptr<juce::XmlElement> mainXmlElem(xmlDoc.getDocumentElement());
        if (mainXmlElem == nullptr) {
            juce::AlertWindow alert("Error in Load Speaker Setup !",
                                    "Your file is corrupted !\n" + xmlDoc.getLastParseError(),
                                    juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mGrisFeel);
            alert.addButton("Ok", 0);
            alert.runModalLoop();
        } else {
            if (mainXmlElem->hasTagName("SpeakerSetup")) {
                this->lockSpeakers.lock();
                this->listSpeaker.clear();
                this->lockSpeakers.unlock();
                if (path.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) == 0) {
                    this->jackClient->modeSelected = (ModeSpatEnum)(VBAP_HRTF);
                    this->comBoxModeSpat->setSelectedId(VBAP_HRTF + 1, juce::NotificationType::dontSendNotification);
                } else if (path.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) == 0) {
                    this->jackClient->modeSelected = (ModeSpatEnum)(STEREO);
                    this->comBoxModeSpat->setSelectedId(STEREO + 1, juce::NotificationType::dontSendNotification);
                } else if (!isNewSameAsOld && oldPath.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
                           && oldPath.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
                    int spatMode = mainXmlElem->getIntAttribute("SpatMode");
                    this->jackClient->modeSelected = (ModeSpatEnum)(spatMode);
                    this->comBoxModeSpat->setSelectedId(spatMode + 1, juce::NotificationType::dontSendNotification);
                } else if (!isNewSameAsLastSetup) {
                    int spatMode = mainXmlElem->getIntAttribute("SpatMode");
                    this->jackClient->modeSelected = (ModeSpatEnum)(spatMode);
                    this->comBoxModeSpat->setSelectedId(spatMode + 1, juce::NotificationType::dontSendNotification);
                }

                bool loadSetupFromXYZ = false;
                if (isNewSameAsOld && this->jackClient->modeSelected == LBAP)
                    loadSetupFromXYZ = true;

                this->setNameConfig();
                this->jackClient->processBlockOn = false;
                this->jackClient->clearOutput();
                this->jackClient->maxOutputPatch = 0;
                juce::Array<int> layoutIndexes;
                int maxLayoutIndex = 0;
                forEachXmlChildElement(*mainXmlElem, ring)
                {
                    if (ring->hasTagName("Ring")) {
                        forEachXmlChildElement(*ring, spk)
                        {
                            if (spk->hasTagName("Speaker")) {
                                // Safety against layoutIndex doubles in the speaker setup.
                                int layoutIndex = spk->getIntAttribute("LayoutIndex");
                                if (layoutIndexes.contains(layoutIndex)) {
                                    layoutIndex = ++maxLayoutIndex;
                                }
                                layoutIndexes.add(layoutIndex);
                                if (layoutIndex > maxLayoutIndex) {
                                    maxLayoutIndex = layoutIndex;
                                }

                                this->listSpeaker.add(new Speaker(this,
                                                                  layoutIndex,
                                                                  spk->getIntAttribute("OutputPatch"),
                                                                  spk->getDoubleAttribute("Azimuth"),
                                                                  spk->getDoubleAttribute("Zenith"),
                                                                  spk->getDoubleAttribute("Radius")));
                                if (loadSetupFromXYZ) {
                                    this->listSpeaker.getLast()->setCoordinate(
                                        glm::vec3(spk->getDoubleAttribute("PositionX"),
                                                  spk->getDoubleAttribute("PositionZ"),
                                                  spk->getDoubleAttribute("PositionY")));
                                }
                                if (spk->hasAttribute("Gain")) {
                                    this->listSpeaker.getLast()->setGain(spk->getDoubleAttribute("Gain"));
                                }
                                if (spk->hasAttribute("HighPassCutoff")) {
                                    this->listSpeaker.getLast()->setHighPassCutoff(
                                        spk->getDoubleAttribute("HighPassCutoff"));
                                }
                                if (spk->hasAttribute("DirectOut")) {
                                    this->listSpeaker.getLast()->setDirectOut(spk->getBoolAttribute("DirectOut"));
                                }
                                this->jackClient->addOutput((unsigned int)spk->getIntAttribute("OutputPatch"));
                            }
                        }
                    }
                    if (ring->hasTagName("triplet")) {
                        Triplet tri;
                        tri.id1 = ring->getIntAttribute("id1");
                        tri.id2 = ring->getIntAttribute("id2");
                        tri.id3 = ring->getIntAttribute("id3");
                        this->listTriplet.push_back(tri);
                    }
                }
                this->jackClient->processBlockOn = true;
                ok = true;
            } else {
                if (mainXmlElem->hasTagName("ServerGRIS_Preset")) {
                    msg = "You are trying to open a Server document, and not a Speaker Setup !";
                } else {
                    msg = "Your file is corrupted !\n" + xmlDoc.getLastParseError();
                }
                juce::AlertWindow alert("Error in Load Speaker Setup !", msg, juce::AlertWindow::WarningIcon);
                alert.setLookAndFeel(&mGrisFeel);
                alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
                alert.runModalLoop();
            }
        }
    }
    if (ok) {
        if (this->pathCurrentFileSpeaker.endsWith("default_preset/default_speaker_setup.xml")) {
            this->applicationProperties.getUserSettings()->setValue(
                "lastSpeakerSetupDirectory",
                juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName());
        } else {
            this->applicationProperties.getUserSettings()->setValue(
                "lastSpeakerSetupDirectory",
                juce::File(this->pathCurrentFileSpeaker).getParentDirectory().getFullPathName());
        }
        this->needToComputeVbap = true;
        this->updateLevelComp();
        if (this->getJackClient()->modeSelected != VBAP_HRTF && this->getJackClient()->modeSelected != STEREO) {
            if (this->pathCurrentFileSpeaker.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
                && this->pathCurrentFileSpeaker.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
                this->pathLastVbapSpeakerSetup = this->pathCurrentFileSpeaker;
            }
        }
    } else {
        if (isNewSameAsOld) {
            this->openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
        } else {
            this->openXmlFileSpeaker(oldPath);
        }
    }
}

//==============================================================================
void MainContentComponent::setTitle()
{
    juce::String version = STRING(JUCE_APP_VERSION);
    version = "SpatGRIS v" + version + " - ";
    this->parent.setName(version + juce::File(this->pathCurrentPreset).getFileName());
}

//==============================================================================
void MainContentComponent::openPreset(juce::String path)
{
    juce::String msg;
    this->jackClient->processBlockOn = false;
    juce::File xmlFile = juce::File(path.toStdString());
    juce::XmlDocument xmlDoc(xmlFile);
    std::unique_ptr<juce::XmlElement> mainXmlElem(xmlDoc.getDocumentElement());
    if (mainXmlElem == nullptr) {
        juce::AlertWindow alert("Error in Open Preset !",
                                "Your file is corrupted !\n" + path.toStdString() + "\n"
                                    + xmlDoc.getLastParseError().toStdString(),
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mGrisFeel);
        alert.addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
    } else {
        if (mainXmlElem->hasTagName("SpatServerGRIS_Preset") || mainXmlElem->hasTagName("ServerGRIS_Preset")) {
            this->pathCurrentPreset = path;
            this->oscInputPort
                = mainXmlElem->getIntAttribute("OSC_Input_Port"); // TODO: app preferences instead of project settings ?
            this->tedAddInputs->setText(mainXmlElem->getStringAttribute("Number_Of_Inputs"));
            this->sliderMasterGainOut->setValue(mainXmlElem->getDoubleAttribute("Master_Gain_Out", 0.0),
                                                juce::sendNotification);
            this->sliderInterpolation->setValue(mainXmlElem->getDoubleAttribute("Master_Interpolation", 0.1),
                                                juce::sendNotification);
            this->setShowNumbers(mainXmlElem->getBoolAttribute("Show_Numbers"));
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
                this->speakerView->setCamPosition(80.0f, 25.0f, 22.0f); // TODO: named constants ?
            }

            // Update
            this->textEditorReturnKeyPressed(*this->tedAddInputs);
            this->sliderValueChanged(this->sliderMasterGainOut.get());
            this->sliderValueChanged(this->sliderInterpolation.get());

            juce::File speakerSetup = juce::File(this->pathCurrentFileSpeaker.toStdString());
            if (!this->pathCurrentFileSpeaker.startsWith("/")) {
                this->pathCurrentFileSpeaker
                    = DEFAULT_PRESET_DIRECTORY.getChildFile(this->pathCurrentFileSpeaker).getFullPathName();
            }

            forEachXmlChildElement(*mainXmlElem, input)
            {
                if (input->hasTagName("Input")) {
                    for (auto && it : listSourceInput) {
                        if (it->getId() == input->getIntAttribute("Index")) {
                            it->setColor(juce::Colour::fromFloatRGBA((float)input->getDoubleAttribute("R"),
                                                                     (float)input->getDoubleAttribute("G"),
                                                                     (float)input->getDoubleAttribute("B"),
                                                                     1.0f),
                                         true);
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
            juce::AlertWindow alert("Error in Open Preset !", msg, juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mGrisFeel);
            alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
            alert.runModalLoop();
        }
    }

    this->jackClient->pinkNoiseSound = false;
    this->jackClient->processBlockOn = true;

    if (this->pathCurrentPreset.endsWith("default_preset/default_preset.xml")) {
        this->applicationProperties.getUserSettings()->setValue(
            "lastPresetDirectory",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName());
    } else {
        this->applicationProperties.getUserSettings()->setValue(
            "lastPresetDirectory",
            juce::File(this->pathCurrentPreset).getParentDirectory().getFullPathName());
    }
    this->setTitle();
}

//==============================================================================
void MainContentComponent::getPresetData(juce::XmlElement * xml) const
{
    xml->setAttribute("OSC_Input_Port", juce::String(this->oscInputPort));
    xml->setAttribute("Number_Of_Inputs", this->tedAddInputs->getTextValue().toString());
    xml->setAttribute("Master_Gain_Out", this->sliderMasterGainOut->getValue());
    xml->setAttribute("Master_Interpolation", this->sliderInterpolation->getValue());
    xml->setAttribute("Show_Numbers", this->isNumbersShown);
    xml->setAttribute("Show_Speakers", this->isSpeakersShown);
    xml->setAttribute("Show_Triplets", this->isTripletsShown);
    xml->setAttribute("Use_Alpha", this->isSourceLevelShown);
    xml->setAttribute("Show_Speaker_Level", this->isSpeakerLevelShown);
    xml->setAttribute("Show_Sphere", this->isSphereShown);
    xml->setAttribute("CamAngleX", this->speakerView->getCamAngleX());
    xml->setAttribute("CamAngleY", this->speakerView->getCamAngleY());
    xml->setAttribute("CamDistance", this->speakerView->getCamDistance());

    for (auto const it : listSourceInput) {
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
    this->getPresetData(xml.get());
    xml->writeTo(xmlFile);
    xmlFile.create();
    this->pathCurrentPreset = path;
    this->applicationProperties.getUserSettings()->setValue(
        "lastPresetDirectory",
        juce::File(this->pathCurrentPreset).getParentDirectory().getFullPathName());
    this->setTitle();
}

//==============================================================================
void MainContentComponent::saveSpeakerSetup(juce::String path)
{
    this->pathCurrentFileSpeaker = path;
    juce::File xmlFile = juce::File(path.toStdString());
    juce::XmlElement xml{ "SpeakerSetup" };

    xml.setAttribute("Name", this->nameConfig);
    xml.setAttribute("Dimension", 3);
    xml.setAttribute("SpatMode", getModeSelected());

    juce::XmlElement * xmlRing{ new juce::XmlElement("Ring") };

    for (auto const & it : listSpeaker) {
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

    for (auto const & it : listTriplet) {
        juce::XmlElement * xmlInput{ new juce::XmlElement{ "triplet" } };
        xmlInput->setAttribute("id1", it.id1);
        xmlInput->setAttribute("id2", it.id2);
        xmlInput->setAttribute("id3", it.id3);
        xml.addChildElement(xmlInput);
    }

    xml.writeTo(xmlFile);
    xmlFile.create();

    this->applicationProperties.getUserSettings()->setValue(
        "lastSpeakerSetupDirectory",
        juce::File(this->pathCurrentFileSpeaker).getParentDirectory().getFullPathName());

    this->needToSaveSpeakerSetup = false;

    if (this->getJackClient()->modeSelected != VBAP_HRTF && this->getJackClient()->modeSelected != STEREO) {
        if (this->pathCurrentFileSpeaker.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
            && this->pathCurrentFileSpeaker.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
            this->pathLastVbapSpeakerSetup = this->pathCurrentFileSpeaker;
        }
    }

    this->setNameConfig();
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
    juce::PropertiesFile * props = this->applicationProperties.getUserSettings();

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
        alert.setLookAndFeel(&mGrisFeel);
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
        this->oscInputPort = oscPort;
        props->setValue("OscInputPort", oscPort);
        this->oscReceiver->closeConnection();
        if (this->oscReceiver->startConnection(this->oscInputPort)) {
            std::cout << "OSC receiver connected to port " << oscPort << '\n';
        } else {
            std::cout << "OSC receiver connection to port " << oscPort << " failed... Should popup an alert window."
                      << '\n';
        }
    }

    // Handle recording settings
    this->jackClient->setRecordFormat(fileformat);
    props->setValue("FileFormat", fileformat);

    this->jackClient->setRecordFileConfig(fileconfig);
    props->setValue("FileConfig", fileconfig);

    // Handle CUBE distance attenuation
    float linGain = powf(10.0f, AttenuationDBs[attenuationDB].getFloatValue() * 0.05f);
    this->jackClient->setAttenuationDB(linGain);
    props->setValue("AttenuationDB", attenuationDB);

    float coeff = expf(-M2_PI * AttenuationCutoffs[attenuationHz].getFloatValue() / this->jackClient->sampleRate);
    this->jackClient->setAttenuationHz(coeff);
    props->setValue("AttenuationHz", attenuationHz);

    applicationProperties.saveIfNeeded();
}

//==============================================================================
void MainContentComponent::timerCallback()
{
    this->labelJackLoad->setText(juce::String(this->jackClient->getCpuUsed(), 4) + " %", juce::dontSendNotification);
    int seconds = this->jackClient->indexRecord / this->jackClient->sampleRate;
    int minute = int(seconds / 60) % 60;
    seconds = int(seconds % 60);
    juce::String timeRecorded = ((minute < 10) ? "0" + juce::String(minute) : juce::String(minute)) + " : "
                                + ((seconds < 10) ? "0" + juce::String(seconds) : juce::String(seconds));
    this->labelTimeRecorded->setText(timeRecorded, juce::dontSendNotification);

    if (this->butStartRecord->getToggleState()) {
        this->butStartRecord->setToggleState(false, juce::dontSendNotification);
    }

    if (this->jackClient->isSavingRun()) {
        this->butStartRecord->setButtonText("Stop");
    } else if (this->jackClient->getRecordingPath() == "") {
        this->butStartRecord->setButtonText("Record");
    } else {
        this->butStartRecord->setButtonText("Record");
    }

    if (this->isRecording && !this->jackClient->recording) {
        bool isReadyToMerge = true;
        for (unsigned int i = 0; i < MaxOutputs; i++) {
            if (this->jackClient->recorder[i].backgroundThread.isThreadRunning()) {
                isReadyToMerge = false;
            }
        }
        if (isReadyToMerge) {
            this->isRecording = false;
            if (this->jackClient->getRecordFileConfig()) {
                this->jackClient->processBlockOn = false;
                AudioRenderer * renderer = new AudioRenderer();
                renderer->prepareRecording(this->jackClient->getRecordingPath(),
                                           this->jackClient->outputFilenames,
                                           this->jackClient->sampleRate);
                renderer->runThread();
                this->jackClient->processBlockOn = true;
            }
        }
    }

    if (this->jackClient->overload) {
        this->labelJackLoad->setColour(juce::Label::backgroundColourId, juce::Colours::darkred);
    } else {
        this->labelJackLoad->setColour(juce::Label::backgroundColourId, mGrisFeel.getWinBackgroundColour());
    }

    for (auto && it : listSourceInput) {
        it->getVuMeter()->update();
    }

    for (auto && it : listSpeaker) {
        it->getVuMeter()->update();
    }

    this->jackClientListComponentJack->updateContentCli();

    if (this->isProcessForeground != juce::Process::isForegroundProcess()) {
        this->isProcessForeground = juce::Process::isForegroundProcess();
        if (this->winSpeakConfig != nullptr && this->isProcessForeground) {
            this->winSpeakConfig->setVisible(true);
            this->winSpeakConfig->setAlwaysOnTop(true);
        } else if (this->winSpeakConfig != nullptr && !this->isProcessForeground) {
            this->winSpeakConfig->setVisible(false);
            this->winSpeakConfig->setAlwaysOnTop(false);
        }
        if (this->flatViewWindowSource != nullptr && this->isProcessForeground) {
            this->flatViewWindowSource->toFront(false);
            this->toFront(true);
        }
    }
}

//==============================================================================
void MainContentComponent::paint(juce::Graphics & g)
{
    g.fillAll(mGrisFeel.getWinBackgroundColour());
}

//==============================================================================
void MainContentComponent::textEditorFocusLost(juce::TextEditor & textEditor)
{
    textEditorReturnKeyPressed(textEditor);
}

//==============================================================================
void MainContentComponent::textEditorReturnKeyPressed(juce::TextEditor & textEditor)
{
    if (&textEditor == this->tedAddInputs.get()) {
        unsigned int num_of_inputs = (unsigned int)this->tedAddInputs->getTextValue().toString().getIntValue();
        if (num_of_inputs < 1) {
            this->tedAddInputs->setText("1");
        }
        if (num_of_inputs > MaxInputs) {
            this->tedAddInputs->setText(juce::String(MaxInputs));
        }

        if (this->jackClient->inputsPort.size() != num_of_inputs) {
            this->jackClient->processBlockOn = false;
            this->jackClient->addRemoveInput(num_of_inputs);
            this->jackClient->processBlockOn = true;

            this->lockInputs.lock();
            bool addInput = false;
            for (unsigned int i = 0; i < this->jackClient->inputsPort.size(); i++) {
                if (i >= this->listSourceInput.size()) {
                    this->listSourceInput.add(new Input(*this, mSmallTextGrisFeel, i + 1));
                    addInput = true;
                }
            }
            if (!addInput) {
                auto const listSourceInputSize{ this->listSourceInput.size() };
                auto const jackClientInputPortSize{ this->jackClient->inputsPort.size() };
                if (listSourceInputSize > jackClientInputPortSize) {
                    this->listSourceInput.removeRange(jackClientInputPortSize,
                                                      listSourceInputSize - jackClientInputPortSize);
                }
            }
            this->lockInputs.unlock();
        }
        this->unfocusAllComponents();
        updateLevelComp();
    }
}

//==============================================================================
void MainContentComponent::buttonClicked(juce::Button * button)
{
    if (button == this->butStartRecord.get()) {
        if (this->jackClient->recording) {
            this->jackClient->stopRecord();
            this->butStartRecord->setEnabled(false);
            this->labelTimeRecorded->setColour(juce::Label::textColourId, mGrisFeel.getFontColour());
        } else {
            this->isRecording = true;
            this->jackClient->startRecord();
            this->labelTimeRecorded->setColour(juce::Label::textColourId, mGrisFeel.getRedColour());
        }
        this->butStartRecord->setToggleState(this->jackClient->recording, juce::dontSendNotification);
    } else if (button == this->butInitRecord.get()) {
        this->chooseRecordingPath();
        this->butStartRecord->setEnabled(true);
    }
}

//==============================================================================
void MainContentComponent::sliderValueChanged(juce::Slider * slider)
{
    if (slider == this->sliderMasterGainOut.get()) {
        this->jackClient->masterGainOut = pow(10.0, this->sliderMasterGainOut->getValue() * 0.05);
    }

    else if (slider == this->sliderInterpolation.get()) {
        this->jackClient->interMaster = this->sliderInterpolation->getValue();
    }
}

//==============================================================================
void MainContentComponent::comboBoxChanged(juce::ComboBox * comboBox)
{
    if (this->winSpeakConfig != nullptr && this->needToSaveSpeakerSetup) {
        juce::AlertWindow alert(
            "The speaker configuration has changed!    ",
            "Save your changes or close the speaker configuration window before switching mode...    ",
            juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mGrisFeel);
        alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        this->comBoxModeSpat->setSelectedId(this->jackClient->modeSelected + 1,
                                            juce::NotificationType::dontSendNotification);
        return;
    }

    if (this->comBoxModeSpat.get() == comboBox) {
        this->jackClient->processBlockOn = false;
        this->jackClient->modeSelected = (ModeSpatEnum)(this->comBoxModeSpat->getSelectedId() - 1);
        switch (this->jackClient->modeSelected) {
        case VBAP:
            this->openXmlFileSpeaker(this->pathLastVbapSpeakerSetup);
            this->needToSaveSpeakerSetup = false;
            this->isSpanShown = true;
            break;
        case LBAP:
            this->openXmlFileSpeaker(this->pathLastVbapSpeakerSetup);
            this->needToSaveSpeakerSetup = false;
            this->isSpanShown = true;
            break;
        case VBAP_HRTF:
            this->openXmlFileSpeaker(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName());
            this->needToSaveSpeakerSetup = false;
            this->jackClient->resetHRTF();
            this->isSpanShown = false;
            break;
        case STEREO:
            this->openXmlFileSpeaker(STEREO_SPEAKER_SETUP_FILE.getFullPathName());
            this->needToSaveSpeakerSetup = false;
            this->isSpanShown = false;
            break;
        default:
            break;
        }
        this->jackClient->processBlockOn = true;

        if (this->winSpeakConfig != nullptr) {
            juce::String windowName = juce::String("Speakers Setup Edition - ")
                                      + juce::String(ModeSpatString[this->jackClient->modeSelected])
                                      + juce::String(" - ") + juce::File(this->pathCurrentFileSpeaker).getFileName();
            this->winSpeakConfig->setName(windowName);
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
    if (this->oscLogWindow != nullptr) {
        juce::String address = message.getAddressPattern().toString();
        this->oscLogWindow->addToLog(address + "\n");
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
        this->oscLogWindow->addToLog(msg + "\n");
    }
}

//==============================================================================
void MainContentComponent::chooseRecordingPath()
{
    juce::String dir = this->applicationProperties.getUserSettings()->getValue("lastRecordingDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String extF;
    juce::String extChoice;
    if (this->jackClient->getRecordFormat() == 0) {
        extF = ".wav";
        extChoice = "*.wav,*.aif";
    } else {
        extF = ".aif";
        extChoice = "*.aif,*.wav";
    }

    juce::FileChooser fc("Choose a file to save...", dir + "/recording" + extF, extChoice, USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToSave(true)) {
        juce::String filePath = fc.getResults().getReference(0).getFullPathName();
        this->applicationProperties.getUserSettings()->setValue(
            "lastRecordingDirectory",
            juce::File(filePath).getParentDirectory().getFullPathName());
        this->jackClient->setRecordingPath(filePath);
    }
    this->jackClient->prepareToRecord();
}

//==============================================================================
void MainContentComponent::resized()
{
    juce::Rectangle<int> r(getLocalBounds().reduced(2));

    menuBar->setBounds(0, 0, this->getWidth(), 20);
    r.removeFromTop(20);

    // Lay out the speaker view and the vertical divider.
    Component * vcomps[] = { this->speakerView.get(), this->verticalDividerBar.get(), nullptr };

    // Lay out side-by-side and resize the components' heights as well as widths.
    this->verticalLayout.layOutComponents(vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);

    this->boxMainUI->setBounds(this->speakerView->getWidth() + 6,
                               20,
                               getWidth() - (this->speakerView->getWidth() + 10),
                               getHeight());
    this->boxMainUI->correctSize(getWidth() - this->speakerView->getWidth() - 6, 610);

    this->boxInputsUI->setBounds(0, 2, getWidth() - (this->speakerView->getWidth() + 10), 231);
    this->boxInputsUI->correctSize(((unsigned int)this->listSourceInput.size() * (VuMeterWidthInPixels)) + 4, 200);

    this->boxOutputsUI->setBounds(0, 233, getWidth() - (this->speakerView->getWidth() + 10), 210);
    this->boxOutputsUI->correctSize(((unsigned int)this->listSpeaker.size() * (VuMeterWidthInPixels)) + 4, 180);

    this->boxControlUI->setBounds(0, 443, getWidth() - (this->speakerView->getWidth() + 10), 145);
    this->boxControlUI->correctSize(720, 145);
}
