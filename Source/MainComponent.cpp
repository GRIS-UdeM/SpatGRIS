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
MainContentComponent::MainContentComponent(MainWindow & parent) : mMainWindow(parent)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&mLookAndFeel);

    // Create the menubar.
    mMenuBar.reset(new juce::MenuBarComponent(this));
    this->addAndMakeVisible(mMenuBar.get());

    // Start the Splash screen.
    juce::File fs = juce::File(SPLASH_SCREEN_FILE);
    if (fs.exists()) {
        this->mSplashScreen.reset(new juce::SplashScreen("SpatGRIS2", juce::ImageFileFormat::loadFrom(fs), true));
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
    this->mApplicationProperties.setStorageParameters(options);

    juce::PropertiesFile * props = this->mApplicationProperties.getUserSettings();

    // Initialize class variables.
    this->mIsProcessForeground = true;
    this->mIsNumbersShown = false;
    this->mIsSpeakersShown = true;
    this->mIsTripletsShown = false;
    this->mIsSourceLevelShown = false;
    this->mIsSphereShown = false;
    this->mIsSpanShown = true;
    this->mIsRecording = false;

    // Get a reference to the last opened VBAP speaker setup.
    juce::File lastVbap = juce::File(props->getValue("lastVbapSpeakerSetup", "./not_saved_yet"));
    if (!lastVbap.existsAsFile()) {
        this->mPathLastVbapSpeakerSetup = DEFAULT_SPEAKER_SETUP_FILE.getFullPathName();
    } else {
        this->mPathLastVbapSpeakerSetup = props->getValue("lastVbapSpeakerSetup");
    }

    this->mEditSpeakersWindow = nullptr;
    this->mPropertiesWindow = nullptr;
    this->mFlatViewWindow = nullptr;
    this->mAboutWindow = nullptr;
    this->mOscLogWindow = nullptr;

    // SpeakerViewComponent 3D view
    this->mSpeakerViewComponent.reset(new SpeakerViewComponent(*this));
    addAndMakeVisible(this->mSpeakerViewComponent.get());

    // Box Main
    this->mMainUiBox.reset(new Box(mLookAndFeel, "", true, false));
    addAndMakeVisible(this->mMainUiBox.get());

    // Box Inputs
    this->mInputsUiBox.reset(new Box(mLookAndFeel, "Inputs"));
    addAndMakeVisible(this->mInputsUiBox.get());

    // Box Outputs
    this->mOutputsUiBox.reset(new Box(mLookAndFeel, "Outputs"));
    addAndMakeVisible(this->mOutputsUiBox.get());

    // Box Control
    this->mControlUiBox.reset(new Box(mLookAndFeel));
    addAndMakeVisible(this->mControlUiBox.get());

    this->mMainUiBox->getContent()->addAndMakeVisible(this->mInputsUiBox.get());
    this->mMainUiBox->getContent()->addAndMakeVisible(this->mOutputsUiBox.get());
    this->mMainUiBox->getContent()->addAndMakeVisible(this->mControlUiBox.get());

    // Components in Box Control
    this->mJackStatusLabel.reset(
        addLabel("Jack Unknown", "Jack Status", 0, 0, 80, 28, this->mControlUiBox->getContent()));
    this->mJackLoadLabel.reset(
        addLabel("0.000000 %", "Load Jack CPU", 80, 0, 80, 28, this->mControlUiBox->getContent()));
    this->mJackRateLabel.reset(addLabel("00000 Hz", "Rate", 160, 0, 80, 28, this->mControlUiBox->getContent()));
    this->mJackBufferLabel.reset(
        addLabel("0000 spls", "Buffer Size", 240, 0, 80, 28, this->mControlUiBox->getContent()));
    this->mJackInfoLabel.reset(
        addLabel("...", "Jack Inputs/Outputs system", 320, 0, 90, 28, this->mControlUiBox->getContent()));

    this->mJackStatusLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    this->mJackLoadLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    this->mJackRateLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    this->mJackBufferLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    this->mJackInfoLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());

    addLabel("Gain", "Master Gain Outputs", 15, 30, 120, 20, this->mControlUiBox->getContent());
    this->mMasterGainOutSlider.reset(
        addSlider("Master Gain", "Master Gain Outputs", 5, 45, 60, 60, this->mControlUiBox->getContent()));
    this->mMasterGainOutSlider->setRange(-60.0, 12.0, 0.01);
    this->mMasterGainOutSlider->setTextValueSuffix(" dB");

    addLabel("Interpolation", "Master Interpolation", 60, 30, 120, 20, this->mControlUiBox->getContent());
    this->mInterpolationSlider.reset(
        addSlider("Inter", "Interpolation", 70, 45, 60, 60, this->mControlUiBox->getContent()));
    this->mInterpolationSlider->setRange(0.0, 1.0, 0.001);

    addLabel("Mode :", "Mode of spatilization", 150, 30, 60, 20, this->mControlUiBox->getContent());
    this->mModeSpatCombo.reset(
        addComboBox("", "Mode of spatilization", 155, 48, 90, 22, this->mControlUiBox->getContent()));
    for (int i = 0; i < ModeSpatString.size(); i++) {
        this->mModeSpatCombo->addItem(ModeSpatString[i], i + 1);
    }

    this->mAddInputsTextEditor.reset(
        addTextEditor("Inputs :", "0", "Numbers of Inputs", 122, 83, 43, 22, this->mControlUiBox->getContent()));
    this->mAddInputsTextEditor->setInputRestrictions(3, "0123456789");

    this->mInitRecordButton.reset(
        addButton("Init Recording", "Init Recording", 268, 48, 103, 24, this->mControlUiBox->getContent()));

    this->mStartRecordButton.reset(
        addButton("Record", "Start/Stop Record", 268, 83, 60, 24, this->mControlUiBox->getContent()));
    this->mStartRecordButton->setEnabled(false);

    this->mTimeRecordedLabel.reset(
        addLabel("00:00", "Record time", 327, 83, 50, 24, this->mControlUiBox->getContent()));

    // Jack client box.
    this->mJackClientListComponent.reset(new JackClientListComponent(this, &mLookAndFeel));
    this->mJackClientListComponent->setBounds(410, 0, 304, 138);
    this->mControlUiBox->getContent()->addAndMakeVisible(this->mJackClientListComponent.get());

    // Set up the layout and resizer bars.
    this->mVerticalLayout.setItemLayout(
        0,
        -0.2,
        -0.8,
        -0.435); // width of the speaker view must be between 20% and 80%, preferably 50%
    this->mVerticalLayout.setItemLayout(1, 8, 8, 8); // the vertical divider drag-bar thing is always 8 pixels wide
    this->mVerticalLayout.setItemLayout(
        2,
        150,
        -1.0,
        -0.565); // right panes must be at least 150 pixels wide, preferably 50% of the total width
    this->mVerticalDividerBar.reset(new juce::StretchableLayoutResizerBar(&mVerticalLayout, 1, true));
    this->addAndMakeVisible(mVerticalDividerBar.get());

    // Default application window size.
    this->setSize(1285, 610);

    // Jack Initialization parameters.
    unsigned int BufferValue = props->getIntValue("BufferValue", 1024);
    unsigned int RateValue = props->getIntValue("RateValue", 48000);

    if (std::isnan(float(BufferValue)) || BufferValue == 0 || std::isnan(float(RateValue)) || RateValue == 0) {
        BufferValue = 1024;
        RateValue = 48000;
    }

    this->mSamplingRate = RateValue;

    // Start Jack Server and client.
    int errorCode = 0;
    mAlsaOutputDevice = props->getValue("AlsaOutputDevice", "");
    this->jackServer.reset(new JackServerGris(RateValue, BufferValue, mAlsaOutputDevice, &errorCode));
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

    mAlsaAvailableOutputDevices = this->jackServer->getAvailableOutputDevices();

    unsigned int fileformat = props->getIntValue("FileFormat", 0);
    this->jackClient->setRecordFormat(fileformat);
    unsigned int fileconfig = props->getIntValue("FileConfig", 0);
    this->jackClient->setRecordFileConfig(fileconfig);

    if (!jackClient->isReady()) {
        this->mJackStatusLabel->setText("Jack ERROR", juce::dontSendNotification);
    } else {
        this->mJackStatusLabel->setText("Jack Run", juce::dontSendNotification);
    }

    this->mJackRateLabel->setText(juce::String(this->jackClient->getSampleRate()) + " Hz", juce::dontSendNotification);
    this->mJackBufferLabel->setText(juce::String(this->jackClient->getBufferSize()) + " spls",
                                    juce::dontSendNotification);
    this->mJackInfoLabel->setText("I : " + juce::String(this->jackClient->getNumberOutputs())
                                      + " - O : " + juce::String(this->jackClient->getNumberInputs()),
                                  juce::dontSendNotification);

    // Start the OSC Receiver.
    this->mOscReceiver.reset(new OscInput(*this));
    this->mOscReceiver->startConnection(this->mOscInputPort);

    // Default widget values.
    this->mMasterGainOutSlider->setValue(0.0);
    this->mInterpolationSlider->setValue(0.1);
    this->mModeSpatCombo->setSelectedId(1);

    this->mAddInputsTextEditor->setText("16", juce::dontSendNotification);
    textEditorReturnKeyPressed(*this->mAddInputsTextEditor);

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
    if (this->mSplashScreen) {
        this->mSplashScreen->deleteAfterDelay(juce::RelativeTime::seconds(4), false);
        this->mSplashScreen.release();
    }

    // Initialize the command manager for the menubar items.
    juce::ApplicationCommandManager & commandManager = this->mMainWindow.getApplicationCommandManager();
    commandManager.registerAllCommandsForTarget(this);

    // Restore last vertical divider position and speaker view cam distance.
    if (props->containsKey("sashPosition")) {
        int trueSize = (int)round((this->getWidth() - 3) * abs(props->getDoubleValue("sashPosition")));
        this->mVerticalLayout.setItemPosition(1, trueSize);
    }
}

