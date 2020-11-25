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
#include "ServerGrisConstants.h"

//==============================================================================
MainContentComponent::MainContentComponent(MainWindow & mainWindow, GrisLookAndFeel & newLookAndFeel)
    : mLookAndFeel(newLookAndFeel)
    , mMainWindow(mainWindow)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&newLookAndFeel);

    // Create the menu bar.
    mMenuBar.reset(new juce::MenuBarComponent(this));
    addAndMakeVisible(mMenuBar.get());

    // App user settings storage file.
    juce::PropertiesFile::Options options{};
    options.applicationName = "SpatGRIS2";
    options.commonToAllUsers = false;
    options.filenameSuffix = "xml";
    options.folderName = "GRIS";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.ignoreCaseOfKeyNames = true;
    options.osxLibrarySubFolder = "Application Support";
    mApplicationProperties.setStorageParameters(options);

    auto * props{ mApplicationProperties.getUserSettings() };

    // Get a reference to the last opened VBAP speaker setup.
    juce::File const lastVbap{ props->getValue("lastVbapSpeakerSetup", "./not_saved_yet") };
    if (!lastVbap.existsAsFile()) {
        mPathLastVbapSpeakerSetup = DEFAULT_SPEAKER_SETUP_FILE.getFullPathName();
    } else {
        mPathLastVbapSpeakerSetup = props->getValue("lastVbapSpeakerSetup");
    }

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

    addLabel("Mode :", "Mode of spatialization", 150, 30, 60, 20, mControlUiBox->getContent());
    mModeSpatCombo.reset(addComboBox("", "Mode of spatialization", 155, 48, 90, 22, mControlUiBox->getContent()));
    for (int i{}; i < MODE_SPAT_STRING.size(); i++) {
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

    // Set up the layout and resize bars.
    mVerticalLayout.setItemLayout(0,
                                  -0.2,
                                  -0.8,
                                  -0.435);     // width of the speaker view must be between 20% and 80%, preferably 50%
    mVerticalLayout.setItemLayout(1, 8, 8, 8); // the vertical divider drag-bar thing is always 8 pixels wide
    mVerticalLayout.setItemLayout(
        2,
        150.0,
        -1.0,
        -0.565); // right panes must be at least 150 pixels wide, preferably 50% of the total width
    mVerticalDividerBar.reset(new juce::StretchableLayoutResizerBar(&mVerticalLayout, 1, true));
    addAndMakeVisible(mVerticalDividerBar.get());

    // Default application window size.
    setSize(1285, 610);

    // Jack Initialization parameters.
    auto bufferValue{ static_cast<unsigned>(props->getIntValue("BufferValue", 1024)) };
    auto rateValue{ static_cast<unsigned>(props->getIntValue("RateValue", 48000)) };

    if (bufferValue == 0 || rateValue == 0) {
        bufferValue = 1024u;
        rateValue = 48000u;
    }

    mSamplingRate = rateValue;

    // Start Jack Server and client.
    mAlsaOutputDevice = props->getValue("AlsaOutputDevice", "");
    int errorCode{};
    mJackServer.reset(new JackServerGris{ rateValue, bufferValue, mAlsaOutputDevice, &errorCode });
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
                               "device\n(Sampling Rate, Input/Output Channels, etc.)"));
    }

    mAlsaAvailableOutputDevices = mJackServer->getAvailableOutputDevices();

    auto const fileFormat{ props->getIntValue("FileFormat", 0) };
    mJackClient.setRecordFormat(fileFormat);
    auto const fileConfig{ props->getIntValue("FileConfig", 0) };
    mJackClient.setRecordFileConfig(fileConfig);

    if (!mJackClient.isReady()) {
        mJackStatusLabel->setText("Jack ERROR", juce::dontSendNotification);
    } else {
        mJackStatusLabel->setText("Jack Run", juce::dontSendNotification);
    }

    mJackRateLabel->setText(juce::String(mJackClient.getSampleRate()) + " Hz", juce::dontSendNotification);
    mJackBufferLabel->setText(juce::String(mJackClient.getBufferSize()) + " spls", juce::dontSendNotification);
    mJackInfoLabel->setText("I : " + juce::String(mJackClient.getNumberOutputs())
                                + " - O : " + juce::String(mJackClient.getNumberInputs()),
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
    juce::File const preset{ props->getValue("lastOpenPreset", "./not_saved_yet") };
    if (!preset.existsAsFile()) {
        openPreset(DEFAULT_PRESET_FILE.getFullPathName());
    } else {
        openPreset(props->getValue("lastOpenPreset"));
    }

    // Open the default speaker setup if lastOpenSpeakerSetup is not a valid file.
    juce::File const setup{ props->getValue("lastOpenSpeakerSetup", "./not_saved_yet") };
    if (!setup.existsAsFile()) {
        openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
    } else {
        openXmlFileSpeaker(props->getValue("lastOpenSpeakerSetup"));
    }

    // End layout and start refresh timer.
    resized();
    startTimerHz(24);

    // Start Splash screen.
#if NDEBUG
    if (SPLASH_SCREEN_FILE.exists()) {
        mSplashScreen.reset(
            new juce::SplashScreen("SpatGRIS2", juce::ImageFileFormat::loadFrom(SPLASH_SCREEN_FILE), true));
        mSplashScreen->deleteAfterDelay(juce::RelativeTime::seconds(4), false);
        mSplashScreen.release();
    }
#endif

    // Initialize the command manager for the menu bar items.
    auto & commandManager{ mMainWindow.getApplicationCommandManager() };
    commandManager.registerAllCommandsForTarget(this);

    // Restore last vertical divider position and speaker view cam distance.
    if (props->containsKey("sashPosition")) {
        auto const trueSize{ static_cast<int>(
            std::round(static_cast<double>(getWidth() - 3) * std::abs(props->getDoubleValue("sashPosition")))) };
        mVerticalLayout.setItemPosition(1, trueSize);
    }
}

