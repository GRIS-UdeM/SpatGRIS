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

#include "AudioManager.h"
#include "LevelComponent.h"
#include "MainWindow.h"
#include "constants.hpp"

//==============================================================================
MainContentComponent::MainContentComponent(MainWindow & mainWindow,
                                           GrisLookAndFeel & grisLookAndFeel,
                                           SmallGrisLookAndFeel & smallGrisLookAndFeel)
    : mLookAndFeel(grisLookAndFeel)
    , mSmallLookAndFeel(smallGrisLookAndFeel)
    , mMainWindow(mainWindow)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&grisLookAndFeel);

    // init audio
    auto const deviceType{ mConfiguration.getDeviceType() };
    auto const inputDevice{ mConfiguration.getInputDevice() };
    auto const outputDevice{ mConfiguration.getOutputDevice() };
    auto const sampleRate{ mConfiguration.getSampleRate() };
    auto const bufferSize{ mConfiguration.getBufferSize() };

    AudioManager::init(deviceType, inputDevice, outputDevice, sampleRate, bufferSize);

    mAudioProcessor = std::make_unique<AudioProcessor>();

    auto const attenuationDbIndex{ mConfiguration.getAttenuationDbIndex() };
    mAudioProcessor->setAttenuationDbIndex(attenuationDbIndex);

    auto const attenuationFrequencyIndex{ mConfiguration.getAttenuationFrequencyIndex() };
    mAudioProcessor->setAttenuationFrequencyIndex(attenuationFrequencyIndex);

    // Create the menu bar.
    mMenuBar.reset(new juce::MenuBarComponent(this));
    addAndMakeVisible(mMenuBar.get());

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
    mControlUiBox.reset(new Box(mLookAndFeel, "Controls"));
    addAndMakeVisible(mControlUiBox.get());

    mMainUiBox->getContent()->addAndMakeVisible(mInputsUiBox.get());
    mMainUiBox->getContent()->addAndMakeVisible(mOutputsUiBox.get());
    mMainUiBox->getContent()->addAndMakeVisible(mControlUiBox.get());

    // Components in Box Control
    mCpuUsageLabel.reset(addLabel("CPU usage", "CPU usage", 0, 0, 80, 28, mControlUiBox->getContent()));
    mCpuUsageValue.reset(addLabel("0 %", "CPU usage", 80, 0, 80, 28, mControlUiBox->getContent()));
    mSampleRateLabel.reset(addLabel("0 Hz", "Rate", 120, 0, 80, 28, mControlUiBox->getContent()));
    mBufferSizeLabel.reset(addLabel("0 spls", "Buffer Size", 200, 0, 80, 28, mControlUiBox->getContent()));
    mChannelCountLabel.reset(addLabel("...", "Inputs/Outputs", 280, 0, 90, 28, mControlUiBox->getContent()));

    mCpuUsageLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mCpuUsageValue->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mSampleRateLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mBufferSizeLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mChannelCountLabel->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());

    addLabel("Gain", "Master Gain Outputs", 15, 30, 120, 20, mControlUiBox->getContent());
    mMasterGainOutSlider.reset(
        addSlider("Master Gain", "Master Gain Outputs", 5, 45, 60, 60, mControlUiBox->getContent()));
    mMasterGainOutSlider->setRange(-60.0, 12.0, 0.01);
    mMasterGainOutSlider->setTextValueSuffix(" dB");

    addLabel("Interpolation", "Master Interpolation", 60, 30, 120, 20, mControlUiBox->getContent());
    mInterpolationSlider.reset(addSlider("Inter", "Interpolation", 70, 45, 60, 60, mControlUiBox->getContent()));
    mInterpolationSlider->setRange(0.0, 1.0, 0.001);

    addLabel("Mode :", "Mode of spatialization", 150, 30, 60, 20, mControlUiBox->getContent());
    mSpatModeCombo.reset(addComboBox("", "Mode of spatialization", 155, 48, 90, 22, mControlUiBox->getContent()));
    for (int i{}; i < MODE_SPAT_STRING.size(); i++) {
        mSpatModeCombo->addItem(MODE_SPAT_STRING[i], i + 1);
    }

    mAddInputsTextEditor.reset(
        addTextEditor("Inputs :", "0", "Numbers of Inputs", 122, 83, 43, 22, mControlUiBox->getContent()));
    mAddInputsTextEditor->setInputRestrictions(3, "0123456789");

    mInitRecordButton.reset(
        addButton("Init Recording", "Init Recording", 268, 48, 103, 24, mControlUiBox->getContent()));

    mStartRecordButton.reset(addButton("Record", "Start/Stop Record", 268, 83, 60, 24, mControlUiBox->getContent()));
    mStartRecordButton->setEnabled(false);

    mTimeRecordedLabel.reset(addLabel("00:00", "Record time", 327, 83, 50, 24, mControlUiBox->getContent()));

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

    mSamplingRate = narrow<unsigned>(sampleRate);

    jassert(AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice());

    mCpuUsageLabel->setText("CPU usage : ", juce::dontSendNotification);

    AudioManager::getInstance().getAudioDeviceManager().addChangeListener(this);
    audioParametersChanged();

    // Start the OSC Receiver.
    mOscReceiver.reset(new OscInput(*this));
    mOscReceiver->startConnection(mOscInputPort);

    // Default widget values.
    mMasterGainOutSlider->setValue(0.0);
    mInterpolationSlider->setValue(0.1);
    mSpatModeCombo->setSelectedId(1);

    mAddInputsTextEditor->setText("16", juce::dontSendNotification);
    textEditorReturnKeyPressed(*mAddInputsTextEditor);

    // Open the default project if lastOpenProject is not a valid file.
    openProject(mConfiguration.getLastOpenProject());

    // Open the default speaker setup if lastOpenSpeakerSetup is not a valid file.
    auto const lastSpatMode{ mConfiguration.getLastSpatMode() };
    switch (lastSpatMode) {
    case SpatMode::hrtfVbap:
        openXmlFileSpeaker(BINAURAL_SPEAKER_SETUP_FILE, lastSpatMode);
        break;
    case SpatMode::lbap:
    case SpatMode::vbap:
        openXmlFileSpeaker(mConfiguration.getLastSpeakerSetup_(), lastSpatMode);
        break;
    case SpatMode::stereo:
        openXmlFileSpeaker(STEREO_SPEAKER_SETUP_FILE, lastSpatMode);
        break;
    }

    // End layout and start refresh timer.
    resized();
    startTimerHz(24);

    // Start Splash screen.
#if NDEBUG
    if (SPLASH_SCREEN_FILE.exists()) {
        mSplashScreen.reset(
            new juce::SplashScreen("SpatGRIS3", juce::ImageFileFormat::loadFrom(SPLASH_SCREEN_FILE), true));
        mSplashScreen->deleteAfterDelay(juce::RelativeTime::seconds(4), false);
        mSplashScreen.release();
    }
#endif

    // Initialize the command manager for the menu bar items.
    auto & commandManager{ mMainWindow.getApplicationCommandManager() };
    commandManager.registerAllCommandsForTarget(this);

    // Restore last vertical divider position and speaker view cam distance.
    auto const sashPosition{ mConfiguration.getSashPosition() };
    if (sashPosition) {
        auto const trueSize{ narrow<int>(std::round(narrow<double>(getWidth() - 3) * std::abs(*sashPosition))) };
        mVerticalLayout.setItemPosition(1, trueSize);
    }
}