//==============================================================================
MainContentComponent::~MainContentComponent()
{
    juce::PropertiesFile * props = this->mApplicationProperties.getUserSettings();
    props->setValue("lastOpenPreset", this->mPathCurrentPreset);
    props->setValue("lastOpenSpeakerSetup", this->mPathCurrentFileSpeaker);
    props->setValue("lastVbapSpeakerSetup", this->mPathLastVbapSpeakerSetup);
    props->setValue("sashPosition", this->mVerticalLayout.getItemCurrentRelativeSize(0));
    this->mApplicationProperties.saveIfNeeded();
    this->mApplicationProperties.closeFiles();

    this->mSpeakerLocks.lock();
    this->mSpeakers.clear();
    this->mSpeakerLocks.unlock();

    this->mInputLocks.lock();
    this->mSourceInputs.clear();
    this->mInputLocks.unlock();
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
    sd->setRotaryParameters(M_PI * 1.3f, M_PI * 2.7f, true);
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
        this->handleSavePreset();
    } else if (status == 0) {
        return;
    }

    this->openPreset(DEFAULT_PRESET_FILE.getFullPathName());
}

//==============================================================================
void MainContentComponent::handleOpenPreset()
{
    juce::String dir = this->mApplicationProperties.getUserSettings()->getValue("lastPresetDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(this->mPathCurrentPreset).getFileName();

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
            this->openPreset(chosen);
            loaded = true;
        }
    }

    if (loaded) { // Check for direct out OutputPatch mismatch.
        for (auto && it : mSourceInputs) {
            if (it->getDirectOutChannel() != 0) {
                std::vector<int> directOutOutputPatches = this->jackClient->getDirectOutOutputPatches();
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
    if (!juce::File(this->mPathCurrentPreset).existsAsFile()
        || this->mPathCurrentPreset.endsWith("default_preset/default_preset.xml")) {
        this->handleSaveAsPreset();
    }
    this->savePreset(this->mPathCurrentPreset);
}

//==============================================================================
void MainContentComponent::handleSaveAsPreset()
{
    juce::String dir = this->mApplicationProperties.getUserSettings()->getValue("lastPresetDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(this->mPathCurrentPreset).getFileName();

    juce::FileChooser fc("Choose a file to save...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToSave(true)) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        this->savePreset(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleOpenSpeakerSetup()
{
    juce::String dir = this->mApplicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory");
    if (!juce::File(dir).isDirectory()) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(this->mPathCurrentFileSpeaker).getFileName();

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
            this->openXmlFileSpeaker(chosen);
        }
    }
}

//==============================================================================
void MainContentComponent::handleSaveAsSpeakerSetup()
{
    juce::String dir = this->mApplicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory");
    if (!juce::File(dir).isDirectory() || dir.endsWith("/default_preset")) {
        dir = juce::File("~").getFullPathName();
    }
    juce::String filename = juce::File(this->mPathCurrentFileSpeaker).getFileName();

    juce::FileChooser fc("Choose a file to save...", dir + "/" + filename, "*.xml", USE_OS_NATIVE_DIALOG_BOX);

    if (fc.browseForFileToSave(true)) {
        juce::String chosen = fc.getResults().getReference(0).getFullPathName();
        this->saveSpeakerSetup(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleShowSpeakerEditWindow()
{
    juce::Rectangle<int> result(this->getScreenX() + this->mSpeakerViewComponent->getWidth() + 20,
                                this->getScreenY() + 20,
                                850,
                                600);
    if (this->mEditSpeakersWindow == nullptr) {
        juce::String windowName = juce::String("Speakers Setup Edition - ")
                                  + juce::String(ModeSpatString[this->jackClient->getMode()]) + juce::String(" - ")
                                  + juce::File(this->mPathCurrentFileSpeaker).getFileName();
        this->mEditSpeakersWindow.reset(
            new EditSpeakersWindow(windowName, this->mLookAndFeel, *this, this->mNameConfig));
        this->mEditSpeakersWindow->setBounds(result);
        this->mEditSpeakersWindow->initComp();
    }
    this->mEditSpeakersWindow->setBounds(result);
    this->mEditSpeakersWindow->setResizable(true, true);
    this->mEditSpeakersWindow->setUsingNativeTitleBar(true);
    this->mEditSpeakersWindow->setVisible(true);
    this->mEditSpeakersWindow->setAlwaysOnTop(true);
    this->mEditSpeakersWindow->repaint();
}

//==============================================================================
void MainContentComponent::handleShowPreferences()
{
    juce::PropertiesFile * props = this->mApplicationProperties.getUserSettings();
    if (this->mPropertiesWindow == nullptr) {
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
        this->mPropertiesWindow.reset(new PropertiesWindow{ *this,
                                                            this->mLookAndFeel,
                                                            mAlsaAvailableOutputDevices,
                                                            mAlsaOutputDevice,
                                                            RateValues.indexOf(juce::String(RateValue)),
                                                            BufferSizes.indexOf(juce::String(BufferValue)),
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
    juce::Rectangle<int> result(this->getScreenX() + (this->mSpeakerViewComponent->getWidth() / 2) - 150,
                                this->getScreenY() + (this->mSpeakerViewComponent->getHeight() / 2) - 75,
                                270,
                                height);
    this->mPropertiesWindow->setBounds(result);
    this->mPropertiesWindow->setResizable(false, false);
    this->mPropertiesWindow->setUsingNativeTitleBar(true);
    this->mPropertiesWindow->setVisible(true);
    this->mPropertiesWindow->repaint();
}

//==============================================================================
void MainContentComponent::handleShow2DView()
{
    if (this->mFlatViewWindow == nullptr) {
        this->mFlatViewWindow.reset(new FlatViewWindow{ *this, this->mLookAndFeel });
    } else {
        this->mFlatViewWindowRect.setBounds(this->mFlatViewWindow->getScreenX(),
                                            this->mFlatViewWindow->getScreenY(),
                                            this->mFlatViewWindow->getWidth(),
                                            this->mFlatViewWindow->getHeight());
    }

    if (this->mFlatViewWindowRect.getWidth() == 0) {
        this->mFlatViewWindowRect.setBounds(this->getScreenX() + this->mSpeakerViewComponent->getWidth() + 22,
                                            this->getScreenY() + 100,
                                            500,
                                            500);
    }

    this->mFlatViewWindow->setBounds(this->mFlatViewWindowRect);
    this->mFlatViewWindow->setResizable(true, true);
    this->mFlatViewWindow->setUsingNativeTitleBar(true);
    this->mFlatViewWindow->setVisible(true);
}

//==============================================================================
void MainContentComponent::handleShowOscLogView()
{
    if (this->mOscLogWindow == nullptr) {
        this->mOscLogWindow.reset(new OscLogWindow("OSC Logging Windows",
                                                   this->mLookAndFeel.getWinBackgroundColour(),
                                                   juce::DocumentWindow::allButtons,
                                                   this,
                                                   &this->mLookAndFeel));
    }
    this->mOscLogWindow->centreWithSize(500, 500);
    this->mOscLogWindow->setResizable(false, false);
    this->mOscLogWindow->setUsingNativeTitleBar(true);
    this->mOscLogWindow->setVisible(true);
    this->mOscLogWindow->repaint();
}

//==============================================================================
void MainContentComponent::handleShowAbout()
{
    if (this->mAboutWindow.get() == nullptr) {
        this->mAboutWindow.reset(new AboutWindow{ "About SpatGRIS", this->mLookAndFeel, *this });
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
    this->setShowNumbers(!this->mIsNumbersShown);
}

//==============================================================================
void MainContentComponent::setShowNumbers(bool state)
{
    this->mIsNumbersShown = state;
    this->mSpeakerViewComponent->setShowNumber(state);
}

//==============================================================================
void MainContentComponent::handleShowSpeakers()
{
    this->setShowSpeakers(!this->mIsSpeakersShown);
}

//==============================================================================
void MainContentComponent::setShowSpeakers(bool state)
{
    this->mIsSpeakersShown = state;
    this->mSpeakerViewComponent->setHideSpeaker(!state);
}

//==============================================================================
void MainContentComponent::handleShowTriplets()
{
    this->setShowTriplets(!this->mIsTripletsShown);
}

//==============================================================================
void MainContentComponent::setShowTriplets(bool state)
{
    if (this->getModeSelected() == LBAP && state == true) {
        juce::AlertWindow alert("Can't draw triplets !",
                                "Triplets are not effective with the CUBE mode.",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Close", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        this->setShowTriplets(false);
    } else if (this->validateShowTriplets() || state == false) {
        this->mIsTripletsShown = state;
        this->mSpeakerViewComponent->setShowTriplets(state);
    } else {
        juce::AlertWindow alert("Can't draw all triplets !",
                                "Maybe you didn't compute your current speaker setup ?",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mLookAndFeel);
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
    this->mIsSourceLevelShown = !this->mIsSourceLevelShown;
}

//==============================================================================
void MainContentComponent::handleShowSpeakerLevel()
{
    this->mIsSpeakerLevelShown = !this->mIsSpeakerLevelShown;
}

//==============================================================================
void MainContentComponent::handleShowSphere()
{
    this->mIsSphereShown = !this->mIsSphereShown;
    this->mSpeakerViewComponent->setShowSphere(this->mIsSphereShown);
}

//==============================================================================
void MainContentComponent::handleResetInputPositions()
{
    for (auto && it : this->mSourceInputs) {
        it->resetPosition();
    }
}

//==============================================================================
void MainContentComponent::handleResetMeterClipping()
{
    for (auto && it : this->mSourceInputs) {
        it->getVuMeter()->resetClipping();
    }
    for (auto && it : this->mSpeakers) {
        it->getVuMeter()->resetClipping();
    }
}

//==============================================================================
void MainContentComponent::handleInputColours()
{
    float hue = 0.0f;
    float inc = 1.0 / (this->mSourceInputs.size() + 1);
    for (auto && it : this->mSourceInputs) {
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
        result.setTicked(this->mIsNumbersShown);
        break;
    case MainWindow::ShowSpeakersID:
        result.setInfo("Show Speakers", "Show speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::altModifier);
        result.setTicked(this->mIsSpeakersShown);
        break;
    case MainWindow::ShowTripletsID:
        result.setInfo("Show Speaker Triplets", "Show speaker triplets on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('T', juce::ModifierKeys::altModifier);
        result.setTicked(this->mIsTripletsShown);
        break;
    case MainWindow::ShowSourceLevelID:
        result.setInfo("Show Source Activity", "Activate brightness on sources on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('A', juce::ModifierKeys::altModifier);
        result.setTicked(this->mIsSourceLevelShown);
        break;
    case MainWindow::ShowSpeakerLevelID:
        result.setInfo("Show Speaker Level", "Activate brightness on speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('L', juce::ModifierKeys::altModifier);
        result.setTicked(this->mIsSpeakerLevelShown);
        break;
    case MainWindow::ShowSphereID:
        result.setInfo("Show Sphere/Cube", "Show the sphere on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::altModifier);
        result.setTicked(this->mIsSphereShown);
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
            dynamic_cast<MainWindow *>(&this->mMainWindow)->closeButtonPressed();
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
    juce::ApplicationCommandManager * commandManager = &this->mMainWindow.getApplicationCommandManager();

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
        if (this->jackClient->getVbapDimensions() == 3) {
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
    juce::File xmlFile = juce::File(this->mPathCurrentPreset.toStdString());
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
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert.addButton("Exit", 2, juce::KeyPress(juce::KeyPress::deleteKey));
        exitV = alert.runModalLoop();
        if (exitV == 1) {
            alert.setVisible(false);
            juce::ModalComponentManager::getInstance()->cancelAllModalComponents();
            juce::String dir = this->mApplicationProperties.getUserSettings()->getValue("lastPresetDirectory");
            if (!juce::File(dir).isDirectory()) {
                dir = juce::File("~").getFullPathName();
            }
            juce::String filename = juce::File(this->mPathCurrentPreset).getFileName();

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
    for (auto const & cli : this->jackClient->getClients()) {
        if (cli.portEnd > maxport) {
            maxport = cli.portEnd;
        }
    }
    if (maxport > static_cast<unsigned int>(this->mAddInputsTextEditor->getTextValue().toString().getIntValue())) {
        this->mAddInputsTextEditor->setText(juce::String(maxport), juce::dontSendNotification);
        textEditorReturnKeyPressed(*this->mAddInputsTextEditor);
    }
    this->jackClient->setProcessBlockOn(false);
    this->jackClient->connectionClient(nameCli, conn);
    this->jackClient->setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::selectSpeaker(unsigned int idS)
{
    for (unsigned int i = 0; i < this->mSpeakers.size(); ++i) {
        if (i != idS) {
            this->mSpeakers[i]->unSelectSpeaker();
        } else {
            this->mSpeakers[i]->selectSpeaker();
        }
    }
    if (this->mEditSpeakersWindow != nullptr) {
        this->mEditSpeakersWindow->selectedRow(idS);
    }
}

//==============================================================================
void MainContentComponent::selectTripletSpeaker(int idS)
{
    int countS = 0;
    for (unsigned int i = 0; i < this->mSpeakers.size(); ++i) {
        if (this->mSpeakers[i]->isSelected()) {
            countS += 1;
        }
    }

    if (!this->mSpeakers[idS]->isSelected() && countS < 3) {
        this->mSpeakers[idS]->selectSpeaker();
        countS += 1;
    } else {
        this->mSpeakers[idS]->unSelectSpeaker();
    }

    if (countS == 3) {
        int i1 = -1, i2 = -1, i3 = -1;
        for (unsigned int i = 0; i < this->mSpeakers.size(); ++i) {
            if (this->mSpeakers[i]->isSelected()) {
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
                this->mTriplets.erase(this->mTriplets.begin() + posDel);
            } else {
                this->mTriplets.push_back(tri);
            }
        }
    }
}

//==============================================================================
bool MainContentComponent::tripletExist(Triplet tri, int & pos) const
{
    pos = 0;
    for (auto const & ti : this->mTriplets) {
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
    for (auto && it : this->mSpeakers) {
        it->setSpeakerId(id++);
    }
}

//==============================================================================
void MainContentComponent::reorderSpeakers(std::vector<int> const & newOrder)
{
    auto const size = this->mSpeakers.size();

    juce::Array<Speaker *> tempListSpeaker{};
    tempListSpeaker.resize(size);

    for (int i = 0; i < size; i++) {
        for (auto && it : this->mSpeakers) {
            if (it->getIdSpeaker() == newOrder[i]) {
                tempListSpeaker.setUnchecked(i, it);
                break;
            }
        }
    }

    this->mSpeakerLocks.lock();
    this->mSpeakers.clearQuick(false);
    this->mSpeakers.addArray(tempListSpeaker);
    this->mSpeakerLocks.unlock();
}

//==============================================================================
int MainContentComponent::getMaxSpeakerId() const
{
    int maxId = 0;
    for (auto && it : this->mSpeakers) {
        if (it->getIdSpeaker() > maxId)
            maxId = it->getIdSpeaker();
    }
    return maxId;
}

//==============================================================================
int MainContentComponent::getMaxSpeakerOutputPatch() const
{
    int maxOut = 0;
    for (auto && it : this->mSpeakers) {
        if (it->getOutputPatch() > maxOut)
            maxOut = it->getOutputPatch();
    }
    return maxOut;
}

//==============================================================================
void MainContentComponent::addSpeaker(int sortColumnId, bool isSortedForwards)
{
    int newId = this->getMaxSpeakerId() + 1;

    this->mSpeakerLocks.lock();
    this->mSpeakers.add(new Speaker(this, newId, newId, 0.0f, 0.0f, 1.0f));

    if (sortColumnId == 1 && isSortedForwards) {
        for (unsigned int i = 0; i < this->mSpeakers.size(); i++) {
            this->mSpeakers[i]->setSpeakerId(i + 1);
        }
    } else if (sortColumnId == 1 && !isSortedForwards) {
        for (unsigned int i = 0; i < this->mSpeakers.size(); i++) {
            this->mSpeakers[i]->setSpeakerId((int)this->mSpeakers.size() - i);
        }
    }
    this->mSpeakerLocks.unlock();

    this->jackClient->setProcessBlockOn(false);
    this->jackClient->addOutput(this->mSpeakers.getLast()->getOutputPatch());
    this->jackClient->setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::insertSpeaker(int position, int sortColumnId, bool isSortedForwards)
{
    int newPosition = position + 1;
    int newId = this->getMaxSpeakerId() + 1;
    int newOut = this->getMaxSpeakerOutputPatch() + 1;

    this->mSpeakerLocks.lock();
    if (sortColumnId == 1 && isSortedForwards) {
        newId = this->mSpeakers[position]->getIdSpeaker() + 1;
        this->mSpeakers.insert(newPosition, new Speaker(this, newId, newOut, 0.0f, 0.0f, 1.0f));
        for (unsigned int i = 0; i < this->mSpeakers.size(); i++) {
            this->mSpeakers.getUnchecked(i)->setSpeakerId(i + 1);
        }
    } else if (sortColumnId == 1 && !isSortedForwards) {
        newId = this->mSpeakers[position]->getIdSpeaker() - 1;
        ;
        this->mSpeakers.insert(newPosition, new Speaker(this, newId, newOut, 0.0f, 0.0f, 1.0f));
        for (unsigned int i = 0; i < this->mSpeakers.size(); i++) {
            this->mSpeakers.getUnchecked(i)->setSpeakerId(this->mSpeakers.size() - i);
        }
    } else {
        this->mSpeakers.insert(newPosition, new Speaker(this, newId, newOut, 0.0f, 0.0f, 1.0f));
    }
    this->mSpeakerLocks.unlock();

    this->jackClient->setProcessBlockOn(false);
    this->jackClient->clearOutput();
    for (auto && it : this->mSpeakers) {
        this->jackClient->addOutput(it->getOutputPatch());
    }
    this->jackClient->setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::removeSpeaker(int idSpeaker)
{
    this->jackClient->removeOutput(idSpeaker);
    this->mSpeakerLocks.lock();
    this->mSpeakers.remove(idSpeaker, true);
    this->mSpeakerLocks.unlock();
}

//==============================================================================
bool MainContentComponent::isRadiusNormalized() const
{
    auto const mode{ this->jackClient->getMode() };
    if (mode == VBAP || mode == VBAP_HRTF)
        return true;
    else
        return false;
}

//==============================================================================
void MainContentComponent::updateInputJack(int inInput, Input & inp)
{
    auto const mode{ jackClient->getMode() };
    auto & si = this->jackClient->getSourcesIn()[inInput];

    if (mode == LBAP) {
        si.radazi = inp.getAzimuth();
        si.radele = M_PI2 - inp.getZenith();
    } else {
        si.azimuth = ((inp.getAzimuth() / M2_PI) * 360.0f);
        if (si.azimuth > 180.0f) {
            si.azimuth = si.azimuth - 360.0f;
        }
        si.zenith = 90.0f - (inp.getZenith() / M2_PI) * 360.0f;
    }
    si.radius = inp.getRadius();

    si.aziSpan = inp.getAzimuthSpan() * 0.5f;
    si.zenSpan = inp.getZenithSpan() * 2.0f;

    if (mode == VBAP || mode == VBAP_HRTF) {
        this->jackClient->getVbapSourcesToUpdate()[inInput] = 1;
    }
}

//==============================================================================
void MainContentComponent::setListTripletFromVbap()
{
    this->clearListTriplet();
    for (auto const & it : jackClient->getVbapTriplets()) {
        Triplet tri;
        tri.id1 = it[0];
        tri.id2 = it[1];
        tri.id3 = it[2];
        this->mTriplets.push_back(tri);
    }
}

//==============================================================================
Speaker const * MainContentComponent::getSpeakerFromOutputPatch(int out) const
{
    for (auto const * it : this->mSpeakers) {
        if (it->getOutputPatch() == out && !it->isDirectOut()) {
            return it;
        }
    }
    return nullptr;
}

//==============================================================================
Speaker * MainContentComponent::getSpeakerFromOutputPatch(int out)
{
    for (auto it : this->mSpeakers) {
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
float MainContentComponent::getLevelsAlpha(int indexLevel) const
{
    float level = this->jackClient->getLevelsIn(indexLevel);
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
    float level = this->jackClient->getLevelsOut(indexLevel);
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

    if (this->mSpeakers.size() == 0)
        return false;

    // Test for a 2-D or 3-D configuration.
    float zenith = -1.0f;
    for (auto && it : this->mSpeakers) {
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
    if ((this->mSpeakers.size() - directOutSpeakers) < dimensions) {
        juce::AlertWindow alert("Not enough speakers!    ",
                                "Do you want to reload previous config?    ",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("No", 0);
        alert.addButton("Yes", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            this->openXmlFileSpeaker(this->mPathCurrentFileSpeaker);
        }
        return false;
    }

    // Test for duplicated output patch.
    std::vector<int> tempout;
    for (unsigned int i = 0; i < this->mSpeakers.size(); i++) {
        if (!this->mSpeakers[i]->isDirectOut()) {
            tempout.push_back(this->mSpeakers[i]->getOutputPatch());
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
                if (this->mPathCurrentFileSpeaker.compare(this->mPathLastVbapSpeakerSetup) == 0) {
                    this->openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
                } else {
                    this->openXmlFileSpeaker(this->mPathLastVbapSpeakerSetup);
                }
            }
            return false;
        }
    }

    this->jackClient->setProcessBlockOn(false);
    this->jackClient->setMaxOutputPatch(0);

    // Save mute/solo/directout states
    bool inputsIsMuted[MaxInputs];
    bool inputsIsSolo[MaxInputs];
    auto const soloIn = this->jackClient->getSoloIn();
    int directOuts[MaxInputs];
    auto const * sourcesIn{ jackClient->getSourcesIn() };
    for (unsigned int i = 0; i < MaxInputs; i++) {
        inputsIsMuted[i] = sourcesIn[i].isMuted;
        inputsIsSolo[i] = sourcesIn[i].isSolo;
        directOuts[i] = sourcesIn[i].directOut;
    }

    bool outputsIsMuted[MaxInputs];
    bool outputsIsSolo[MaxInputs];
    bool soloOut = this->jackClient->getSoloOut();
    auto const * speakersOut{ jackClient->getSpeakersOut() };
    for (unsigned int i = 0; i < MaxOutputs; i++) {
        outputsIsMuted[i] = speakersOut[i].isMuted;
        outputsIsSolo[i] = speakersOut[i].isSolo;
    }

    // Cleanup speakers output patch
    for (auto & it : this->jackClient->getSpeakersOut()) {
        it.outputPatch = 0;
    }

    // Create outputs.
    int i = 0, x = 2;
    auto const mode{ jackClient->getMode() };
    for (auto && it : this->mSpeakers) {
        juce::Rectangle<int> level(x, 4, VuMeterWidthInPixels, 200);
        it->getVuMeter()->setBounds(level);
        it->getVuMeter()->resetClipping();
        this->mOutputsUiBox->getContent()->addAndMakeVisible(it->getVuMeter());
        it->getVuMeter()->repaint();

        x += VuMeterWidthInPixels;

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

        this->jackClient->getSpeakersOut()[i++] = so;

        if (static_cast<unsigned int>(it->getOutputPatch()) > this->jackClient->getMaxOutputPatch())
            this->jackClient->setMaxOutputPatch(it->getOutputPatch());
    }

    // Set user gain and highpass filter cutoff frequency for each speaker.
    for (auto const * speaker : this->mSpeakers) {
        this->jackClient->getSpeakersOut()[speaker->outputPatch - 1].gain = std::pow(10.0f, speaker->getGain() * 0.05f);
        if (speaker->getHighPassCutoff() > 0.0f) {
            double * coeffs;
            Linkwitz_Riley_compute_variables(static_cast<double>(speaker->getHighPassCutoff()),
                                             (double)this->mSamplingRate,
                                             &coeffs,
                                             7);
            auto & speakerOut{ jackClient->getSpeakersOut()[speaker->outputPatch - 1] };
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
    this->mInputLocks.lock();
    for (auto * input : this->mSourceInputs) {
        juce::Rectangle<int> level(x, 4, VuMeterWidthInPixels, 200);
        input->getVuMeter()->setBounds(level);
        input->getVuMeter()->updateDirectOutMenu(this->mSpeakers);
        input->getVuMeter()->resetClipping();
        this->mInputsUiBox->getContent()->addAndMakeVisible(input->getVuMeter());
        input->getVuMeter()->repaint();

        x += VuMeterWidthInPixels;

        SourceIn si;
        si.id = input->getId();
        si.radazi = input->getAzimuth();
        si.radele = M_PI2 - input->getZenith();
        si.azimuth = input->getAzimuth();
        si.zenith = input->getZenith();
        si.radius = input->getRadius();
        si.gain = 0.0f;
        this->jackClient->getSourcesIn()[i++] = si;
    }

    this->mInputLocks.unlock();

    if (this->mEditSpeakersWindow != nullptr) {
        this->mEditSpeakersWindow->updateWinContent();
    }

    this->mOutputsUiBox->repaint();
    this->resized();

    // Temporarily remove direct out speakers to construct vbap or lbap algorithm.
    i = 0;
    std::vector<Speaker *> tempListSpeaker;
    tempListSpeaker.resize(this->mSpeakers.size());
    for (auto && it : this->mSpeakers) {
        if (!it->isDirectOut()) {
            tempListSpeaker[i++] = it;
        }
    }
    tempListSpeaker.resize(i);

    bool retval = false;
    if (mode == VBAP || mode == VBAP_HRTF) {
        this->jackClient->setVbapDimensions(dimensions);
        if (dimensions == 2) {
            this->setShowTriplets(false);
        }
        retval = this->jackClient->initSpeakersTriplet(tempListSpeaker, dimensions, this->mNeedToComputeVbap);

        if (retval) {
            this->setListTripletFromVbap();
            this->mNeedToComputeVbap = false;
        } else {
            juce::AlertWindow alert("Not a valid DOME 3-D configuration!    ",
                                    "Maybe you want to open it in CUBE mode? Reload the default speaker setup...    ",
                                    juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
            alert.runModalLoop();
            this->openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
            return false;
        }
    } else if (mode == LBAP) {
        this->setShowTriplets(false);
        retval = this->jackClient->lbapSetupSpeakerField(tempListSpeaker);
    }

    // Restore mute/solo/directout states
    this->jackClient->setSoloIn(soloIn);
    for (unsigned int i = 0; i < MaxInputs; i++) {
        auto & sourceIn{ jackClient->getSourcesIn()[i] };
        sourceIn.isMuted = inputsIsMuted[i];
        sourceIn.isSolo = inputsIsSolo[i];
        sourceIn.directOut = directOuts[i];
    }
    this->mInputLocks.lock();
    for (unsigned int i = 0; i < this->mSourceInputs.size(); i++) {
        this->mSourceInputs[i]->setDirectOutChannel(directOuts[i]);
    }
    this->mInputLocks.unlock();

    this->jackClient->setSoloOut(soloOut);
    for (unsigned int i = 0; i < MaxOutputs; i++) {
        auto & speakerOut{ jackClient->getSpeakersOut()[i] };
        speakerOut.isMuted = outputsIsMuted[i];
        speakerOut.isSolo = outputsIsSolo[i];
    }

    this->jackClient->setProcessBlockOn(true);

    return retval;
}

//==============================================================================
void MainContentComponent::setNameConfig()
{
    this->mNameConfig = this->mPathCurrentFileSpeaker.fromLastOccurrenceOf("/", false, false);
    this->mSpeakerViewComponent->setNameConfig(this->mNameConfig);
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

    this->jackClient->setSoloIn(false);
    for (unsigned int i = 0; i < MaxInputs; i++) {
        if (sourcesIn[i].isSolo) {
            this->jackClient->setSoloIn(true);
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
    for (unsigned int i = 0; i < MaxOutputs; i++) {
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
    if (juce::File(this->mPathCurrentFileSpeaker).existsAsFile()) {
        this->openXmlFileSpeaker(this->mPathCurrentFileSpeaker);
    }
}

//==============================================================================
void MainContentComponent::openXmlFileSpeaker(juce::String path)
{
    juce::String msg;
    juce::String oldPath = this->mPathCurrentFileSpeaker;
    bool isNewSameAsOld = oldPath.compare(path) == 0;
    bool isNewSameAsLastSetup = this->mPathLastVbapSpeakerSetup.compare(path) == 0;
    bool ok = false;
    if (!juce::File(path.toStdString()).existsAsFile()) {
        juce::AlertWindow alert("Error in Load Speaker Setup !",
                                "Can't found file " + path.toStdString() + ", the current setup will be kept.",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
    } else {
        this->mPathCurrentFileSpeaker = path.toStdString();
        juce::XmlDocument xmlDoc(juce::File(this->mPathCurrentFileSpeaker));
        std::unique_ptr<juce::XmlElement> mainXmlElem(xmlDoc.getDocumentElement());
        if (mainXmlElem == nullptr) {
            juce::AlertWindow alert("Error in Load Speaker Setup !",
                                    "Your file is corrupted !\n" + xmlDoc.getLastParseError(),
                                    juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("Ok", 0);
            alert.runModalLoop();
        } else {
            if (mainXmlElem->hasTagName("SpeakerSetup")) {
                this->mSpeakerLocks.lock();
                this->mSpeakers.clear();
                this->mSpeakerLocks.unlock();
                if (path.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) == 0) {
                    this->jackClient->setMode(VBAP_HRTF);
                    this->mModeSpatCombo->setSelectedId(VBAP_HRTF + 1, juce::NotificationType::dontSendNotification);
                } else if (path.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) == 0) {
                    this->jackClient->setMode(STEREO);
                    this->mModeSpatCombo->setSelectedId(STEREO + 1, juce::NotificationType::dontSendNotification);
                } else if (!isNewSameAsOld && oldPath.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
                           && oldPath.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
                    int spatMode = mainXmlElem->getIntAttribute("SpatMode");
                    this->jackClient->setMode(static_cast<ModeSpatEnum>(spatMode));
                    this->mModeSpatCombo->setSelectedId(spatMode + 1, juce::NotificationType::dontSendNotification);
                } else if (!isNewSameAsLastSetup) {
                    int spatMode = mainXmlElem->getIntAttribute("SpatMode");
                    this->jackClient->setMode(static_cast<ModeSpatEnum>(spatMode));
                    this->mModeSpatCombo->setSelectedId(spatMode + 1, juce::NotificationType::dontSendNotification);
                }

                bool loadSetupFromXYZ = false;
                if (isNewSameAsOld && this->jackClient->getMode() == LBAP)
                    loadSetupFromXYZ = true;

                this->setNameConfig();
                this->jackClient->setProcessBlockOn(false);
                this->jackClient->clearOutput();
                this->jackClient->setMaxOutputPatch(0);
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

                                this->mSpeakers.add(new Speaker(this,
                                                                layoutIndex,
                                                                spk->getIntAttribute("OutputPatch"),
                                                                spk->getDoubleAttribute("Azimuth"),
                                                                spk->getDoubleAttribute("Zenith"),
                                                                spk->getDoubleAttribute("Radius")));
                                if (loadSetupFromXYZ) {
                                    this->mSpeakers.getLast()->setCoordinate(
                                        glm::vec3(spk->getDoubleAttribute("PositionX"),
                                                  spk->getDoubleAttribute("PositionZ"),
                                                  spk->getDoubleAttribute("PositionY")));
                                }
                                if (spk->hasAttribute("Gain")) {
                                    this->mSpeakers.getLast()->setGain(spk->getDoubleAttribute("Gain"));
                                }
                                if (spk->hasAttribute("HighPassCutoff")) {
                                    this->mSpeakers.getLast()->setHighPassCutoff(
                                        spk->getDoubleAttribute("HighPassCutoff"));
                                }
                                if (spk->hasAttribute("DirectOut")) {
                                    this->mSpeakers.getLast()->setDirectOut(spk->getBoolAttribute("DirectOut"));
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
                        this->mTriplets.push_back(tri);
                    }
                }
                this->jackClient->setProcessBlockOn(true);
                ok = true;
            } else {
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
        if (this->mPathCurrentFileSpeaker.endsWith("default_preset/default_speaker_setup.xml")) {
            this->mApplicationProperties.getUserSettings()->setValue(
                "lastSpeakerSetupDirectory",
                juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName());
        } else {
            this->mApplicationProperties.getUserSettings()->setValue(
                "lastSpeakerSetupDirectory",
                juce::File(this->mPathCurrentFileSpeaker).getParentDirectory().getFullPathName());
        }
        this->mNeedToComputeVbap = true;
        this->updateLevelComp();
        auto const mode{ jackClient->getMode() };
        if (mode != VBAP_HRTF && mode != STEREO) {
            if (this->mPathCurrentFileSpeaker.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
                && this->mPathCurrentFileSpeaker.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
                this->mPathLastVbapSpeakerSetup = this->mPathCurrentFileSpeaker;
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
    this->mMainWindow.setName(version + juce::File(this->mPathCurrentPreset).getFileName());
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
    this->mEditSpeakersWindow.reset();
    this->jackClient->setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::openPreset(juce::String path)
{
    juce::String msg;
    this->jackClient->setProcessBlockOn(false);
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
            this->mPathCurrentPreset = path;
            this->mOscInputPort
                = mainXmlElem->getIntAttribute("OSC_Input_Port"); // TODO: app preferences instead of project settings ?
            this->mAddInputsTextEditor->setText(mainXmlElem->getStringAttribute("Number_Of_Inputs"));
            this->mMasterGainOutSlider->setValue(mainXmlElem->getDoubleAttribute("Master_Gain_Out", 0.0),
                                                 juce::sendNotification);
            this->mInterpolationSlider->setValue(mainXmlElem->getDoubleAttribute("Master_Interpolation", 0.1),
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
                this->mIsSourceLevelShown = mainXmlElem->getBoolAttribute("Use_Alpha");
            } else {
                this->mIsSourceLevelShown = false;
            }
            if (mainXmlElem->hasAttribute("Use_Alpha")) {
                this->mIsSourceLevelShown = mainXmlElem->getBoolAttribute("Use_Alpha");
            } else {
                this->mIsSourceLevelShown = false;
            }
            if (mainXmlElem->hasAttribute("Show_Speaker_Level")) {
                this->mIsSpeakerLevelShown = mainXmlElem->getBoolAttribute("Show_Speaker_Level");
            } else {
                this->mIsSpeakerLevelShown = false;
            }
            if (mainXmlElem->hasAttribute("Show_Sphere")) {
                this->mIsSphereShown = mainXmlElem->getBoolAttribute("Show_Sphere");
            } else {
                this->mIsSphereShown = false;
            }
            this->mSpeakerViewComponent->setShowSphere(this->mIsSphereShown);

            if (mainXmlElem->hasAttribute("CamAngleX")) {
                float angleX = mainXmlElem->getDoubleAttribute("CamAngleX");
                float angleY = mainXmlElem->getDoubleAttribute("CamAngleY");
                float distance = mainXmlElem->getDoubleAttribute("CamDistance");
                this->mSpeakerViewComponent->setCamPosition(angleX, angleY, distance);
            } else {
                this->mSpeakerViewComponent->setCamPosition(80.0f, 25.0f, 22.0f); // TODO: named constants ?
            }

            // Update
            this->textEditorReturnKeyPressed(*this->mAddInputsTextEditor);
            this->sliderValueChanged(this->mMasterGainOutSlider.get());
            this->sliderValueChanged(this->mInterpolationSlider.get());

            juce::File speakerSetup = juce::File(this->mPathCurrentFileSpeaker.toStdString());
            if (!this->mPathCurrentFileSpeaker.startsWith("/")) {
                this->mPathCurrentFileSpeaker
                    = DEFAULT_PRESET_DIRECTORY.getChildFile(this->mPathCurrentFileSpeaker).getFullPathName();
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
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
            alert.runModalLoop();
        }
    }

    this->jackClient->setPinkNoiseActive(false);
    this->jackClient->setProcessBlockOn(true);

    if (this->mPathCurrentPreset.endsWith("default_preset/default_preset.xml")) {
        this->mApplicationProperties.getUserSettings()->setValue(
            "lastPresetDirectory",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName());
    } else {
        this->mApplicationProperties.getUserSettings()->setValue(
            "lastPresetDirectory",
            juce::File(this->mPathCurrentPreset).getParentDirectory().getFullPathName());
    }
    this->setTitle();
}

//==============================================================================
void MainContentComponent::getPresetData(juce::XmlElement * xml) const
{
    xml->setAttribute("OSC_Input_Port", juce::String(this->mOscInputPort));
    xml->setAttribute("Number_Of_Inputs", this->mAddInputsTextEditor->getTextValue().toString());
    xml->setAttribute("Master_Gain_Out", this->mMasterGainOutSlider->getValue());
    xml->setAttribute("Master_Interpolation", this->mInterpolationSlider->getValue());
    xml->setAttribute("Show_Numbers", this->mIsNumbersShown);
    xml->setAttribute("Show_Speakers", this->mIsSpeakersShown);
    xml->setAttribute("Show_Triplets", this->mIsTripletsShown);
    xml->setAttribute("Use_Alpha", this->mIsSourceLevelShown);
    xml->setAttribute("Show_Speaker_Level", this->mIsSpeakerLevelShown);
    xml->setAttribute("Show_Sphere", this->mIsSphereShown);
    xml->setAttribute("CamAngleX", this->mSpeakerViewComponent->getCamAngleX());
    xml->setAttribute("CamAngleY", this->mSpeakerViewComponent->getCamAngleY());
    xml->setAttribute("CamDistance", this->mSpeakerViewComponent->getCamDistance());

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
    this->getPresetData(xml.get());
    xml->writeTo(xmlFile);
    xmlFile.create();
    this->mPathCurrentPreset = path;
    this->mApplicationProperties.getUserSettings()->setValue(
        "lastPresetDirectory",
        juce::File(this->mPathCurrentPreset).getParentDirectory().getFullPathName());
    this->setTitle();
}

//==============================================================================
void MainContentComponent::saveSpeakerSetup(juce::String path)
{
    this->mPathCurrentFileSpeaker = path;
    juce::File xmlFile = juce::File(path.toStdString());
    juce::XmlElement xml{ "SpeakerSetup" };

    xml.setAttribute("Name", this->mNameConfig);
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

    this->mApplicationProperties.getUserSettings()->setValue(
        "lastSpeakerSetupDirectory",
        juce::File(this->mPathCurrentFileSpeaker).getParentDirectory().getFullPathName());

    this->mNeedToSaveSpeakerSetup = false;

    auto const mode{ jackClient->getMode() };
    if (mode != VBAP_HRTF && mode != STEREO) {
        if (this->mPathCurrentFileSpeaker.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
            && this->mPathCurrentFileSpeaker.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
            this->mPathLastVbapSpeakerSetup = this->mPathCurrentFileSpeaker;
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
    juce::PropertiesFile * props = this->mApplicationProperties.getUserSettings();

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
        this->mOscInputPort = oscPort;
        props->setValue("OscInputPort", oscPort);
        this->mOscReceiver->closeConnection();
        if (this->mOscReceiver->startConnection(this->mOscInputPort)) {
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
    this->jackClient->setAttenuationDb(linGain);
    props->setValue("AttenuationDB", attenuationDB);

    float coeff = expf(-M2_PI * AttenuationCutoffs[attenuationHz].getFloatValue() / this->jackClient->getSampleRate());
    this->jackClient->setAttenuationHz(coeff);
    props->setValue("AttenuationHz", attenuationHz);

    mApplicationProperties.saveIfNeeded();
}

//==============================================================================
void MainContentComponent::timerCallback()
{
    this->mJackLoadLabel->setText(juce::String(this->jackClient->getCpuUsed() * 100.0f, 4) + " %",
                                  juce::dontSendNotification);
    int seconds = this->jackClient->getIndexRecord() / this->jackClient->getSampleRate();
    int minute = int(seconds / 60) % 60;
    seconds = int(seconds % 60);
    juce::String timeRecorded = ((minute < 10) ? "0" + juce::String(minute) : juce::String(minute)) + " : "
                                + ((seconds < 10) ? "0" + juce::String(seconds) : juce::String(seconds));
    this->mTimeRecordedLabel->setText(timeRecorded, juce::dontSendNotification);

    if (this->mStartRecordButton->getToggleState()) {
        this->mStartRecordButton->setToggleState(false, juce::dontSendNotification);
    }

    if (this->jackClient->isSavingRun()) {
        this->mStartRecordButton->setButtonText("Stop");
    } else if (this->jackClient->getRecordingPath() == "") {
        this->mStartRecordButton->setButtonText("Record");
    } else {
        this->mStartRecordButton->setButtonText("Record");
    }

    if (this->mIsRecording && !this->jackClient->isRecording()) {
        bool isReadyToMerge = true;
        for (unsigned int i = 0; i < MaxOutputs; i++) {
            if (this->jackClient->getRecorders()[i].backgroundThread.isThreadRunning()) {
                isReadyToMerge = false;
            }
        }
        if (isReadyToMerge) {
            this->mIsRecording = false;
            if (this->jackClient->getRecordFileConfig()) {
                this->jackClient->setProcessBlockOn(false);
                AudioRenderer * renderer = new AudioRenderer();
                renderer->prepareRecording(this->jackClient->getRecordingPath(),
                                           this->jackClient->getOutputFileNames(),
                                           this->jackClient->getSampleRate());
                renderer->runThread();
                this->jackClient->setProcessBlockOn(true);
            }
        }
    }

    if (this->jackClient->isOverloaded()) {
        this->mJackLoadLabel->setColour(juce::Label::backgroundColourId, juce::Colours::darkred);
    } else {
        this->mJackLoadLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    }

    for (auto && it : mSourceInputs) {
        it->getVuMeter()->update();
    }

    for (auto && it : mSpeakers) {
        it->getVuMeter()->update();
    }

    this->mJackClientListComponent->updateContentCli();

    if (this->mIsProcessForeground != juce::Process::isForegroundProcess()) {
        this->mIsProcessForeground = juce::Process::isForegroundProcess();
        if (this->mEditSpeakersWindow != nullptr && this->mIsProcessForeground) {
            this->mEditSpeakersWindow->setVisible(true);
            this->mEditSpeakersWindow->setAlwaysOnTop(true);
        } else if (this->mEditSpeakersWindow != nullptr && !this->mIsProcessForeground) {
            this->mEditSpeakersWindow->setVisible(false);
            this->mEditSpeakersWindow->setAlwaysOnTop(false);
        }
        if (this->mFlatViewWindow != nullptr && this->mIsProcessForeground) {
            this->mFlatViewWindow->toFront(false);
            this->toFront(true);
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
    if (&textEditor == this->mAddInputsTextEditor.get()) {
        unsigned int num_of_inputs = (unsigned int)this->mAddInputsTextEditor->getTextValue().toString().getIntValue();
        if (num_of_inputs < 1) {
            this->mAddInputsTextEditor->setText("1");
        }
        if (num_of_inputs > MaxInputs) {
            this->mAddInputsTextEditor->setText(juce::String(MaxInputs));
        }

        if (this->jackClient->getInputPorts().size() != num_of_inputs) {
            this->jackClient->setProcessBlockOn(false);
            this->jackClient->addRemoveInput(num_of_inputs);
            this->jackClient->setProcessBlockOn(true);

            this->mInputLocks.lock();
            bool addInput = false;
            for (unsigned int i = 0; i < this->jackClient->getInputPorts().size(); i++) {
                if (i >= this->mSourceInputs.size()) {
                    this->mSourceInputs.add(new Input(*this, mSmallLookAndFeel, i + 1));
                    addInput = true;
                }
            }
            if (!addInput) {
                auto const listSourceInputSize{ this->mSourceInputs.size() };
                auto const jackClientInputPortSize{ this->jackClient->getInputPorts().size() };
                if (listSourceInputSize > jackClientInputPortSize) {
                    this->mSourceInputs.removeRange(jackClientInputPortSize,
                                                    listSourceInputSize - jackClientInputPortSize);
                }
            }
            this->mInputLocks.unlock();
        }
        this->unfocusAllComponents();
        updateLevelComp();
    }
}

//==============================================================================
void MainContentComponent::buttonClicked(juce::Button * button)
{
    if (button == this->mStartRecordButton.get()) {
        if (this->jackClient->isRecording()) {
            this->jackClient->stopRecord();
            this->mStartRecordButton->setEnabled(false);
            this->mTimeRecordedLabel->setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
        } else {
            this->mIsRecording = true;
            this->jackClient->startRecord();
            this->mTimeRecordedLabel->setColour(juce::Label::textColourId, mLookAndFeel.getRedColour());
        }
        this->mStartRecordButton->setToggleState(this->jackClient->isRecording(), juce::dontSendNotification);
    } else if (button == this->mInitRecordButton.get()) {
        this->chooseRecordingPath();
        this->mStartRecordButton->setEnabled(true);
    }
}

//==============================================================================
void MainContentComponent::sliderValueChanged(juce::Slider * slider)
{
    if (slider == this->mMasterGainOutSlider.get()) {
        this->jackClient->setMasterGainOut(pow(10.0, this->mMasterGainOutSlider->getValue() * 0.05));
    }

    else if (slider == this->mInterpolationSlider.get()) {
        this->jackClient->setInterMaster(this->mInterpolationSlider->getValue());
    }
}

//==============================================================================
void MainContentComponent::comboBoxChanged(juce::ComboBox * comboBox)
{
    if (this->mEditSpeakersWindow != nullptr && this->mNeedToSaveSpeakerSetup) {
        juce::AlertWindow alert(
            "The speaker configuration has changed!    ",
            "Save your changes or close the speaker configuration window before switching mode...    ",
            juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        this->mModeSpatCombo->setSelectedId(this->jackClient->getMode() + 1,
                                            juce::NotificationType::dontSendNotification);
        return;
    }

    if (this->mModeSpatCombo.get() == comboBox) {
        this->jackClient->setProcessBlockOn(false);
        this->jackClient->setMode((ModeSpatEnum)(this->mModeSpatCombo->getSelectedId() - 1));
        switch (jackClient->getMode()) {
        case VBAP:
            this->openXmlFileSpeaker(this->mPathLastVbapSpeakerSetup);
            this->mNeedToSaveSpeakerSetup = false;
            this->mIsSpanShown = true;
            break;
        case LBAP:
            this->openXmlFileSpeaker(this->mPathLastVbapSpeakerSetup);
            this->mNeedToSaveSpeakerSetup = false;
            this->mIsSpanShown = true;
            break;
        case VBAP_HRTF:
            this->openXmlFileSpeaker(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName());
            this->mNeedToSaveSpeakerSetup = false;
            this->jackClient->resetHrtf();
            this->mIsSpanShown = false;
            break;
        case STEREO:
            this->openXmlFileSpeaker(STEREO_SPEAKER_SETUP_FILE.getFullPathName());
            this->mNeedToSaveSpeakerSetup = false;
            this->mIsSpanShown = false;
            break;
        default:
            break;
        }
        this->jackClient->setProcessBlockOn(true);

        if (this->mEditSpeakersWindow != nullptr) {
            juce::String windowName = juce::String("Speakers Setup Edition - ")
                                      + juce::String(ModeSpatString[this->jackClient->getMode()]) + juce::String(" - ")
                                      + juce::File(this->mPathCurrentFileSpeaker).getFileName();
            this->mEditSpeakersWindow->setName(windowName);
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
    if (this->mOscLogWindow != nullptr) {
        juce::String address = message.getAddressPattern().toString();
        this->mOscLogWindow->addToLog(address + "\n");
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
        this->mOscLogWindow->addToLog(msg + "\n");
    }
}

//==============================================================================
void MainContentComponent::chooseRecordingPath()
{
    juce::String dir = this->mApplicationProperties.getUserSettings()->getValue("lastRecordingDirectory");
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
        this->mApplicationProperties.getUserSettings()->setValue(
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

    mMenuBar->setBounds(0, 0, this->getWidth(), 20);
    r.removeFromTop(20);

    // Lay out the speaker view and the vertical divider.
    Component * vcomps[] = { this->mSpeakerViewComponent.get(), this->mVerticalDividerBar.get(), nullptr };

    // Lay out side-by-side and resize the components' heights as well as widths.
    this->mVerticalLayout.layOutComponents(vcomps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);

    this->mMainUiBox->setBounds(this->mSpeakerViewComponent->getWidth() + 6,
                                20,
                                getWidth() - (this->mSpeakerViewComponent->getWidth() + 10),
                                getHeight());
    this->mMainUiBox->correctSize(getWidth() - this->mSpeakerViewComponent->getWidth() - 6, 610);

    this->mInputsUiBox->setBounds(0, 2, getWidth() - (this->mSpeakerViewComponent->getWidth() + 10), 231);
    this->mInputsUiBox->correctSize(((unsigned int)this->mSourceInputs.size() * (VuMeterWidthInPixels)) + 4, 200);

    this->mOutputsUiBox->setBounds(0, 233, getWidth() - (this->mSpeakerViewComponent->getWidth() + 10), 210);
    this->mOutputsUiBox->correctSize(((unsigned int)this->mSpeakers.size() * (VuMeterWidthInPixels)) + 4, 180);

    this->mControlUiBox->setBounds(0, 443, getWidth() - (this->mSpeakerViewComponent->getWidth() + 10), 145);
    this->mControlUiBox->correctSize(720, 145);
}