//==============================================================================
MainContentComponent::~MainContentComponent()
{
    auto * props{ mApplicationProperties.getUserSettings() };
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
juce::Label * MainContentComponent::addLabel(juce::String const & s,
                                             juce::String const & tooltip,
                                             int const x,
                                             int const y,
                                             int const w,
                                             int const h,
                                             Component * into) const
{
    auto * lb{ new juce::Label{} };
    lb->setText(s, juce::NotificationType::dontSendNotification);
    lb->setTooltip(tooltip);
    lb->setJustificationType(juce::Justification::left);
    lb->setFont(mLookAndFeel.getFont());
    lb->setLookAndFeel(&mLookAndFeel);
    lb->setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
    lb->setBounds(x, y, w, h);
    into->addAndMakeVisible(lb);
    return lb;
}

//==============================================================================
juce::TextButton * MainContentComponent::addButton(juce::String const & s,
                                                   juce::String const & tooltip,
                                                   int const x,
                                                   int const y,
                                                   int const w,
                                                   int const h,
                                                   Component * into)
{
    auto * tb{ new juce::TextButton{} };
    tb->setTooltip(tooltip);
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
juce::ToggleButton * MainContentComponent::addToggleButton(juce::String const & s,
                                                           juce::String const & tooltip,
                                                           int const x,
                                                           int const y,
                                                           int const w,
                                                           int const h,
                                                           Component * into,
                                                           bool const toggle)
{
    auto * tb{ new juce::ToggleButton{} };
    tb->setTooltip(tooltip);
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
juce::TextEditor * MainContentComponent::addTextEditor(juce::String const & s,
                                                       juce::String const & emptyS,
                                                       juce::String const & tooltip,
                                                       int const x,
                                                       int const y,
                                                       int const w,
                                                       int const h,
                                                       Component * into,
                                                       int const wLab)
{
    auto * te{ new juce::TextEditor{} };
    te->setTooltip(tooltip);
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
juce::Slider * MainContentComponent::addSlider(juce::String const & /*s*/,
                                               juce::String const & tooltip,
                                               int const x,
                                               int const y,
                                               int const w,
                                               int const h,
                                               Component * into)
{
    auto * sd{ new juce::Slider{} };
    sd->setTooltip(tooltip);
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
juce::ComboBox * MainContentComponent::addComboBox(juce::String const & /*s*/,
                                                   juce::String const & tooltip,
                                                   int const x,
                                                   int const y,
                                                   int const w,
                                                   int const h,
                                                   Component * into)
{
    // TODO : naked new
    auto * cb{ new juce::ComboBox{} };
    cb->setTooltip(tooltip);
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
    juce::AlertWindow alert{ "Closing current preset !", "Do you want to save ?", juce::AlertWindow::InfoIcon };
    alert.setLookAndFeel(&mLookAndFeel);
    alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::deleteKey));
    alert.addButton("yes", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert.addButton("No", 2, juce::KeyPress(juce::KeyPress::escapeKey));

    auto const status{ alert.runModalLoop() };
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
    juce::File dir{ mApplicationProperties.getUserSettings()->getValue("lastPresetDirectory") };
    if (!dir.isDirectory()) {
        dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    }
    auto const filename{ juce::File{ mPathCurrentPreset }.getFileName() };

    juce::FileChooser fc("Choose a file to open...",
                         dir.getFullPathName() + "/" + filename,
                         "*.xml",
                         USE_OS_NATIVE_DIALOG_BOX);

    auto loaded{ false };
    if (fc.browseForFileToOpen()) {
        auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
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
        for (auto const * it : mSourceInputs) {
            if (it->getDirectOutChannel() != 0) {
                auto const directOutOutputPatches{ mJackClient.getDirectOutOutputPatches() };
                if (std::find(directOutOutputPatches.cbegin(), directOutOutputPatches.cend(), it->getDirectOutChannel())
                    == directOutOutputPatches.cend()) {
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
    juce::File dir{ mApplicationProperties.getUserSettings()->getValue("lastPresetDirectory") };
    if (!dir.isDirectory()) {
        dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    }
    auto const filename{ juce::File{ mPathCurrentPreset }.getFileName() };

    juce::FileChooser fc{ "Choose a file to save...",
                          dir.getFullPathName() + "/" + filename,
                          "*.xml",
                          USE_OS_NATIVE_DIALOG_BOX };

    if (fc.browseForFileToSave(true)) {
        auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
        savePreset(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleOpenSpeakerSetup()
{
    juce::File dir{ mApplicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory") };
    if (!dir.isDirectory()) {
        dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    }
    auto const filename{ juce::File{ mPathCurrentFileSpeaker }.getFileName() };

    juce::FileChooser fc{ "Choose a file to open...",
                          dir.getFullPathName() + "/" + filename,
                          "*.xml",
                          USE_OS_NATIVE_DIALOG_BOX };

    if (fc.browseForFileToOpen()) {
        auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
        juce::AlertWindow alert{ "Load Speaker Setup !",
                                 "You want to load : " + chosen + "\nEverything not saved will be lost !",
                                 juce::AlertWindow::WarningIcon };
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
    juce::File dir{ mApplicationProperties.getUserSettings()->getValue("lastSpeakerSetupDirectory") };
    if (!dir.isDirectory() || dir.getFullPathName().endsWith("/default_preset")) {
        dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    }
    auto const filename{ juce::File{ mPathCurrentFileSpeaker }.getFileName() };

    juce::FileChooser fc{ "Choose a file to save...",
                          dir.getFullPathName() + "/" + filename,
                          "*.xml",
                          USE_OS_NATIVE_DIALOG_BOX };

    if (fc.browseForFileToSave(true)) {
        auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
        saveSpeakerSetup(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleShowSpeakerEditWindow()
{
    juce::Rectangle<int> const result{ getScreenX() + mSpeakerViewComponent->getWidth() + 20,
                                       getScreenY() + 20,
                                       850,
                                       600 };
    if (mEditSpeakersWindow == nullptr) {
        auto const windowName = juce::String("Speakers Setup Edition - ")
                                + juce::String(MODE_SPAT_STRING[static_cast<int>(mJackClient.getMode())]) + " - "
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
    auto const * props{ mApplicationProperties.getUserSettings() };
    if (mPropertiesWindow == nullptr) {
        auto const bufferValue{ props->getIntValue("BufferValue", 1024) };
        auto const rateValue{ props->getIntValue("RateValue", 48000) };
        auto const fileFormat{ props->getIntValue("FileFormat", 0) };
        auto const fileConfig{ props->getIntValue("FileConfig", 0) };
        auto const attenuationDb{ props->getIntValue("AttenuationDB", 3) };
        auto const attenuationHz{ props->getIntValue("AttenuationHz", 3) };
        auto const oscInputPort{ props->getIntValue("OscInputPort", 18032) };

        mPropertiesWindow.reset(new PropertiesWindow{ *this,
                                                      mLookAndFeel,
                                                      mAlsaAvailableOutputDevices,
                                                      mAlsaOutputDevice,
                                                      RATE_VALUES.indexOf(juce::String(rateValue)),
                                                      BUFFER_SIZES.indexOf(juce::String(bufferValue)),
                                                      fileFormat,
                                                      fileConfig,
                                                      attenuationDb,
                                                      attenuationHz,
                                                      oscInputPort });
    }
    auto const height{ mAlsaAvailableOutputDevices.isEmpty() ? 420 : 450 };
    juce::Rectangle<int> const result{ mSpeakerViewComponent->getWidth() / 2 + getScreenX() - 150,
                                       mSpeakerViewComponent->getHeight() / 2 + getScreenY() - 75,
                                       270,
                                       height };
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
    if (!mAboutWindow) {
        mAboutWindow.reset(new AboutWindow{ "About SpatGRIS", mLookAndFeel, *this });
    }
}

//==============================================================================
void MainContentComponent::handleOpenManual()
{
    if (SERVER_GRIS_MANUAL_FILE.exists()) {
        juce::Process::openDocument("file:" + SERVER_GRIS_MANUAL_FILE.getFullPathName(), juce::String());
    }
}

//==============================================================================
void MainContentComponent::handleShowNumbers()
{
    setShowNumbers(!mIsNumbersShown);
}

//==============================================================================
void MainContentComponent::setShowNumbers(bool const state)
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
void MainContentComponent::setShowSpeakers(bool const state)
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
void MainContentComponent::setShowTriplets(bool const state)
{
    if (getModeSelected() == ModeSpatEnum::LBAP && state == true) {
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
    for (auto const & triplet : getTriplets()) {
        auto const * spk1 = getSpeakerFromOutputPatch(triplet.id1);
        auto const * spk2 = getSpeakerFromOutputPatch(triplet.id2);
        auto const * spk3 = getSpeakerFromOutputPatch(triplet.id3);

        if (spk1 == nullptr || spk2 == nullptr || spk3 == nullptr) {
            return false;
        }
    }

    return true;
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
    auto hue = 0.0f;
    auto const inc = 1.0f / static_cast<float>(mSourceInputs.size() + 1);
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
void MainContentComponent::getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo & result)
{
    const juce::String generalCategory("General");

    switch (commandId) {
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
juce::PopupMenu MainContentComponent::getMenuForIndex(int /*menuIndex*/, const juce::String & menuName)
{
    juce::ApplicationCommandManager * commandManager = &mMainWindow.getApplicationCommandManager();

    juce::PopupMenu menu;

    if (menuName == "File") {
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
        if (mJackClient.getVbapDimensions() == 3) {
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
void MainContentComponent::menuItemSelected(int /*menuItemID*/, int /*topLevelMenuIndex*/)
{
    // ???
}

//==============================================================================
// Exit functions.
bool MainContentComponent::isPresetModified() const
{
    juce::File const xmlFile{ mPathCurrentPreset };
    juce::XmlDocument xmlDoc{ xmlFile };
    auto const savedState{ xmlDoc.getDocumentElement() };
    if (!savedState) {
        return true;
    }

    auto const currentState{ std::make_unique<juce::XmlElement>("ServerGRIS_Preset") };
    getPresetData(currentState.get());

    if (!savedState->isEquivalentTo(currentState.get(), true)) {
        return true;
    }

    return false;
}

//==============================================================================
bool MainContentComponent::exitApp()
{
    auto exitV{ 2 };

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
            juce::File dir{ mApplicationProperties.getUserSettings()->getValue("lastPresetDirectory") };
            if (!dir.isDirectory()) {
                dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
            }
            auto const filename{ juce::File(mPathCurrentPreset).getFileName() };

            juce::FileChooser fc("Choose a file to save...",
                                 dir.getFullPathName() + "/" + filename,
                                 "*.xml",
                                 USE_OS_NATIVE_DIALOG_BOX);

            if (fc.browseForFileToSave(true)) {
                auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
                savePreset(chosen);
            } else {
                exitV = 0;
            }
        }
    }

    return exitV != 0;
}

//==============================================================================
void MainContentComponent::connectionClientJack(juce::String const & clientName, bool const conn)
{
    unsigned int maxPort{};
    for (auto const & cli : mJackClient.getClients()) {
        if (cli.portEnd > maxPort) {
            maxPort = cli.portEnd;
        }
    }
    if (maxPort > static_cast<unsigned int>(mAddInputsTextEditor->getTextValue().toString().getIntValue())) {
        mAddInputsTextEditor->setText(juce::String{ maxPort }, juce::dontSendNotification);
        textEditorReturnKeyPressed(*mAddInputsTextEditor);
    }
    mJackClient.setProcessBlockOn(false);
    mJackClient.connectionClient(clientName, conn);
    mJackClient.setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::selectSpeaker(int const idS) const
{
    for (int i{}; i < mSpeakers.size(); ++i) {
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
void MainContentComponent::selectTripletSpeaker(int const idS)
{
    auto countS{ std::count_if(mSpeakers.begin(), mSpeakers.end(), [](Speaker const * speaker) {
        return speaker->isSelected();
    }) };

    if (!mSpeakers[idS]->isSelected() && countS < 3) {
        mSpeakers[idS]->selectSpeaker();
        countS += 1;
    } else {
        mSpeakers[idS]->unSelectSpeaker();
    }

    if (countS == 3) {
        auto i1{ -1 };
        auto i2{ -1 };
        auto i3{ -1 };
        for (int i{}; i < mSpeakers.size(); ++i) {
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
            Triplet const tri{ i1, i2, i3 };
            auto posDel{ -1 };
            if (tripletExists(tri, posDel)) {
                mTriplets.erase(mTriplets.begin() + posDel);
            } else {
                mTriplets.push_back(tri);
            }
        }
    }
}

//==============================================================================
bool MainContentComponent::tripletExists(Triplet const & tri, int & pos) const
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
    auto id{ 1 };
    for (auto * it : mSpeakers) {
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
    int maxId{};
    for (auto const * it : mSpeakers) {
        if (it->getIdSpeaker() > maxId) {
            maxId = it->getIdSpeaker();
        }
    }
    return maxId;
}

//==============================================================================
int MainContentComponent::getMaxSpeakerOutputPatch() const
{
    int maxOut{};
    for (auto const * it : mSpeakers) {
        if (it->getOutputPatch() > maxOut) {
            maxOut = it->getOutputPatch();
        }
    }
    return maxOut;
}

//==============================================================================
void MainContentComponent::addSpeaker(int const sortColumnId, bool const isSortedForwards)
{
    auto const newId{ getMaxSpeakerId() + 1 };

    mSpeakerLocks.lock();
    mSpeakers.add(new Speaker{ *this, newId, newId, 0.0f, 0.0f, 1.0f });

    if (sortColumnId == 1 && isSortedForwards) {
        for (int i{}; i < mSpeakers.size(); ++i) {
            mSpeakers[i]->setSpeakerId(i + 1);
        }
    } else if (sortColumnId == 1 && !isSortedForwards) {
        for (int i{}; i < mSpeakers.size(); ++i) {
            mSpeakers[i]->setSpeakerId(mSpeakers.size() - i);
        }
    }
    mSpeakerLocks.unlock();

    mJackClient.setProcessBlockOn(false);
    mJackClient.addOutput(mSpeakers.getLast()->getOutputPatch());
    mJackClient.setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::insertSpeaker(int const position, int const sortColumnId, bool const isSortedForwards)
{
    auto const newPosition{ position + 1 };
    auto const newOut{ getMaxSpeakerOutputPatch() + 1 };
    auto newId{ getMaxSpeakerId() + 1 };

    mSpeakerLocks.lock();
    if (sortColumnId == 1 && isSortedForwards) {
        newId = mSpeakers[position]->getIdSpeaker() + 1;
        mSpeakers.insert(newPosition, new Speaker{ *this, newId, newOut, 0.0f, 0.0f, 1.0f });
        for (int i{}; i < mSpeakers.size(); ++i) {
            mSpeakers.getUnchecked(i)->setSpeakerId(i + 1);
        }
    } else if (sortColumnId == 1 && !isSortedForwards) {
        newId = mSpeakers[position]->getIdSpeaker() - 1;
        mSpeakers.insert(newPosition, new Speaker{ *this, newId, newOut, 0.0f, 0.0f, 1.0f });
        for (int i{}; i < mSpeakers.size(); ++i) {
            mSpeakers.getUnchecked(i)->setSpeakerId(mSpeakers.size() - i);
        }
    } else {
        mSpeakers.insert(newPosition, new Speaker{ *this, newId, newOut, 0.0f, 0.0f, 1.0f });
    }
    mSpeakerLocks.unlock();

    mJackClient.setProcessBlockOn(false);
    mJackClient.clearOutput();
    for (auto * it : mSpeakers) {
        mJackClient.addOutput(it->getOutputPatch());
    }
    mJackClient.setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::removeSpeaker(int const idSpeaker)
{
    mJackClient.removeOutput(idSpeaker);
    mSpeakerLocks.lock();
    mSpeakers.remove(idSpeaker, true);
    mSpeakerLocks.unlock();
}

//==============================================================================
bool MainContentComponent::isRadiusNormalized() const
{
    auto const mode{ mJackClient.getMode() };
    if (mode == ModeSpatEnum::VBAP || mode == ModeSpatEnum::VBAP_HRTF)
        return true;
    else
        return false;
}

//==============================================================================
void MainContentComponent::updateInputJack(int inInput, Input & inp)
{
    auto const mode{ mJackClient.getMode() };
    auto & si = mJackClient.getSourcesIn()[inInput];

    if (mode == ModeSpatEnum::LBAP) {
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

    if (mode == ModeSpatEnum::VBAP || mode == ModeSpatEnum::VBAP_HRTF) {
        mJackClient.getVbapSourcesToUpdate()[inInput] = 1;
    }
}

//==============================================================================
void MainContentComponent::setTripletsFromVbap()
{
    clearTriplets();
    for (auto const & it : mJackClient.getVbapTriplets()) {
        jassert(it.size() == 3u);
        Triplet const tri{ it[0], it[1], it[2] };
        mTriplets.push_back(tri);
    }
}

//==============================================================================
Speaker const * MainContentComponent::getSpeakerFromOutputPatch(int const out) const
{
    for (auto const * it : mSpeakers) {
        if (it->getOutputPatch() == out && !it->isDirectOut()) {
            return it;
        }
    }
    return nullptr;
}

//==============================================================================
Speaker * MainContentComponent::getSpeakerFromOutputPatch(int const out)
{
    for (auto * it : mSpeakers) {
        if (it->getOutputPatch() == out && !it->isDirectOut()) {
            return it;
        }
    }
    return nullptr;
}

//==============================================================================
static void linkwitzRileyComputeVariables(double const freq, double const sr, double ** coefficients, int const length)
{
    auto const wc{ 2.0 * juce::MathConstants<double>::pi * freq };
    auto const wc2{ wc * wc };
    auto const wc3{ wc2 * wc };
    auto const wc4{ wc2 * wc2 };
    auto const k{ wc / std::tan(juce::MathConstants<double>::pi * freq / sr) };
    auto const k2{ k * k };
    auto const k3{ k2 * k };
    auto const k4{ k2 * k2 };
    static auto constexpr SQRT2{ juce::MathConstants<double>::sqrt2 };
    auto const sqTmp1{ SQRT2 * wc3 * k };
    auto const sqTmp2{ SQRT2 * wc * k3 };
    auto const aTmp{ 4.0 * wc2 * k2 + 2.0 * sqTmp1 + k4 + 2.0 * sqTmp2 + wc4 };
    auto const k4ATmp{ k4 / aTmp };

    *coefficients = static_cast<double *>(malloc(length * sizeof(double)));

    /* common */
    auto const b1{ (4.0 * (wc4 + sqTmp1 - k4 - sqTmp2)) / aTmp };
    auto const b2{ (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / aTmp };
    auto const b3{ (4.0 * (wc4 - sqTmp1 + sqTmp2 - k4)) / aTmp };
    auto const b4{ (k4 - 2.0 * sqTmp1 + wc4 - 2.0 * sqTmp2 + 4.0 * wc2 * k2) / aTmp };

    /* highpass */
    auto const ha0{ k4ATmp };
    auto const ha1{ -4.0 * k4ATmp };
    auto const ha2{ 6.0 * k4ATmp };

    (*coefficients)[0] = b1;
    (*coefficients)[1] = b2;
    (*coefficients)[2] = b3;
    (*coefficients)[3] = b4;
    (*coefficients)[4] = ha0;
    (*coefficients)[5] = ha1;
    (*coefficients)[6] = ha2;
}

//==============================================================================
float MainContentComponent::getLevelsAlpha(int const indexLevel) const
{
    auto const level{ mJackClient.getLevelsIn(indexLevel) };
    if (level > 0.0001f) {
        // -80 dB
        return 1.0f;
    }
    return std::sqrt(level * 10000.0f);
}

//==============================================================================
float MainContentComponent::getSpeakerLevelsAlpha(int const indexLevel) const
{
    auto const level{ mJackClient.getLevelsOut(indexLevel) };
    float alpha;
    if (level > 0.001f) {
        // -60 dB
        alpha = 1.0f;
    } else {
        alpha = std::sqrt(level * 1000.0f);
    }
    if (alpha < 0.6f) {
        alpha = 0.6f;
    }
    return alpha;
}

//==============================================================================
bool MainContentComponent::updateLevelComp()
{
    if (mSpeakers.isEmpty()) {
        return false;
    }

    auto dimensions{ 2 };
    int directOutSpeakers{};

    // Test for a 2-D or 3-D configuration.
    auto zenith{ -1.0f };
    for (auto const * it : mSpeakers) {
        if (it->isDirectOut()) {
            directOutSpeakers++;
        } else if (zenith == -1.0f) {
            zenith = it->getAziZenRad().y;
        } else if (it->getAziZenRad().y < (zenith - 4.9f) || it->getAziZenRad().y > (zenith + 4.9f)) {
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
    std::vector<int> tempOut;
    for (auto const * speaker : mSpeakers) {
        if (!speaker->isDirectOut()) {
            tempOut.push_back(speaker->getOutputPatch());
        }
    }

    std::sort(tempOut.begin(), tempOut.end());
    for (size_t i{}; i < tempOut.size() - size_t{ 1u }; ++i) {
        if (tempOut[i] == tempOut[i + 1u]) {
            juce::AlertWindow alert{ "Duplicated Output Numbers!    ",
                                     "Some output numbers are used more than once. Do you want to continue anyway?    "
                                     "\nIf you continue, you may have to fix your speaker setup before using it!   ",
                                     juce::AlertWindow::WarningIcon };
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

    mJackClient.setProcessBlockOn(false);
    mJackClient.setMaxOutputPatch(0u);

    // Save mute/solo/directOut states
    std::array<bool, MAX_INPUTS> inputsIsMuted{};
    std::array<bool, MAX_INPUTS> inputsIsSolo{};
    auto const soloIn{ mJackClient.getSoloIn() };
    std::array<int, MAX_INPUTS> directOuts{};
    auto const & sourcesIn{ mJackClient.getSourcesIn() };

    for (size_t i{}; i < MAX_INPUTS; ++i) {
        inputsIsMuted[i] = sourcesIn[i].isMuted;
        inputsIsSolo[i] = sourcesIn[i].isSolo;
        directOuts[i] = sourcesIn[i].directOut;
    }

    std::array<bool, MAX_OUTPUTS> outputsIsMuted{};
    std::array<bool, MAX_OUTPUTS> outputsIsSolo{};
    auto const soloOut{ mJackClient.getSoloOut() };
    auto const & speakersOut{ mJackClient.getSpeakersOut() };
    for (size_t i{}; i < MAX_OUTPUTS; ++i) {
        outputsIsMuted[i] = speakersOut[i].isMuted;
        outputsIsSolo[i] = speakersOut[i].isSolo;
    }

    // Cleanup speakers output patch
    for (auto & it : mJackClient.getSpeakersOut()) {
        it.outputPatch = 0;
    }

    // Create outputs.
    int i{};
    auto x{ 2 };
    auto const mode{ mJackClient.getMode() };
    for (auto * speaker : mSpeakers) {
        juce::Rectangle<int> level{ x, 4, VU_METER_WIDTH_IN_PIXELS, 200 };
        speaker->getVuMeter()->setBounds(level);
        speaker->getVuMeter()->resetClipping();
        mOutputsUiBox->getContent()->addAndMakeVisible(speaker->getVuMeter());
        speaker->getVuMeter()->repaint();

        x += VU_METER_WIDTH_IN_PIXELS;

        if (mode == ModeSpatEnum::VBAP || mode == ModeSpatEnum::VBAP_HRTF) {
            speaker->normalizeRadius();
        }

        SpeakerOut so;
        so.id = speaker->getOutputPatch();
        so.x = speaker->getCoordinate().x;
        so.y = speaker->getCoordinate().y;
        so.z = speaker->getCoordinate().z;
        so.azimuth = speaker->getAziZenRad().x;
        so.zenith = speaker->getAziZenRad().y;
        so.radius = speaker->getAziZenRad().z;
        so.outputPatch = speaker->getOutputPatch();
        so.directOut = speaker->isDirectOut();

        mJackClient.getSpeakersOut()[i++] = so;

        if (static_cast<unsigned int>(speaker->getOutputPatch()) > mJackClient.getMaxOutputPatch())
            mJackClient.setMaxOutputPatch(speaker->getOutputPatch());
    }

    // Set user gain and highpass filter cutoff frequency for each speaker.
    for (auto const * speaker : mSpeakers) {
        auto & speakerOut{ mJackClient.getSpeakersOut()[speaker->getOutputPatch() - 1] };
        speakerOut.gain = std::pow(10.0f, speaker->getGain() * 0.05f);
        if (speaker->getHighPassCutoff() > 0.0f) {
            double * coefficients;
            linkwitzRileyComputeVariables(static_cast<double>(speaker->getHighPassCutoff()),
                                          static_cast<double>(mSamplingRate),
                                          &coefficients,
                                          7);
            speakerOut.b1 = coefficients[0];
            speakerOut.b2 = coefficients[1];
            speakerOut.b3 = coefficients[2];
            speakerOut.b4 = coefficients[3];
            speakerOut.ha0 = coefficients[4];
            speakerOut.ha1 = coefficients[5];
            speakerOut.ha2 = coefficients[6];
            speakerOut.hpActive = true;
            free(coefficients);
            // TODO : naked free
        }
    }

    i = 0;
    x = 2;
    mInputLocks.lock();
    std::vector<int> directOutMenuItems{};
    for (auto const * speaker : mSpeakers) {
        if (speaker->isDirectOut()) {
            directOutMenuItems.push_back(speaker->getOutputPatch());
        }
    }
    for (auto * input : mSourceInputs) {
        juce::Rectangle<int> level{ x, 4, VU_METER_WIDTH_IN_PIXELS, 200 };
        input->getVuMeter()->setBounds(level);
        if (input->isInput()) { // TODO : wut?
            input->getVuMeter()->updateDirectOutMenu(directOutMenuItems);
        }
        input->getVuMeter()->resetClipping();
        mInputsUiBox->getContent()->addAndMakeVisible(input->getVuMeter());
        input->getVuMeter()->repaint();

        x += VU_METER_WIDTH_IN_PIXELS;

        SourceIn sourceIn;
        sourceIn.id = input->getId();
        sourceIn.radAzimuth = input->getAzimuth();
        sourceIn.radElevation = juce::MathConstants<float>::halfPi - input->getZenith();
        sourceIn.azimuth = input->getAzimuth();
        sourceIn.zenith = input->getZenith();
        sourceIn.radius = input->getRadius();
        sourceIn.gain = 0.0f;
        mJackClient.getSourcesIn()[i++] = sourceIn;
    }

    mInputLocks.unlock();

    if (mEditSpeakersWindow != nullptr) {
        mEditSpeakersWindow->updateWinContent();
    }

    mOutputsUiBox->repaint();
    resized();

    // Temporarily remove direct out speakers to construct vbap or lbap algorithm.
    i = 0;
    std::vector<Speaker *> tempListSpeaker{};
    tempListSpeaker.resize(mSpeakers.size());
    for (auto * speaker : mSpeakers) {
        if (!speaker->isDirectOut()) {
            tempListSpeaker[i++] = speaker;
        }
    }
    tempListSpeaker.resize(i);

    auto returnValue{ false };
    if (mode == ModeSpatEnum::VBAP || mode == ModeSpatEnum::VBAP_HRTF) {
        mJackClient.setVbapDimensions(dimensions);
        if (dimensions == 2) {
            setShowTriplets(false);
        }
        returnValue = mJackClient.initSpeakersTriplet(tempListSpeaker, dimensions, mNeedToComputeVbap);

        if (returnValue) {
            setTripletsFromVbap();
            mNeedToComputeVbap = false;
        } else {
            juce::AlertWindow alert{ "Not a valid DOME 3-D configuration!    ",
                                     "Maybe you want to open it in CUBE mode? Reload the default speaker setup...    ",
                                     juce::AlertWindow::WarningIcon };
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
            alert.runModalLoop();
            openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE.getFullPathName());
            return false;
        }
    } else if (mode == ModeSpatEnum::LBAP) {
        setShowTriplets(false);
        returnValue = mJackClient.lbapSetupSpeakerField(tempListSpeaker);
    }

    // Restore mute/solo/directOut states
    mJackClient.setSoloIn(soloIn);
    for (size_t sourceInIndex{}; sourceInIndex < MAX_INPUTS; ++sourceInIndex) {
        auto & sourceIn{ mJackClient.getSourcesIn()[sourceInIndex] };
        sourceIn.isMuted = inputsIsMuted[sourceInIndex];
        sourceIn.isSolo = inputsIsSolo[sourceInIndex];
        sourceIn.directOut = directOuts[sourceInIndex];
    }
    mInputLocks.lock();
    for (int sourceInputIndex{}; sourceInputIndex < mSourceInputs.size(); sourceInputIndex++) {
        mSourceInputs[sourceInputIndex]->setDirectOutChannel(directOuts[sourceInputIndex]);
    }
    mInputLocks.unlock();

    mJackClient.setSoloOut(soloOut);
    for (size_t speakerOutIndex{}; speakerOutIndex < MAX_OUTPUTS; ++speakerOutIndex) {
        auto & speakerOut{ mJackClient.getSpeakersOut()[speakerOutIndex] };
        speakerOut.isMuted = outputsIsMuted[speakerOutIndex];
        speakerOut.isSolo = outputsIsSolo[speakerOutIndex];
    }

    mJackClient.setProcessBlockOn(true);

    return returnValue;
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
    mJackClient.getSourcesIn()[index].isMuted = mute;
}

//==============================================================================
void MainContentComponent::muteOutput(int const id, bool const mute)
{
    auto const index{ id - 1 };
    mJackClient.getSpeakersOut()[index].isMuted = mute;
}

//==============================================================================
void MainContentComponent::soloInput(int const id, bool const solo)
{
    auto & sourcesIn{ mJackClient.getSourcesIn() };
    auto const index{ id - 1 };
    sourcesIn[index].isSolo = solo;

    mJackClient.setSoloIn(false);
    for (unsigned int i = 0; i < MAX_INPUTS; i++) {
        if (sourcesIn[i].isSolo) {
            mJackClient.setSoloIn(true);
            break;
        }
    }
}

//==============================================================================
void MainContentComponent::soloOutput(int const id, bool const solo)
{
    auto const index{ id - 1 };
    auto & speakersOut{ mJackClient.getSpeakersOut() };
    speakersOut[index].isSolo = solo;

    mJackClient.setSoloOut(false);
    for (unsigned int i = 0; i < MAX_OUTPUTS; i++) {
        if (speakersOut[i].isSolo) {
            mJackClient.setSoloOut(true);
            break;
        }
    }
}

//==============================================================================
void MainContentComponent::setDirectOut(int const id, int const chn)
{
    auto const index{ id - 1 };
    mJackClient.getSourcesIn()[index].directOut = chn;
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
                    mJackClient.setMode(ModeSpatEnum::VBAP_HRTF);
                    mModeSpatCombo->setSelectedId(static_cast<int>(ModeSpatEnum::VBAP_HRTF) + 1,
                                                  juce::NotificationType::dontSendNotification);
                } else if (path.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) == 0) {
                    mJackClient.setMode(ModeSpatEnum::STEREO);
                    mModeSpatCombo->setSelectedId(static_cast<int>(ModeSpatEnum::STEREO) + 1,
                                                  juce::NotificationType::dontSendNotification);
                } else if (!isNewSameAsOld && oldPath.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
                           && oldPath.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
                    auto const spatMode = mainXmlElem->getIntAttribute("SpatMode");
                    mJackClient.setMode(static_cast<ModeSpatEnum>(spatMode));
                    mModeSpatCombo->setSelectedId(spatMode + 1, juce::NotificationType::dontSendNotification);
                } else if (!isNewSameAsLastSetup) {
                    auto const spatMode = mainXmlElem->getIntAttribute("SpatMode");
                    mJackClient.setMode(static_cast<ModeSpatEnum>(spatMode));
                    mModeSpatCombo->setSelectedId(spatMode + 1, juce::NotificationType::dontSendNotification);
                }

                auto const loadSetupFromXyz{ isNewSameAsOld && mJackClient.getMode() == ModeSpatEnum::LBAP };

                setNameConfig();
                mJackClient.setProcessBlockOn(false);
                mJackClient.clearOutput();
                mJackClient.setMaxOutputPatch(0);
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

                                mSpeakers.add(new Speaker{ *this,
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
                                mJackClient.addOutput(static_cast<unsigned int>(spk->getIntAttribute("OutputPatch")));
                            }
                        }
                    }
                    if (ring->hasTagName("triplet")) {
                        Triplet const triplet{ ring->getIntAttribute("id1"),
                                               ring->getIntAttribute("id2"),
                                               ring->getIntAttribute("id3") };
                        mTriplets.push_back(triplet);
                    }
                }
                mJackClient.setProcessBlockOn(true);
                ok = true;
            } else {
                juce::String msg;
                if (mainXmlElem->hasTagName("ServerGRIS_Preset")) {
                    msg = "You are trying to open a Server document, and not a Speaker Setup !";
                } else {
                    msg = "Your file is corrupted !\n" + xmlDoc.getLastParseError();
                }
                juce::AlertWindow alert{ "Error in Load Speaker Setup !", msg, juce::AlertWindow::WarningIcon };
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
        auto const mode{ mJackClient.getMode() };
        if (mode != ModeSpatEnum::VBAP_HRTF && mode != ModeSpatEnum::STEREO) {
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
void MainContentComponent::setTitle() const
{
    auto const title{ juce::String{ "SpatGRIS v" } + STRING(JUCE_APP_VERSION) + " - "
                      + juce::File{ mPathCurrentPreset }.getFileName() };
    mMainWindow.setName(title);
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
    mJackClient.setProcessBlockOn(true);
}

//==============================================================================
void MainContentComponent::openPreset(juce::String const & path)
{
    juce::String msg;
    mJackClient.setProcessBlockOn(false);
    juce::File const xmlFile{ path };
    juce::XmlDocument xmlDoc{ xmlFile };
    auto const mainXmlElem{ xmlDoc.getDocumentElement() };
    if (!mainXmlElem) {
        juce::AlertWindow alert{ "Error in Open Preset !",
                                 "Your file is corrupted !\n" + path.toStdString() + "\n"
                                     + xmlDoc.getLastParseError().toStdString(),
                                 juce::AlertWindow::WarningIcon };
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
                auto const angleX{ static_cast<float>(mainXmlElem->getDoubleAttribute("CamAngleX")) };
                auto const angleY{ static_cast<float>(mainXmlElem->getDoubleAttribute("CamAngleY")) };
                auto const distance{ static_cast<float>(mainXmlElem->getDoubleAttribute("CamDistance")) };
                mSpeakerViewComponent->setCamPosition(angleX, angleY, distance);
            } else {
                mSpeakerViewComponent->setCamPosition(80.0f, 25.0f, 22.0f); // TODO: named constants ?
            }

            // Update
            textEditorReturnKeyPressed(*mAddInputsTextEditor);
            sliderValueChanged(mMasterGainOutSlider.get());
            sliderValueChanged(mInterpolationSlider.get());

            juce::File const speakerSetup{ mPathCurrentFileSpeaker };
            if (!mPathCurrentFileSpeaker.startsWith("/")) {
                mPathCurrentFileSpeaker
                    = DEFAULT_PRESET_DIRECTORY.getChildFile(mPathCurrentFileSpeaker).getFullPathName();
            }

            for (auto * input{ mainXmlElem->getFirstChildElement() }; input != nullptr;
                 input = input->getNextElement()) {
                if (input->hasTagName("Input")) {
                    for (auto * it : mSourceInputs) {
                        if (it->getId() == input->getIntAttribute("Index")) {
                            it->setColor(juce::Colour::fromFloatRGBA(static_cast<float>(input->getDoubleAttribute("R")),
                                                                     static_cast<float>(input->getDoubleAttribute("G")),
                                                                     static_cast<float>(input->getDoubleAttribute("B")),
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

    mJackClient.setPinkNoiseActive(false);
    mJackClient.setProcessBlockOn(true);

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

    for (auto const * sourceInput : mSourceInputs) {
        auto * xmlInput{ new juce::XmlElement{ "Input" } };
        xmlInput->setAttribute("Index", sourceInput->getId());
        xmlInput->setAttribute("R", sourceInput->getColor().x);
        xmlInput->setAttribute("G", sourceInput->getColor().y);
        xmlInput->setAttribute("B", sourceInput->getColor().z);
        xmlInput->setAttribute("DirectOut", juce::String(sourceInput->getDirectOutChannel()));
        xml->addChildElement(xmlInput);
    }
}

//==============================================================================
void MainContentComponent::savePreset(juce::String const & path)
{
    juce::File const xmlFile{ path };
    auto const xml{ std::make_unique<juce::XmlElement>("ServerGRIS_Preset") };
    getPresetData(xml.get());
    [[maybe_unused]] auto success{ xml->writeTo(xmlFile) };
    jassert(success);
    success = xmlFile.create();
    jassert(success);
    mPathCurrentPreset = path;
    mApplicationProperties.getUserSettings()->setValue(
        "lastPresetDirectory",
        juce::File{ mPathCurrentPreset }.getParentDirectory().getFullPathName());
    setTitle();
}

//==============================================================================
void MainContentComponent::saveSpeakerSetup(juce::String const & path)
{
    mPathCurrentFileSpeaker = path;
    juce::File const xmlFile{ path };
    juce::XmlElement xml{ "SpeakerSetup" };

    xml.setAttribute("Name", mNameConfig);
    xml.setAttribute("Dimension", 3);
    xml.setAttribute("SpatMode", static_cast<int>(getModeSelected()));

    auto * xmlRing{ new juce::XmlElement{ "Ring" } };

    for (auto const * it : mSpeakers) {
        auto * xmlInput{ new juce::XmlElement{ "Speaker" } };
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

    for (auto const & triplet : mTriplets) {
        auto * xmlInput{ new juce::XmlElement{ "triplet" } };
        xmlInput->setAttribute("id1", triplet.id1);
        xmlInput->setAttribute("id2", triplet.id2);
        xmlInput->setAttribute("id3", triplet.id3);
        xml.addChildElement(xmlInput);
    }

    [[maybe_unused]] auto success{ xml.writeTo(xmlFile) };
    jassert(success);
    success = xmlFile.create();
    jassert(success);

    mApplicationProperties.getUserSettings()->setValue(
        "lastSpeakerSetupDirectory",
        juce::File{ mPathCurrentFileSpeaker }.getParentDirectory().getFullPathName());

    mNeedToSaveSpeakerSetup = false;

    auto const mode{ mJackClient.getMode() };
    if (mode != ModeSpatEnum::VBAP_HRTF && mode != ModeSpatEnum::STEREO) {
        if (mPathCurrentFileSpeaker.compare(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName()) != 0
            && mPathCurrentFileSpeaker.compare(STEREO_SPEAKER_SETUP_FILE.getFullPathName()) != 0) {
            mPathLastVbapSpeakerSetup = mPathCurrentFileSpeaker;
        }
    }

    setNameConfig();
}

//==============================================================================
void MainContentComponent::saveProperties(juce::String const & device,
                                          int const rate,
                                          int const buff,
                                          int const fileFormat,
                                          int const fileConfig,
                                          int const attenuationDb,
                                          int const attenuationHz,
                                          int oscPort)
{
    auto * props{ mApplicationProperties.getUserSettings() };

    auto const deviceValue{ props->getValue("AlsaOutputDevice", "") };
    auto bufferValue{ props->getIntValue("BufferValue", 1024) };
    auto rateValue{ props->getIntValue("RateValue", 48000) };
    auto const oscInputPort{ props->getIntValue("OscInputPort", 18032) };

    if (bufferValue == 0) {
        bufferValue = 1024;
    }
    if (rateValue == 0) {
        rateValue = 48000;
    }

    if (device.compare(deviceValue) != 0 || rate != rateValue || buff != bufferValue) {
        juce::AlertWindow alert("You Need to Restart SpatGRIS!",
                                "New settings will be effective on next launch of the SpatGRIS.",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Cancel", 0);
        alert.addButton("Ok", 1, juce::KeyPress{ juce::KeyPress::returnKey });
        if (alert.runModalLoop() != 0) {
            props->setValue("AlsaOutputDevice", device);
            props->setValue("BufferValue", buff);
            props->setValue("RateValue", rate);
        }
    }

    // Handle OSC Input Port
    if (oscPort < 0 || oscPort > 65535) {
        oscPort = 18032;
    }
    if (oscPort != oscInputPort) {
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
    mJackClient.setRecordFormat(fileFormat);
    props->setValue("FileFormat", fileFormat);

    mJackClient.setRecordFileConfig(fileConfig);
    props->setValue("FileConfig", fileConfig);

    // Handle CUBE distance attenuation
    auto const linGain{ std::pow(10.0f, ATTENUATION_DB[attenuationDb].getFloatValue() * 0.05f) };
    mJackClient.setAttenuationDb(linGain);
    props->setValue("AttenuationDB", attenuationDb);

    auto const coefficient{ std::exp(-juce::MathConstants<float>::twoPi
                                     * ATTENUATION_CUTOFFS[attenuationHz].getFloatValue()
                                     / static_cast<float>(mJackClient.getSampleRate())) };
    mJackClient.setAttenuationHz(coefficient);
    props->setValue("AttenuationHz", attenuationHz);

    mApplicationProperties.saveIfNeeded();
}

//==============================================================================
void MainContentComponent::timerCallback()
{
    auto const cpuLoad{ static_cast<int>(std::round(mJackClient.getCpuUsed() * 100.0f)) };
    mJackLoadLabel->setText(juce::String{ cpuLoad } + " %", juce::dontSendNotification);
    auto seconds{ static_cast<int>(mJackClient.getIndexRecord() / mJackClient.getSampleRate()) };
    auto const minute{ seconds / 60 % 60 };
    seconds = seconds % 60;
    auto const timeRecorded{ ((minute < 10) ? "0" + juce::String(minute) : juce::String(minute)) + " : "
                             + ((seconds < 10) ? "0" + juce::String(seconds) : juce::String(seconds)) };
    mTimeRecordedLabel->setText(timeRecorded, juce::dontSendNotification);

    if (mStartRecordButton->getToggleState()) {
        mStartRecordButton->setToggleState(false, juce::dontSendNotification);
    }

    if (mJackClient.isSavingRun()) {
        mStartRecordButton->setButtonText("Stop");
    } else {
        mStartRecordButton->setButtonText("Record");
    }

    if (mIsRecording && !mJackClient.isRecording()) {
        auto const & recorders{ mJackClient.getRecorders() };
        auto const isReadyToMerge{ std::all_of(
            recorders.begin(),
            recorders.end(),
            [](AudioRecorder const & recorder) -> bool { return !recorder.backgroundThread.isThreadRunning(); }) };

        if (isReadyToMerge) {
            mIsRecording = false;
            if (mJackClient.getRecordFileConfig()) {
                mJackClient.setProcessBlockOn(false);
                auto * renderer{ new AudioRenderer{} };
                renderer->prepareRecording(mJackClient.getRecordingPath(),
                                           mJackClient.getOutputFileNames(),
                                           mJackClient.getSampleRate());
                renderer->runThread();
                mJackClient.setProcessBlockOn(true);
            }
        }
    }

    if (mJackClient.isOverloaded()) {
        mJackLoadLabel->setColour(juce::Label::backgroundColourId, juce::Colours::darkred);
    } else {
        mJackLoadLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    }

    for (auto * sourceInput : mSourceInputs) {
        sourceInput->getVuMeter()->update();
    }

    for (auto * speaker : mSpeakers) {
        speaker->getVuMeter()->update();
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
        auto const numOfInputs{ static_cast<unsigned>(mAddInputsTextEditor->getTextValue().toString().getIntValue()) };
        if (numOfInputs < 1) {
            mAddInputsTextEditor->setText("1");
        }
        if (numOfInputs > MAX_INPUTS) {
            mAddInputsTextEditor->setText(juce::String(MAX_INPUTS));
        }

        if (mJackClient.getInputPorts().size() != numOfInputs) {
            mJackClient.setProcessBlockOn(false);
            mJackClient.addRemoveInput(numOfInputs);
            mJackClient.setProcessBlockOn(true);

            mInputLocks.lock();
            auto addInput{ false };
            auto const numInputPorts{ static_cast<int>(mJackClient.getInputPorts().size()) };
            for (int i{}; i < numInputPorts; ++i) {
                if (i >= mSourceInputs.size()) {
                    mSourceInputs.add(new Input{ *this, mSmallLookAndFeel, i + 1 });
                    addInput = true;
                }
            }
            if (!addInput) {
                auto const listSourceInputSize{ mSourceInputs.size() };
                auto const jackClientInputPortSize{ static_cast<int>(mJackClient.getInputPorts().size()) };
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
        if (mJackClient.isRecording()) {
            mJackClient.stopRecord();
            mStartRecordButton->setEnabled(false);
            mTimeRecordedLabel->setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
        } else {
            mIsRecording = true;
            mJackClient.startRecord();
            mTimeRecordedLabel->setColour(juce::Label::textColourId, mLookAndFeel.getRedColour());
        }
        mStartRecordButton->setToggleState(mJackClient.isRecording(), juce::dontSendNotification);
    } else if (button == mInitRecordButton.get()) {
        chooseRecordingPath();
        mStartRecordButton->setEnabled(true);
    }
}

//==============================================================================
void MainContentComponent::sliderValueChanged(juce::Slider * slider)
{
    if (slider == mMasterGainOutSlider.get()) {
        mJackClient.setMasterGainOut(std::pow(10.0f, static_cast<float>(mMasterGainOutSlider->getValue()) * 0.05f));
    }

    else if (slider == mInterpolationSlider.get()) {
        mJackClient.setInterMaster(static_cast<float>(mInterpolationSlider->getValue()));
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
        mModeSpatCombo->setSelectedId(static_cast<int>(mJackClient.getMode()) + 1,
                                      juce::NotificationType::dontSendNotification);
        return;
    }

    if (mModeSpatCombo.get() == comboBox) {
        mJackClient.setProcessBlockOn(false);
        mJackClient.setMode(static_cast<ModeSpatEnum>(mModeSpatCombo->getSelectedId() - 1));
        switch (mJackClient.getMode()) {
        case ModeSpatEnum::VBAP:
            openXmlFileSpeaker(mPathLastVbapSpeakerSetup);
            mNeedToSaveSpeakerSetup = false;
            mIsSpanShown = true;
            break;
        case ModeSpatEnum::LBAP:
            openXmlFileSpeaker(mPathLastVbapSpeakerSetup);
            mNeedToSaveSpeakerSetup = false;
            mIsSpanShown = true;
            break;
        case ModeSpatEnum::VBAP_HRTF:
            openXmlFileSpeaker(BINAURAL_SPEAKER_SETUP_FILE.getFullPathName());
            mNeedToSaveSpeakerSetup = false;
            mJackClient.resetHrtf();
            mIsSpanShown = false;
            break;
        case ModeSpatEnum::STEREO:
            openXmlFileSpeaker(STEREO_SPEAKER_SETUP_FILE.getFullPathName());
            mNeedToSaveSpeakerSetup = false;
            mIsSpanShown = false;
            break;
        default:
            jassertfalse;
        }
        mJackClient.setProcessBlockOn(true);

        if (mEditSpeakersWindow != nullptr) {
            auto const windowName{ juce::String("Speakers Setup Edition - ")
                                   + juce::String(MODE_SPAT_STRING[static_cast<int>(mJackClient.getMode())])
                                   + juce::String(" - ") + juce::File(mPathCurrentFileSpeaker).getFileName() };
            mEditSpeakersWindow->setName(windowName);
        }
    }
}

//==============================================================================
juce::StringArray MainContentComponent::getMenuBarNames()
{
    char const * names[] = { "File", "View", "Help", nullptr };
    return juce::StringArray{ names };
}

//==============================================================================
void MainContentComponent::setOscLogging(juce::OSCMessage const & message) const
{
    if (mOscLogWindow) {
        auto const address{ message.getAddressPattern().toString() };
        mOscLogWindow->addToLog(address + "\n");
        juce::String msg;
        for (auto const & element : message) {
            if (element.isInt32()) {
                msg += juce::String{ element.getInt32() } + " ";
            } else if (element.isFloat32()) {
                msg += juce::String{ element.getFloat32() } + " ";
            } else if (element.isString()) {
                msg += element.getString() + " ";
            }
        }

        mOscLogWindow->addToLog(msg + "\n");
    }
}

//==============================================================================
void MainContentComponent::chooseRecordingPath()
{
    juce::File dir{ mApplicationProperties.getUserSettings()->getValue("lastRecordingDirectory") };
    if (!dir.isDirectory()) {
        dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    }
    juce::String extF;
    juce::String extChoice;
    if (mJackClient.getRecordFormat() == 0) {
        extF = ".wav";
        extChoice = "*.wav,*.aif";
    } else {
        extF = ".aif";
        extChoice = "*.aif,*.wav";
    }

    juce::FileChooser fc{ "Choose a file to save...",
                          dir.getFullPathName() + "/recording" + extF,
                          extChoice,
                          USE_OS_NATIVE_DIALOG_BOX };

    if (fc.browseForFileToSave(true)) {
        auto const filePath{ fc.getResults().getReference(0).getFullPathName() };
        mApplicationProperties.getUserSettings()->setValue("lastRecordingDirectory",
                                                           juce::File(filePath).getParentDirectory().getFullPathName());
        mJackClient.setRecordingPath(filePath);
    }
    mJackClient.prepareToRecord();
}

//==============================================================================
void MainContentComponent::resized()
{
    auto r{ getLocalBounds().reduced(2) };

    mMenuBar->setBounds(0, 0, getWidth(), 20);
    r.removeFromTop(20);

    // Lay out the speaker view and the vertical divider.
    Component * vComps[] = { mSpeakerViewComponent.get(), mVerticalDividerBar.get(), nullptr };

    // Lay out side-by-side and resize the components' heights as well as widths.
    mVerticalLayout.layOutComponents(vComps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);

    mMainUiBox->setBounds(mSpeakerViewComponent->getWidth() + 6,
                          20,
                          getWidth() - (mSpeakerViewComponent->getWidth() + 10),
                          getHeight());
    mMainUiBox->correctSize(getWidth() - mSpeakerViewComponent->getWidth() - 6, 610);

    mInputsUiBox->setBounds(0, 2, getWidth() - (mSpeakerViewComponent->getWidth() + 10), 231);
    mInputsUiBox->correctSize(static_cast<unsigned>(mSourceInputs.size()) * (VU_METER_WIDTH_IN_PIXELS) + 4, 200);

    mOutputsUiBox->setBounds(0, 233, getWidth() - (mSpeakerViewComponent->getWidth() + 10), 210);
    mOutputsUiBox->correctSize(static_cast<unsigned>(mSpeakers.size()) * (VU_METER_WIDTH_IN_PIXELS) + 4, 180);

    mControlUiBox->setBounds(0, 443, getWidth() - (mSpeakerViewComponent->getWidth() + 10), 145);
    mControlUiBox->correctSize(720, 145);
}