//==============================================================================
MainContentComponent::~MainContentComponent()
{
    mConfiguration.setSashPosition(mVerticalLayout.getItemCurrentRelativeSize(0));

    mSpeakerViewComponent.reset();

    {
        juce::ScopedLock sl{ mSpeakersLock };
        mSpeakers.clear();
    }

    mInputsLock.lock();
    mInputs.clear();
    mInputsLock.unlock();
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
    juce::AlertWindow alert{ "Closing current project !", "Do you want to save ?", juce::AlertWindow::InfoIcon };
    alert.setLookAndFeel(&mLookAndFeel);
    alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::deleteKey));
    alert.addButton("yes", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert.addButton("No", 2, juce::KeyPress(juce::KeyPress::escapeKey));

    auto const status{ alert.runModalLoop() };
    if (status == 1) {
        handleSaveProject();
    } else if (status == 0) {
        return;
    }

    openProject(DEFAULT_PROJECT_FILE.getFullPathName());
}

//==============================================================================
void MainContentComponent::handleOpenProject()
{
    auto const lastOpenProject{ mConfiguration.getLastOpenProject() };
    auto const dir{ lastOpenProject.getParentDirectory() };
    auto const filename{ lastOpenProject.getFileName() };

    juce::FileChooser fc("Choose a file to open...", dir.getFullPathName() + "/" + filename, "*.xml", true);

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
            openProject(chosen);
            loaded = true;
        }
    }

    if (loaded) { // Check for direct out OutputPatch mismatch.
        for (auto const * it : mInputs) {
            if (it->getDirectOutChannel() != output_patch_t{}) {
                auto const directOutOutputPatches{ mAudioProcessor->getDirectOutOutputPatches() };
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
void MainContentComponent::handleSaveProject() const
{
    auto const lastOpenProject{ mConfiguration.getLastOpenProject() };
    if (!lastOpenProject.existsAsFile()
        || lastOpenProject.getFullPathName().endsWith("default_preset/default_preset.xml")) {
        handleSaveAsProject();
    }
    saveProject(lastOpenProject.getFullPathName());
}

//==============================================================================
void MainContentComponent::handleSaveAsProject() const
{
    auto const lastOpenProject{ mConfiguration.getLastOpenProject() };

    juce::FileChooser fc{ "Choose a file to save...", lastOpenProject.getFullPathName(), "*.xml", true };

    if (fc.browseForFileToSave(true)) {
        auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
        saveProject(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleOpenSpeakerSetup()
{
    juce::FileChooser fc{ "Choose a file to open...", mCurrentSpeakerSetup, "*.xml", true };

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
    juce::FileChooser fc{ "Choose a file to save...", mCurrentSpeakerSetup, "*.xml", true };

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
                                + juce::String(MODE_SPAT_STRING[static_cast<int>(mAudioProcessor->getMode())]) + " - "
                                + mCurrentSpeakerSetup.getFileName();
        mEditSpeakersWindow = std::make_unique<EditSpeakersWindow>(windowName, mLookAndFeel, *this, mConfigurationName);
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
    if (mPropertiesWindow == nullptr) {
        auto const fileFormat{ mConfiguration.getRecordingFormat() };
        auto const fileConfig{ mConfiguration.getRecordingConfig() };

        auto const attenuationDb{ mConfiguration.getAttenuationDbIndex() };
        auto const attenuationHz{ mConfiguration.getAttenuationFrequencyIndex() };
        auto const oscInputPort{ mConfiguration.getOscInputPort() };

        mPropertiesWindow.reset(new SettingsWindow{ *this,
                                                    mLookAndFeel,
                                                    fileFormat,
                                                    fileConfig,
                                                    attenuationDb,
                                                    attenuationHz,
                                                    oscInputPort });
    }
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
    if (getModeSelected() == SpatMode::lbap && state == true) {
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
    for (auto * input : mInputs) {
        input->resetPosition();
    }
}

//==============================================================================
void MainContentComponent::handleResetMeterClipping()
{
    for (auto * input : mInputs) {
        input->getVuMeter()->resetClipping();
    }
    for (auto * speaker : mSpeakers) {
        speaker->getVuMeter()->resetClipping();
    }
}

//==============================================================================
void MainContentComponent::handleInputColours()
{
    auto hue = 0.0f;
    auto const inc = 1.0f / static_cast<float>(mInputs.size() + 1);
    for (auto * input : mInputs) {
        input->setColor(juce::Colour::fromHSV(hue, 1, 0.75, 1), true);
        hue += inc;
    }
}

//==============================================================================
// Command manager methods.
void MainContentComponent::getAllCommands(juce::Array<juce::CommandID> & commands)
{
    // this returns the set of all commands that this target can perform.
    const juce::CommandID ids[] = {
        MainWindow::NewProjectID,
        MainWindow::OpenProjectID,
        MainWindow::SaveProjectID,
        MainWindow::SaveAsProjectID,
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
        MainWindow::OpenSettingsWindowID,
        MainWindow::QuitID,
        MainWindow::AboutID,
        MainWindow::OpenManualID,
    };

    commands.addArray(ids, juce::numElementsInArray(ids));
}

//==============================================================================
void MainContentComponent::getCommandInfo(juce::CommandID const commandId, juce::ApplicationCommandInfo & result)
{
    const juce::String generalCategory("General");

    switch (commandId) {
    case MainWindow::NewProjectID:
        result.setInfo("New Project", "Close the current project and open the default.", generalCategory, 0);
        result.addDefaultKeypress('N', juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::OpenProjectID:
        result.setInfo("Open Project", "Choose a new project on disk.", generalCategory, 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::SaveProjectID:
        result.setInfo("Save Project", "Save the current project on disk.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::commandModifier);
        break;
    case MainWindow::SaveAsProjectID:
        result.setInfo("Save Project As...", "Save the current project under a new name on disk.", generalCategory, 0);
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
    case MainWindow::OpenSettingsWindowID:
        result.setInfo("Settings...", "Open the settings window.", generalCategory, 0);
        result.addDefaultKeypress(',', juce::ModifierKeys::commandModifier);
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
        case MainWindow::NewProjectID:
            handleNew();
            break;
        case MainWindow::OpenProjectID:
            handleOpenProject();
            break;
        case MainWindow::SaveProjectID:
            handleSaveProject();
            break;
        case MainWindow::SaveAsProjectID:
            handleSaveAsProject();
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
        case MainWindow::OpenSettingsWindowID:
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
void MainContentComponent::audioParametersChanged()
{
    juce::ScopedLock const lock{ mAudioProcessor->getCriticalSection() };

    auto * currentAudioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };

    if (!currentAudioDevice) {
        return;
    }

    auto const sampleRate{ currentAudioDevice->getCurrentSampleRate() };
    auto const bufferSize{ currentAudioDevice->getCurrentBufferSizeSamples() };
    auto const inputCount{ currentAudioDevice->getActiveInputChannels().countNumberOfSetBits() };
    auto const outputCount{ currentAudioDevice->getActiveOutputChannels().countNumberOfSetBits() };

    mConfiguration.setSampleRate(sampleRate);
    mConfiguration.setBufferSize(bufferSize);

    mSamplingRate = narrow<unsigned>(sampleRate);

    mSampleRateLabel->setText(juce::String{ narrow<unsigned>(sampleRate) } + " Hz",
                              juce::NotificationType::dontSendNotification);
    mBufferSizeLabel->setText(juce::String{ bufferSize } + " samples", juce::NotificationType::dontSendNotification);
    mChannelCountLabel->setText("I : " + juce::String{ inputCount } + " - O : " + juce::String{ outputCount },
                                juce::dontSendNotification);
}

//==============================================================================
juce::PopupMenu MainContentComponent::getMenuForIndex(int /*menuIndex*/, const juce::String & menuName)
{
    juce::ApplicationCommandManager * commandManager = &mMainWindow.getApplicationCommandManager();

    juce::PopupMenu menu;

    if (menuName == "File") {
        menu.addCommandItem(commandManager, MainWindow::NewProjectID);
        menu.addCommandItem(commandManager, MainWindow::OpenProjectID);
        menu.addCommandItem(commandManager, MainWindow::SaveProjectID);
        menu.addCommandItem(commandManager, MainWindow::SaveAsProjectID);
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::OpenSpeakerSetupID);
        menu.addSeparator();
        menu.addCommandItem(commandManager, MainWindow::OpenSettingsWindowID);
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
        if (mAudioProcessor->getVbapDimensions() == 3) {
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
    } else if (menuName == "Help") {
        menu.addCommandItem(commandManager, MainWindow::AboutID);
        menu.addCommandItem(commandManager, MainWindow::OpenManualID);
    }
    return menu;
}

//==============================================================================
void MainContentComponent::menuItemSelected(int /*menuItemID*/, int /*topLevelMenuIndex*/)
{
}

//==============================================================================
// Exit functions.
bool MainContentComponent::isProjectModified() const
{
    auto const xmlFile{ mConfiguration.getLastOpenProject() };
    juce::XmlDocument xmlDoc{ xmlFile };
    auto const savedState{ xmlDoc.getDocumentElement() };
    if (!savedState) {
        return true;
    }

    auto const currentState{ std::make_unique<juce::XmlElement>("ServerGRIS_Preset") };
    getProjectData(currentState.get());

    if (!savedState->isEquivalentTo(currentState.get(), true)) {
        return true;
    }

    return false;
}

//==============================================================================
bool MainContentComponent::exitApp()
{
    auto exitV{ 2 };

    if (isProjectModified()) {
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
            auto const lastOpenProject{ mConfiguration.getLastOpenProject() };

            juce::FileChooser fc("Choose a file to save...", lastOpenProject.getFullPathName(), "*.xml", true);

            if (fc.browseForFileToSave(true)) {
                auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
                saveProject(chosen);
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
    for (auto const & cli : mAudioProcessor->getClients()) {
        if (cli.portEnd > maxPort) {
            maxPort = cli.portEnd;
        }
    }
    if (maxPort > narrow<unsigned>(mAddInputsTextEditor->getTextValue().toString().getIntValue())) {
        mAddInputsTextEditor->setText(juce::String{ maxPort }, juce::dontSendNotification);
        textEditorReturnKeyPressed(*mAddInputsTextEditor);
    }

    mAudioProcessor->connectionClient(clientName, conn);
}

//==============================================================================
void MainContentComponent::selectSpeaker(speaker_id_t const idS) const
{
    for (int i{}; i < mSpeakers.size(); ++i) {
        if (i != idS.get()) {
            mSpeakers[i]->unSelectSpeaker();
        } else {
            mSpeakers[i]->selectSpeaker();
        }
    }
    if (mEditSpeakersWindow != nullptr) {
        mEditSpeakersWindow->selectedRow(idS.get());
    }
}

//==============================================================================
void MainContentComponent::selectTripletSpeaker(speaker_id_t const idS)
{
    auto countS{ std::count_if(mSpeakers.begin(), mSpeakers.end(), [](Speaker const * speaker) {
        return speaker->isSelected();
    }) };

    if (!mSpeakers[idS.get()]->isSelected() && countS < 3) {
        mSpeakers[idS.get()]->selectSpeaker();
        countS += 1;
    } else {
        mSpeakers[idS.get()]->unSelectSpeaker();
    }

    if (countS == 3) {
        static constexpr output_patch_t INVALID_PATCH{ -1 };
        auto i1{ INVALID_PATCH };
        auto i2{ INVALID_PATCH };
        auto i3{ INVALID_PATCH };
        for (int i{}; i < mSpeakers.size(); ++i) {
            if (mSpeakers[i]->isSelected()) {
                if (i1 == INVALID_PATCH) {
                    i1 = output_patch_t{ i };
                } else {
                    if (i2 == INVALID_PATCH) {
                        i2 = output_patch_t{ i };
                    } else {
                        if (i3 == INVALID_PATCH) {
                            i3 = output_patch_t{ i };
                        }
                    }
                }
            }
        }
        if (i1 != INVALID_PATCH && i2 != INVALID_PATCH && i3 != INVALID_PATCH) {
            Triplet const tri{ i1, i2, i3 };
            auto posDel{ -1 };
            if (tripletExists(tri, posDel)) {
                mTriplets.remove(mTriplets.begin() + posDel);
            } else {
                mTriplets.add(tri);
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
    speaker_id_t id{ 1 };
    for (auto * it : mSpeakers) {
        it->setSpeakerId(id++);
    }
}

//==============================================================================
void MainContentComponent::reorderSpeakers(std::vector<speaker_id_t> const & newOrder)
{
    auto const size = mSpeakers.size();

    juce::Array<Speaker *> tempListSpeaker{};
    tempListSpeaker.resize(size);

    for (int i{}; i < size; ++i) {
        for (auto * speaker : mSpeakers) {
            if (speaker->getIdSpeaker() == newOrder[i]) {
                tempListSpeaker.setUnchecked(i, speaker);
                break;
            }
        }
    }

    juce::ScopedLock sl{ mSpeakersLock };
    mSpeakers.clearQuick(false);
    mSpeakers.addArray(tempListSpeaker);
}

//==============================================================================
speaker_id_t MainContentComponent::getMaxSpeakerId() const
{
    speaker_id_t maxId{};
    for (auto const * it : mSpeakers) {
        if (it->getIdSpeaker() > maxId) {
            maxId = it->getIdSpeaker();
        }
    }
    return maxId;
}

//==============================================================================
output_patch_t MainContentComponent::getMaxSpeakerOutputPatch() const
{
    output_patch_t maxOut{};
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
    {
        juce::ScopedLock sl{ mSpeakersLock };
        auto const newId{ ++getMaxSpeakerId() };
        mSpeakers.add(new Speaker{ *this, mSmallLookAndFeel, newId, output_patch_t{ newId.get() }, 0.0f, 0.0f, 1.0f });

        if (sortColumnId == 1 && isSortedForwards) {
            for (int i{}; i < mSpeakers.size(); ++i) {
                speaker_id_t const newId{ i + 1 };
                mSpeakers[i]->setSpeakerId(newId);
            }
        } else if (sortColumnId == 1 && !isSortedForwards) {
            for (int i{}; i < mSpeakers.size(); ++i) {
                speaker_id_t const newId{ mSpeakers.size() - i };
                mSpeakers[i]->setSpeakerId(newId);
            }
        }
    }

    mAudioProcessor->addOutput(mSpeakers.getLast()->getOutputPatch());
}

//==============================================================================
void MainContentComponent::insertSpeaker(int const position, int const sortColumnId, bool const isSortedForwards)
{
    auto const newPosition{ position + 1 };
    auto const newOut{ ++getMaxSpeakerOutputPatch() };
    auto newId{ ++getMaxSpeakerId() };

    {
        juce::ScopedLock sl{ mSpeakersLock };
        if (sortColumnId == 1 && isSortedForwards) {
            newId = ++mSpeakers[position]->getIdSpeaker();
            mSpeakers.insert(newPosition, new Speaker{ *this, mSmallLookAndFeel, newId, newOut, 0.0f, 0.0f, 1.0f });
            for (auto i{ 1 }; i <= mSpeakers.size(); ++i) {
                mSpeakers.getUnchecked(i)->setSpeakerId(speaker_id_t{ i });
            }
        } else if (sortColumnId == 1 && !isSortedForwards) {
            newId = --mSpeakers[position]->getIdSpeaker();
            mSpeakers.insert(newPosition, new Speaker{ *this, mSmallLookAndFeel, newId, newOut, 0.0f, 0.0f, 1.0f });
            for (int i{}; i < mSpeakers.size(); ++i) {
                speaker_id_t const id{ mSpeakers.size() - i };
                mSpeakers.getUnchecked(i)->setSpeakerId(id);
            }
        } else {
            mSpeakers.insert(newPosition, new Speaker{ *this, mSmallLookAndFeel, newId, newOut, 0.0f, 0.0f, 1.0f });
        }
    }

    juce::ScopedLock const lock{ mAudioProcessor->getCriticalSection() };
    mAudioProcessor->clearOutput();
    for (auto * it : mSpeakers) {
        mAudioProcessor->addOutput(it->getOutputPatch());
    }
}

//==============================================================================
void MainContentComponent::removeSpeaker(int const idSpeaker)
{
    mAudioProcessor->removeOutput(idSpeaker);
    juce::ScopedLock sl{ mSpeakersLock };
    mSpeakers.remove(idSpeaker, true);
}

//==============================================================================
bool MainContentComponent::isRadiusNormalized() const
{
    auto const mode{ mAudioProcessor->getMode() };
    if (mode == SpatMode::vbap || mode == SpatMode::hrtfVbap)
        return true;
    else
        return false;
}

//==============================================================================
void MainContentComponent::updateInputJack(int const inInput, Input & inp) const
{
    auto const mode{ mAudioProcessor->getMode() };
    auto & si = mAudioProcessor->getSourcesIn()[inInput];

    if (mode == SpatMode::lbap) {
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

    if (mode == SpatMode::vbap || mode == SpatMode::hrtfVbap) {
        mAudioProcessor->getVbapSourcesToUpdate()[inInput] = 1;
    }
}

//==============================================================================
void MainContentComponent::setTripletsFromVbap()
{
    mTriplets = mAudioProcessor->getVbapTriplets();
}

//==============================================================================
Speaker const * MainContentComponent::getSpeakerFromOutputPatch(output_patch_t const out) const
{
    for (auto const * it : mSpeakers) {
        if (it->getOutputPatch() == out && !it->isDirectOut()) {
            return it;
        }
    }
    return nullptr;
}

//==============================================================================
Speaker * MainContentComponent::getSpeakerFromOutputPatch(output_patch_t const out)
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
    auto const level{ mAudioProcessor->getLevelsIn(indexLevel) };
    if (level > 0.0001f) {
        // -80 dB
        return 1.0f;
    }
    return std::sqrt(level * 10000.0f);
}

//==============================================================================
float MainContentComponent::getSpeakerLevelsAlpha(int const indexLevel) const
{
    auto const level{ mAudioProcessor->getLevelsOut(indexLevel) };
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
    // TODO : this function is 100 times longer than it should be.

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
            zenith = it->getPolarCoords().y;
        } else if (it->getPolarCoords().y < (zenith - 4.9f) || it->getPolarCoords().y > (zenith + 4.9f)) {
            dimensions = 3;
        }
    }

    // Too few speakers...
    if ((mSpeakers.size() - directOutSpeakers) < dimensions) {
        juce::AlertWindow alert("Not enough speakers !    ",
                                "Do you want to reload the default setup ?    ",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("No", 0);
        alert.addButton("Yes", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE);
        }
        return false;
    }

    // Test for duplicated output patch.
    std::vector<output_patch_t> tempOut;
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
                openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE);
            }
            return false;
        }
    }

    juce::ScopedLock const lock{ mAudioProcessor->getCriticalSection() };
    mAudioProcessor->setMaxOutputPatch(output_patch_t{});

    // Save mute/solo/directOut states
    std::array<bool, MAX_INPUTS> inputsIsMuted{};
    std::array<bool, MAX_INPUTS> inputsIsSolo{};
    auto const soloIn{ mAudioProcessor->getSoloIn() };
    std::array<output_patch_t, MAX_INPUTS> directOuts{};
    auto const & sourcesIn{ mAudioProcessor->getSourcesIn() };

    for (size_t i{}; i < MAX_INPUTS; ++i) {
        inputsIsMuted[i] = sourcesIn[i].isMuted;
        inputsIsSolo[i] = sourcesIn[i].isSolo;
        directOuts[i] = sourcesIn[i].directOut;
    }

    std::array<bool, MAX_OUTPUTS> outputsIsMuted{};
    std::array<bool, MAX_OUTPUTS> outputsIsSolo{};
    auto const soloOut{ mAudioProcessor->getSoloOut() };
    auto const & speakersOut{ mAudioProcessor->getSpeakersOut() };
    for (size_t i{}; i < MAX_OUTPUTS; ++i) {
        outputsIsMuted[i] = speakersOut[i].isMuted;
        outputsIsSolo[i] = speakersOut[i].isSolo;
    }

    // Cleanup speakers output patch
    for (auto & it : mAudioProcessor->getSpeakersOut()) {
        it.outputPatch = output_patch_t{ 0 };
    }

    // Create outputs.
    int i{};
    auto x{ 2 };
    auto const mode{ mAudioProcessor->getMode() };
    for (auto * speaker : mSpeakers) {
        juce::Rectangle<int> level{ x, 4, VU_METER_WIDTH_IN_PIXELS, 200 };
        speaker->getVuMeter()->setBounds(level);
        speaker->getVuMeter()->resetClipping();
        mOutputsUiBox->getContent()->addAndMakeVisible(speaker->getVuMeter());
        speaker->getVuMeter()->repaint();

        x += VU_METER_WIDTH_IN_PIXELS;

        if (mode == SpatMode::vbap || mode == SpatMode::hrtfVbap) {
            speaker->normalizeRadius();
        }

        SpeakerData so;
        so.id = speaker_id_t{ speaker->getOutputPatch().get() };
        so.x = speaker->getCartesianCoords().x;
        so.y = speaker->getCartesianCoords().y;
        so.z = speaker->getCartesianCoords().z;
        so.azimuth = speaker->getPolarCoords().x;
        so.zenith = speaker->getPolarCoords().y;
        so.radius = speaker->getPolarCoords().z;
        so.outputPatch = speaker->getOutputPatch();
        so.directOut = speaker->isDirectOut();

        mAudioProcessor->getSpeakersOut()[i++] = so;

        if (speaker->getOutputPatch() > mAudioProcessor->getMaxOutputPatch()) {
            mAudioProcessor->setMaxOutputPatch(speaker->getOutputPatch());
        }
    }

    // Set user gain and highpass filter cutoff frequency for each speaker.
    for (auto const * speaker : mSpeakers) {
        auto & speakerOut{ mAudioProcessor->getSpeakersOut()[speaker->getOutputPatch().get() - 1] };
        speakerOut.gain = std::pow(10.0f, speaker->getGain() * 0.05f);
        if (speaker->getHighPassCutoff() > 0.0f) {
            double * coefficients;
            linkwitzRileyComputeVariables(static_cast<double>(speaker->getHighPassCutoff()),
                                          narrow<double>(mSamplingRate),
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
    mInputsLock.lock();
    std::vector<output_patch_t> directOutMenuItems{};
    for (auto const * speaker : mSpeakers) {
        if (speaker->isDirectOut()) {
            directOutMenuItems.push_back(speaker->getOutputPatch());
        }
    }
    for (auto * input : mInputs) {
        juce::Rectangle<int> level{ x, 4, VU_METER_WIDTH_IN_PIXELS, 200 };
        input->getVuMeter()->setBounds(level);
        if (input->isInput()) { // TODO : wut?
            input->getVuMeter()->updateDirectOutMenu(directOutMenuItems);
        }
        input->getVuMeter()->resetClipping();
        mInputsUiBox->getContent()->addAndMakeVisible(input->getVuMeter());
        input->getVuMeter()->repaint();

        x += VU_METER_WIDTH_IN_PIXELS;

        SourceData sourceIn;
        sourceIn.id = input->getId();
        sourceIn.radAzimuth = input->getAzimuth();
        sourceIn.radElevation = juce::MathConstants<float>::halfPi - input->getZenith();
        sourceIn.azimuth = input->getAzimuth();
        sourceIn.zenith = input->getZenith();
        sourceIn.radius = input->getRadius();
        sourceIn.gain = 0.0f;
        mAudioProcessor->getSourcesIn()[i++] = sourceIn;
    }

    mInputsLock.unlock();

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
    if (mode == SpatMode::vbap || mode == SpatMode::hrtfVbap) {
        mAudioProcessor->setVbapDimensions(dimensions);
        if (dimensions == 2) {
            setShowTriplets(false);
        }
        returnValue = mAudioProcessor->initSpeakersTriplet(tempListSpeaker, dimensions, mNeedToComputeVbap);

        if (returnValue) {
            setTripletsFromVbap();
            mNeedToComputeVbap = false;
        } else {
            juce::AlertWindow alert{ "Not a valid DOME 3-D configuration!    ",
                                     "Maybe you want to open it in CUBE mode? Reload the default speaker setup ?    ",
                                     juce::AlertWindow::WarningIcon };
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
            alert.runModalLoop();
            openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE);
            return false;
        }
    } else if (mode == SpatMode::lbap) {
        setShowTriplets(false);
        returnValue = mAudioProcessor->lbapSetupSpeakerField(tempListSpeaker);
    }

    // Restore mute/solo/directOut states
    mAudioProcessor->setSoloIn(soloIn);
    for (size_t sourceInIndex{}; sourceInIndex < MAX_INPUTS; ++sourceInIndex) {
        auto & sourceIn{ mAudioProcessor->getSourcesIn()[sourceInIndex] };
        sourceIn.isMuted = inputsIsMuted[sourceInIndex];
        sourceIn.isSolo = inputsIsSolo[sourceInIndex];
        sourceIn.directOut = directOuts[sourceInIndex];
    }
    mInputsLock.lock();
    for (int sourceInputIndex{}; sourceInputIndex < mInputs.size(); sourceInputIndex++) {
        mInputs[sourceInputIndex]->setDirectOutChannel(directOuts[sourceInputIndex]);
    }
    mInputsLock.unlock();

    mAudioProcessor->setSoloOut(soloOut);
    for (size_t speakerOutIndex{}; speakerOutIndex < MAX_OUTPUTS; ++speakerOutIndex) {
        auto & speakerOut{ mAudioProcessor->getSpeakersOut()[speakerOutIndex] };
        speakerOut.isMuted = outputsIsMuted[speakerOutIndex];
        speakerOut.isSolo = outputsIsSolo[speakerOutIndex];
    }

    return returnValue;
}

//==============================================================================
void MainContentComponent::setCurrentSpeakerSetup(juce::File const & file)
{
    mCurrentSpeakerSetup = file;
    mConfigurationName = file.getFileNameWithoutExtension();
    mSpeakerViewComponent->setNameConfig(mConfigurationName);
}

//==============================================================================
void MainContentComponent::muteInput(int const id, bool const mute) const
{
    auto const index{ id - 1 };
    mAudioProcessor->getSourcesIn()[index].isMuted = mute;
}

//==============================================================================
void MainContentComponent::muteOutput(output_patch_t const id, bool const mute) const
{
    auto const index{ id.get() - 1 };
    mAudioProcessor->getSpeakersOut()[index].isMuted = mute;
}

//==============================================================================
void MainContentComponent::soloInput(output_patch_t const id, bool const solo) const
{
    auto & sourcesIn{ mAudioProcessor->getSourcesIn() };
    auto const index{ id.get() - 1 };
    sourcesIn[index].isSolo = solo;

    mAudioProcessor->setSoloIn(false);
    for (unsigned int i = 0; i < MAX_INPUTS; i++) {
        if (sourcesIn[i].isSolo) {
            mAudioProcessor->setSoloIn(true);
            break;
        }
    }
}

//==============================================================================
void MainContentComponent::soloOutput(output_patch_t const id, bool const solo) const
{
    auto const index{ id.get() - 1 };
    auto & speakersOut{ mAudioProcessor->getSpeakersOut() };
    speakersOut[index].isSolo = solo;

    mAudioProcessor->setSoloOut(false);
    for (unsigned int i = 0; i < MAX_OUTPUTS; i++) {
        if (speakersOut[i].isSolo) {
            mAudioProcessor->setSoloOut(true);
            break;
        }
    }
}

//==============================================================================
void MainContentComponent::setDirectOut(int const id, output_patch_t const chn) const
{
    auto const index{ id - 1 };
    mAudioProcessor->getSourcesIn()[index].directOut = chn;
}

//==============================================================================
void MainContentComponent::reloadXmlFileSpeaker()
{
    openXmlFileSpeaker(mConfiguration.getLastSpeakerSetup_(), mAudioProcessor->getMode());
}

//==============================================================================
void MainContentComponent::openXmlFileSpeaker(juce::File const & file, std::optional<SpatMode> const forceSpatMode)
{
    jassert(file.existsAsFile());

    auto const isNewSameAsOld{ file == mCurrentSpeakerSetup };

    if (!file.existsAsFile()) {
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                          "Error in Load Speaker Setup !",
                                          "Cannot find file " + file.getFullPathName() + ", loading default setup.");
        openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE);
        return;
    }

    juce::XmlDocument xmlDoc{ file };
    auto const mainXmlElem(xmlDoc.getDocumentElement());
    if (!mainXmlElem) {
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                          "Error in Load Speaker Setup !",
                                          "Your file is corrupted !\n" + xmlDoc.getLastParseError());
        openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE);
        return;
    }

    if (!mainXmlElem->hasTagName("SpeakerSetup")) {
        auto const msg{ mainXmlElem->hasTagName("ServerGRIS_Preset")
                            ? "You are trying to open a Server document, and not a Speaker Setup !"
                            : "Your file is corrupted !\n" + xmlDoc.getLastParseError() };
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon, "Error in Load Speaker Setup !", msg);
        openXmlFileSpeaker(DEFAULT_SPEAKER_SETUP_FILE);
        return;
    }

    {
        juce::ScopedLock sl{ mSpeakersLock };
        mSpeakers.clear();
    }

    auto const spatMode{ forceSpatMode ? *forceSpatMode
                                       : static_cast<SpatMode>(mainXmlElem->getIntAttribute("SpatMode")) };

    mAudioProcessor->setMode(spatMode);
    mSpatModeCombo->setSelectedId(static_cast<int>(spatMode) + 1, juce::dontSendNotification);

    auto const loadSetupFromXyz{ spatMode == SpatMode::lbap };

    juce::ScopedLock const lock{ mAudioProcessor->getCriticalSection() };
    mAudioProcessor->clearOutput();
    mAudioProcessor->setMaxOutputPatch(output_patch_t{});
    juce::Array<int> layoutIndexes{};
    int maxLayoutIndex{};
    forEachXmlChildElement(*mainXmlElem, ring)
    {
        if (ring->hasTagName("Ring")) {
            forEachXmlChildElement(*ring, spk)
            {
                if (spk->hasTagName("Speaker")) {
                    // Safety against layoutIndex doubles in the speaker setup.
                    auto layoutIndex{ spk->getIntAttribute("LayoutIndex") };
                    if (layoutIndexes.contains(layoutIndex)) {
                        layoutIndex = ++maxLayoutIndex;
                    }
                    layoutIndexes.add(layoutIndex);
                    if (layoutIndex > maxLayoutIndex) {
                        maxLayoutIndex = layoutIndex;
                    }

                    output_patch_t const outputPatch{ spk->getIntAttribute("OutputPatch") };
                    auto const azimuth{ static_cast<float>(spk->getDoubleAttribute("Azimuth")) };
                    auto const zenith{ static_cast<float>(spk->getDoubleAttribute("Zenith")) };
                    auto const radius{ static_cast<float>(spk->getDoubleAttribute("Radius")) };
                    mSpeakers.add(new Speaker{ *this,
                                               mSmallLookAndFeel,
                                               speaker_id_t{ layoutIndex },
                                               outputPatch,
                                               azimuth,
                                               zenith,
                                               radius });
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
                    mAudioProcessor->addOutput(output_patch_t{ spk->getIntAttribute("OutputPatch") });
                }
            }
        }
        if (ring->hasTagName("triplet")) {
            Triplet const triplet{ output_patch_t{ ring->getIntAttribute("id1") },
                                   output_patch_t{ ring->getIntAttribute("id2") },
                                   output_patch_t{ ring->getIntAttribute("id3") } };
            mTriplets.add(triplet);
        }
    }

    setCurrentSpeakerSetup(file);
    mConfiguration.setLastSpatMode(spatMode);
    if (spatMode == SpatMode::vbap || spatMode == SpatMode::lbap) {
        mConfiguration.setLastSpeakerSetup_(file);
    }

    mNeedToComputeVbap = true;
    updateLevelComp();
}

//==============================================================================
void MainContentComponent::setTitle() const
{
    auto const currentProject{ mConfiguration.getLastOpenProject() };
    auto const title{ juce::String{ "SpatGRIS v" } + STRING(JUCE_APP_VERSION) + " - " + currentProject.getFileName() };
    mMainWindow.setName(title);
}

//==============================================================================
void MainContentComponent::handleTimer(bool const state)
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
}

//==============================================================================
void MainContentComponent::openProject(juce::File const & file)
{
    jassert(file.existsAsFile());

    juce::ScopedLock const lock{ mAudioProcessor->getCriticalSection() };

    juce::XmlDocument xmlDoc{ file };
    auto const mainXmlElem{ xmlDoc.getDocumentElement() };
    if (!mainXmlElem) {
        juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon,
                                          "Error in Open Project !",
                                          "Your file is corrupted !\n" + file.getFullPathName() + "\n"
                                              + xmlDoc.getLastParseError());
        return;
    }

    if (!mainXmlElem->hasTagName("SpatServerGRIS_Preset") && !mainXmlElem->hasTagName("ServerGRIS_Preset")) {
        auto const msg{ mainXmlElem->hasTagName("SpeakerSetup")
                            ? "You are trying to open a Speaker Setup instead of a project file !"
                            : "Your file is corrupted !\n" + xmlDoc.getLastParseError() };
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon, "Error in Open Project !", msg);
        return;
    }

    mOscInputPort = mainXmlElem->getIntAttribute("OSC_Input_Port");
    mAddInputsTextEditor->setText(mainXmlElem->getStringAttribute("Number_Of_Inputs"));
    mMasterGainOutSlider->setValue(mainXmlElem->getDoubleAttribute("Master_Gain_Out", 0.0), juce::sendNotification);
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
        mSpeakerViewComponent->setCamPosition(80.0f, 25.0f, 22.0f);
    }

    // Update
    textEditorReturnKeyPressed(*mAddInputsTextEditor);
    sliderValueChanged(mMasterGainOutSlider.get());
    sliderValueChanged(mInterpolationSlider.get());

    for (auto * input{ mainXmlElem->getFirstChildElement() }; input != nullptr; input = input->getNextElement()) {
        if (input->hasTagName("Input")) {
            for (auto * it : mInputs) {
                if (it->getId() == input->getIntAttribute("Index")) {
                    it->setColor(juce::Colour::fromFloatRGBA(static_cast<float>(input->getDoubleAttribute("R")),
                                                             static_cast<float>(input->getDoubleAttribute("G")),
                                                             static_cast<float>(input->getDoubleAttribute("B")),
                                                             1.0f),
                                 true);
                    if (input->hasAttribute("DirectOut")) {
                        it->setDirectOutChannel(output_patch_t{ input->getIntAttribute("DirectOut") });
                        setDirectOut(it->getId(), output_patch_t{ input->getIntAttribute("DirectOut") });
                    } else {
                        it->setDirectOutChannel(output_patch_t{});
                        setDirectOut(it->getId(), output_patch_t{});
                    }
                }
            }
        }
    }

    mConfiguration.setLastOpenProject(file);
    mAudioProcessor->setPinkNoiseActive(false);
    setTitle();
}

//==============================================================================
void MainContentComponent::getProjectData(juce::XmlElement * xml) const
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

    for (auto const * sourceInput : mInputs) {
        auto * xmlInput{ new juce::XmlElement{ "Input" } };
        xmlInput->setAttribute("Index", sourceInput->getId());
        xmlInput->setAttribute("R", sourceInput->getColor().x);
        xmlInput->setAttribute("G", sourceInput->getColor().y);
        xmlInput->setAttribute("B", sourceInput->getColor().z);
        xmlInput->setAttribute("DirectOut", juce::String(sourceInput->getDirectOutChannel().get()));
        xml->addChildElement(xmlInput);
    }
}

//==============================================================================
void MainContentComponent::saveProject(juce::String const & path) const
{
    juce::File const xmlFile{ path };
    auto const xml{ std::make_unique<juce::XmlElement>("ServerGRIS_Preset") };
    getProjectData(xml.get());
    [[maybe_unused]] auto success{ xml->writeTo(xmlFile) };
    jassert(success);
    success = xmlFile.create();
    jassert(success);
    mConfiguration.setLastOpenProject(path);

    setTitle();
}

//==============================================================================
void MainContentComponent::saveSpeakerSetup(juce::String const & path)
{
    juce::File const xmlFile{ path };
    juce::XmlElement xml{ "SpeakerSetup" };

    xml.setAttribute("Name", mConfigurationName);
    xml.setAttribute("Dimension", 3);
    xml.setAttribute("SpatMode", static_cast<int>(getModeSelected()));

    auto * xmlRing{ new juce::XmlElement{ "Ring" } };

    for (auto const * it : mSpeakers) {
        auto * xmlInput{ new juce::XmlElement{ "Speaker" } };
        xmlInput->setAttribute("PositionY", it->getCartesianCoords().z);
        xmlInput->setAttribute("PositionX", it->getCartesianCoords().x);
        xmlInput->setAttribute("PositionZ", it->getCartesianCoords().y);
        xmlInput->setAttribute("Azimuth", it->getPolarCoords().x);
        xmlInput->setAttribute("Zenith", it->getPolarCoords().y);
        xmlInput->setAttribute("Radius", it->getPolarCoords().z);
        xmlInput->setAttribute("LayoutIndex", it->getIdSpeaker().get());
        xmlInput->setAttribute("OutputPatch", it->getOutputPatch().get());
        xmlInput->setAttribute("Gain", it->getGain());
        xmlInput->setAttribute("HighPassCutoff", it->getHighPassCutoff());
        xmlInput->setAttribute("DirectOut", it->isDirectOut());
        xmlRing->addChildElement(xmlInput);
    }
    xml.addChildElement(xmlRing);

    for (auto const & triplet : mTriplets) {
        auto * xmlInput{ new juce::XmlElement{ "triplet" } };
        xmlInput->setAttribute("id1", triplet.id1.get());
        xmlInput->setAttribute("id2", triplet.id2.get());
        xmlInput->setAttribute("id3", triplet.id3.get());
        xml.addChildElement(xmlInput);
    }

    [[maybe_unused]] auto success{ xml.writeTo(xmlFile) };
    jassert(success);
    success = xmlFile.create();
    jassert(success);

    mConfiguration.setLastSpeakerSetup_(path);
    mNeedToSaveSpeakerSetup = false;
    setCurrentSpeakerSetup(path);
}

//==============================================================================
void MainContentComponent::saveProperties(juce::String const & audioDeviceType,
                                          juce::String const & inputDevice,
                                          juce::String const & outputDevice,
                                          double const sampleRate,
                                          int const bufferSize,
                                          RecordingFormat const recordingFormat,
                                          RecordingConfig const recordingConfig,
                                          int const attenuationDbIndex,
                                          int const attenuationFrequencyIndex,
                                          int oscPort)
{
    // Handle audio options
    mConfiguration.setDeviceType(audioDeviceType);
    mConfiguration.setInputDevice(inputDevice);
    mConfiguration.setOutputDevice(outputDevice);
    mConfiguration.setSampleRate(sampleRate);
    mConfiguration.setBufferSize(bufferSize);

    // Handle OSC Input Port
    if (oscPort < 0 || oscPort > MAX_OSC_INPUT_PORT) {
        oscPort = DEFAULT_OSC_INPUT_PORT;
    }
    auto const previousOscPort{ mConfiguration.getOscInputPort() };
    if (oscPort != previousOscPort) {
        mOscInputPort = oscPort;
        mConfiguration.setOscInputPort(oscPort);
        mOscReceiver->closeConnection();
        mOscReceiver->startConnection(oscPort);
    }

    // Handle recording settings
    mConfiguration.setRecordingFormat(recordingFormat);
    mConfiguration.setRecordingConfig(recordingConfig);

    // Handle CUBE distance attenuation
    mAudioProcessor->setAttenuationDbIndex(attenuationDbIndex);
    mConfiguration.setAttenuationDbIndex(attenuationDbIndex);

    auto * currentAudioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
    jassert(currentAudioDevice);

    mAudioProcessor->setAttenuationFrequencyIndex(attenuationFrequencyIndex);
    mConfiguration.setAttenuationFrequencyIndex(attenuationFrequencyIndex);
}

//==============================================================================
void MainContentComponent::timerCallback()
{
    auto & audioManager{ AudioManager::getInstance() };
    auto & audioDeviceManager{ audioManager.getAudioDeviceManager() };
    auto * audioDevice{ audioDeviceManager.getCurrentAudioDevice() };

    if (!audioDevice) {
        return;
    }

    // TODO : static variables no good
    static double cpuRunningAverage{};
    static double amountToRemove{};
    auto const currentCpuUsage{ audioDeviceManager.getCpuUsage() * 100.0 };
    if (currentCpuUsage > cpuRunningAverage) {
        cpuRunningAverage = currentCpuUsage;
        amountToRemove = 0.01;
    } else {
        cpuRunningAverage = std::max(cpuRunningAverage - amountToRemove, currentCpuUsage);
        amountToRemove *= 1.1;
    }

    auto const cpuLoad{ narrow<int>(std::round(cpuRunningAverage)) };
    mCpuUsageValue->setText(juce::String{ cpuLoad } + " %", juce::dontSendNotification);

    auto const sampleRate{ audioDevice->getCurrentSampleRate() };
    auto seconds{ static_cast<int>(static_cast<double>(audioManager.getNumSamplesRecorded()) / sampleRate) };
    auto const minute{ seconds / 60 % 60 };
    seconds = seconds % 60;
    auto const timeRecorded{ ((minute < 10) ? "0" + juce::String{ minute } : juce::String{ minute }) + " : "
                             + ((seconds < 10) ? "0" + juce::String{ seconds } : juce::String{ seconds }) };
    mTimeRecordedLabel->setText(timeRecorded, juce::dontSendNotification);

    if (mStartRecordButton->getToggleState()) {
        mStartRecordButton->setToggleState(false, juce::dontSendNotification);
    }

    if (audioManager.isRecording()) {
        mStartRecordButton->setButtonText("Stop");
    } else {
        mStartRecordButton->setButtonText("Record");
    }

    if (mAudioProcessor->isOverloaded()) {
        mCpuUsageValue->setColour(juce::Label::backgroundColourId, juce::Colours::darkred);
    } else {
        mCpuUsageValue->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    }

    for (auto * sourceInput : mInputs) {
        sourceInput->getVuMeter()->update();
    }

    for (auto * speaker : mSpeakers) {
        speaker->getVuMeter()->update();
    }

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
        auto const numOfInputs{ narrow<unsigned>(mAddInputsTextEditor->getTextValue().toString().getIntValue()) };
        if (numOfInputs < 1) {
            mAddInputsTextEditor->setText("1");
        }
        if (numOfInputs > MAX_INPUTS) {
            mAddInputsTextEditor->setText(juce::String(MAX_INPUTS));
        }

        if (mAudioProcessor->getInputPorts().size() != numOfInputs) {
            mAudioProcessor->addRemoveInput(numOfInputs);

            mInputsLock.lock();
            auto addInput{ false };
            auto const numInputPorts{ narrow<int>(mAudioProcessor->getInputPorts().size()) };
            for (int i{}; i < numInputPorts; ++i) {
                if (i >= mInputs.size()) {
                    mInputs.add(new Input{ *this, mSmallLookAndFeel, i + 1 });
                    addInput = true;
                }
            }
            if (!addInput) {
                auto const listSourceInputSize{ mInputs.size() };
                auto const jackClientInputPortSize{ narrow<int>(mAudioProcessor->getInputPorts().size()) };
                if (listSourceInputSize > jackClientInputPortSize) {
                    mInputs.removeRange(jackClientInputPortSize, listSourceInputSize - jackClientInputPortSize);
                }
            }
            mInputsLock.unlock();
        }
        unfocusAllComponents();
        updateLevelComp();
    }
}

//==============================================================================
void MainContentComponent::buttonClicked(juce::Button * button)
{
    auto & audioManager{ AudioManager::getInstance() };

    if (button == mStartRecordButton.get()) {
        if (audioManager.isRecording()) {
            audioManager.stopRecording();
            mStartRecordButton->setEnabled(false);
            mTimeRecordedLabel->setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
        } else {
            audioManager.startRecording();
            mTimeRecordedLabel->setColour(juce::Label::textColourId, mLookAndFeel.getRedColour());
        }
        mStartRecordButton->setToggleState(audioManager.isRecording(), juce::dontSendNotification);
    } else if (button == mInitRecordButton.get()) {
        if (initRecording()) {
            mStartRecordButton->setEnabled(true);
        }
    }
}

//==============================================================================
void MainContentComponent::sliderValueChanged(juce::Slider * slider)
{
    if (slider == mMasterGainOutSlider.get()) {
        mAudioProcessor->setMasterGainOut(
            std::pow(10.0f, static_cast<float>(mMasterGainOutSlider->getValue()) * 0.05f));
    }

    else if (slider == mInterpolationSlider.get()) {
        mAudioProcessor->setInterMaster(static_cast<float>(mInterpolationSlider->getValue()));
    }
}

//==============================================================================
void MainContentComponent::comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == mSpatModeCombo.get()) {
        if (mEditSpeakersWindow != nullptr && mNeedToSaveSpeakerSetup) {
            juce::AlertWindow alert(
                "The speaker configuration has changed!    ",
                "Save your changes or close the speaker configuration window before switching mode...    ",
                juce::AlertWindow::WarningIcon);
            alert.setLookAndFeel(&mLookAndFeel);
            alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
            alert.runModalLoop();
            mSpatModeCombo->setSelectedId(static_cast<int>(mAudioProcessor->getMode()) + 1,
                                          juce::NotificationType::dontSendNotification);
            return;
        }

        juce::ScopedLock const lock{ mAudioProcessor->getCriticalSection() };
        auto const newSpatMode{ static_cast<SpatMode>(mSpatModeCombo->getSelectedId() - 1) };
        mAudioProcessor->setMode(newSpatMode);
        mNeedToSaveSpeakerSetup = false;

        switch (newSpatMode) {
        case SpatMode::vbap:
        case SpatMode::lbap:
            openXmlFileSpeaker(mConfiguration.getLastSpeakerSetup_(), newSpatMode);
            mIsSpanShown = true;
            break;
        case SpatMode::hrtfVbap:
            openXmlFileSpeaker(BINAURAL_SPEAKER_SETUP_FILE, newSpatMode);
            mAudioProcessor->resetHrtf();
            mIsSpanShown = false;
            break;
        case SpatMode::stereo:
            openXmlFileSpeaker(STEREO_SPEAKER_SETUP_FILE, newSpatMode);
            mIsSpanShown = false;
            break;
        }

        if (mEditSpeakersWindow != nullptr) {
            auto const windowName{ juce::String("Speakers Setup Edition - ")
                                   + juce::String(MODE_SPAT_STRING[static_cast<int>(newSpatMode)]) + juce::String(" - ")
                                   + mCurrentSpeakerSetup.getFileName() };
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
bool MainContentComponent::initRecording() const
{
    auto const dir{ mConfiguration.getLastRecordingDirectory() };
    juce::String extF;
    juce::String extChoice;

    auto const recordingFormat{ mConfiguration.getRecordingFormat() };

    if (recordingFormat == RecordingFormat::wav) {
        extF = ".wav";
        extChoice = "*.wav,*.aif";
    } else {
        extF = ".aif";
        extChoice = "*.aif,*.wav";
    }

    auto const recordingConfig{ mConfiguration.getRecordingConfig() };

    juce::FileChooser fc{ "Choose a file to save...", dir.getFullPathName() + "/recording" + extF, extChoice, true };

    if (!fc.browseForFileToSave(true)) {
        return false;
    }

    auto const filePath{ fc.getResults().getReference(0).getFullPathName() };
    mConfiguration.setLastRecordingDirectory(juce::File{ filePath }.getParentDirectory());
    AudioManager::RecordingOptions const recordingOptions{ filePath,
                                                           recordingFormat,
                                                           recordingConfig,
                                                           narrow<double>(mSamplingRate) };
    return AudioManager::getInstance().prepareToRecord(recordingOptions, mSpeakers);
}

//==============================================================================
void MainContentComponent::resized()
{
    static constexpr auto MENU_BAR_HEIGHT = 20;
    static constexpr auto PADDING = 10;

    auto reducedLocalBounds{ getLocalBounds().reduced(2) };

    mMenuBar->setBounds(0, 0, getWidth(), MENU_BAR_HEIGHT);
    reducedLocalBounds.removeFromTop(MENU_BAR_HEIGHT);

    // Lay out the speaker view and the vertical divider.
    Component * vComps[] = { mSpeakerViewComponent.get(), mVerticalDividerBar.get(), nullptr };

    // Lay out side-by-side and resize the components' heights as well as widths.
    mVerticalLayout.layOutComponents(vComps,
                                     3,
                                     reducedLocalBounds.getX(),
                                     reducedLocalBounds.getY(),
                                     reducedLocalBounds.getWidth(),
                                     reducedLocalBounds.getHeight(),
                                     false,
                                     true);

    juce::Rectangle<int> const newMainUiBoxBounds{ mSpeakerViewComponent->getWidth() + 6,
                                                   MENU_BAR_HEIGHT,
                                                   getWidth() - (mSpeakerViewComponent->getWidth() + PADDING),
                                                   getHeight() };
    mMainUiBox->setBounds(newMainUiBoxBounds);
    mMainUiBox->correctSize(getWidth() - mSpeakerViewComponent->getWidth() - 6, 610);

    juce::Rectangle<int> const newInputsUiBoxBounds{ 0,
                                                     2,
                                                     getWidth() - (mSpeakerViewComponent->getWidth() + PADDING),
                                                     231 };
    mInputsUiBox->setBounds(newInputsUiBoxBounds);
    mInputsUiBox->correctSize(mInputs.size() * VU_METER_WIDTH_IN_PIXELS + 4, 200);

    juce::Rectangle<int> const newOutputsUiBoxBounds{ 0,
                                                      233,
                                                      getWidth() - (mSpeakerViewComponent->getWidth() + PADDING),
                                                      210 };
    mOutputsUiBox->setBounds(newOutputsUiBoxBounds);
    mOutputsUiBox->correctSize(mSpeakers.size() * VU_METER_WIDTH_IN_PIXELS + 4, 180);

    juce::Rectangle<int> const newControlUiBoxBounds{ 0,
                                                      443,
                                                      getWidth() - (mSpeakerViewComponent->getWidth() + PADDING),
                                                      145 };
    mControlUiBox->setBounds(newControlUiBoxBounds);
    mControlUiBox->correctSize(410, 145);
}
