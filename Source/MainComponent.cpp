/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MainComponent.h"

#include "AudioManager.h"
#include "LegacyLbapPosition.h"
#include "MainWindow.h"
#include "VuMeterComponent.h"
#include "constants.hpp"

//==============================================================================
static float gainToAlpha(float const gain)
{
    static constexpr auto MIN_ALPHA{ 0.1f };
    static constexpr auto MAX_ALPHA{ 1.0f };
    static constexpr auto ALPHA_RANGE{ MAX_ALPHA - MIN_ALPHA };
    static constexpr auto ALPHA_CURVE{ 0.7f };

    static constexpr dbfs_t MIN_DB{ -60.0f };
    static constexpr dbfs_t MAX_DB{ 0.0f };
    static constexpr dbfs_t DB_RANGE{ MAX_DB - MIN_DB };

    auto const clippedGain{ std::clamp(dbfs_t::fromGain(gain), MIN_DB, MAX_DB) };
    auto const ratio{ (clippedGain - MIN_DB) / DB_RANGE };
    jassert(ratio >= 0.0f && ratio <= 1.0f);
    auto const result{ std::pow(ratio, ALPHA_CURVE) * ALPHA_RANGE + MIN_ALPHA };

    return result;
}

//==============================================================================
MainContentComponent::MainContentComponent(MainWindow & mainWindow,
                                           GrisLookAndFeel & grisLookAndFeel,
                                           SmallGrisLookAndFeel & smallGrisLookAndFeel)
    : mLookAndFeel(grisLookAndFeel)
    , mSmallLookAndFeel(smallGrisLookAndFeel)
    , mMainWindow(mainWindow)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto const showSplashScreen = [&]() {
#if NDEBUG
        if (SPLASH_SCREEN_FILE.exists()) {
            mSplashScreen.reset(
                new juce::SplashScreen("SpatGRIS3", juce::ImageFileFormat::loadFrom(SPLASH_SCREEN_FILE), true));
            mSplashScreen->deleteAfterDelay(juce::RelativeTime::seconds(4), false);
            mSplashScreen.release();
        }
#endif
    };

    auto const initAudioManager = [&]() {
        auto const & audioSettings{ mData.appData.audioSettings };
        AudioManager::init(audioSettings.deviceType,
                           audioSettings.inputDevice,
                           audioSettings.outputDevice,
                           audioSettings.sampleRate,
                           audioSettings.bufferSize);
    };

    auto const initAudioProcessor = [&]() {
        mAudioProcessor = std::make_unique<AudioProcessor>();
        juce::ScopedLock const audioLock{ mAudioProcessor->getLock() };
        auto & audioManager{ AudioManager::getInstance() };
        audioManager.registerAudioProcessor(mAudioProcessor.get());
        AudioManager::getInstance().getAudioDeviceManager().addChangeListener(this);
        audioParametersChanged();
        jassert(AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice());
    };

    auto const initGui = [&]() {
        // Set look and feel
        juce::LookAndFeel::setDefaultLookAndFeel(&grisLookAndFeel);

        // Create the menu bar.
        mMenuBar.reset(new juce::MenuBarComponent(this));
        addAndMakeVisible(mMenuBar.get());

        // SpeakerViewComponent 3D view
        mSpeakerViewComponent.reset(new SpeakerViewComponent(*this));
        addAndMakeVisible(mSpeakerViewComponent.get());

        // Box Main
        mMainLayout = std::make_unique<LayoutComponent>(LayoutComponent::Orientation::vertical, grisLookAndFeel);

        // Box Inputs
        mSourcesLayout = std::make_unique<LayoutComponent>(LayoutComponent::Orientation::horizontal, grisLookAndFeel);
        mSourcesSection = std::make_unique<MainUiSection>("Inputs", mSourcesLayout.get(), mLookAndFeel);

        // Box Outputs
        mSpeakersLayout = std::make_unique<LayoutComponent>(LayoutComponent::Orientation::horizontal, grisLookAndFeel);
        mSpeakersSection = std::make_unique<MainUiSection>("Outputs", mSpeakersLayout.get(), mLookAndFeel);

        // Box Control
        mControlUiBox = std::make_unique<Box>(mLookAndFeel, "Controls");

        mMainLayout->addSection(mSourcesSection.get()).withRelativeSize(1.0f);
        mMainLayout->addSection(mSpeakersSection.get()).withRelativeSize(1.0f);
        // mMainLayout->addSection(mControlUiBox.get()).withChildMinSize();

        addAndMakeVisible(mMainLayout.get());

        // Components in Box Control
        mCpuUsageLabel.reset(addLabel("CPU usage", "CPU usage", 0, 0, 80, 28, mControlUiBox->getContent()));
        mCpuUsageLabel->setText("CPU usage : ", juce::dontSendNotification);
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

        mNumSourcesTextEditor.reset(
            addTextEditor("Inputs :", "0", "Numbers of Inputs", 122, 83, 43, 22, mControlUiBox->getContent()));
        mNumSourcesTextEditor->setInputRestrictions(3, "0123456789");

        mInitRecordButton.reset(
            addButton("Init Recording", "Init Recording", 268, 48, 103, 24, mControlUiBox->getContent()));

        mStartRecordButton.reset(
            addButton("Record", "Start/Stop Record", 268, 83, 60, 24, mControlUiBox->getContent()));
        mStartRecordButton->setEnabled(false);

        mTimeRecordedLabel.reset(addLabel("00:00", "Record time", 327, 83, 50, 24, mControlUiBox->getContent()));

        mSpatModeComponent.setBounds(300, 83, 150, 50);
        mControlUiBox->addAndMakeVisible(mSpatModeComponent);

        // Set up the layout and resize bars.
        mVerticalLayout.setItemLayout(0,
                                      -0.2,
                                      -0.8,
                                      -0.435); // width of the speaker view must be between 20% and 80%, preferably 50%
        mVerticalLayout.setItemLayout(1, 8, 8, 8); // the vertical divider drag-bar thing is always 8 pixels wide
        mVerticalLayout.setItemLayout(
            2,
            150.0,
            -1.0,
            -0.565); // right panes must be at least 150 pixels wide, preferably 50% of the total width
        mVerticalDividerBar.reset(new juce::StretchableLayoutResizerBar(&mVerticalLayout, 1, true));
        addAndMakeVisible(mVerticalDividerBar.get());

        mMasterGainOutSlider->setValue(0.0, juce::dontSendNotification);
        mInterpolationSlider->setValue(0.1, juce::dontSendNotification);

        // Default application window size.
        setSize(1285, 610);

        // Restore last vertical divider position and speaker view cam distance.
        auto const sashPosition{ mData.appData.sashPosition };
        auto const trueSize{ narrow<int>(std::round(narrow<double>(getWidth() - 3) * std::abs(sashPosition))) };
        mVerticalLayout.setItemPosition(1, trueSize);

        mSpeakerViewComponent->setCameraPosition(mData.appData.cameraPosition);
    };

    auto const startOsc = [&]() {
        mOscReceiver.reset(new OscInput(*this));
        mOscReceiver->startConnection(mData.project.oscPort);
    };

    auto const initAppData = [&]() { mData.appData = mConfiguration.load(); };
    auto const initProject = [&]() {
        if (juce::File{ mData.appData.lastProject }.existsAsFile()) {
            loadProject(mData.appData.lastProject, LoadProjectOption::dontRemoveInvalidDirectOuts);
        } else {
            loadProject(DEFAULT_PROJECT_FILE, LoadProjectOption::dontRemoveInvalidDirectOuts);
        }
    };
    auto const initSpeakerSetup = [&]() {
        auto const spatMode{ mData.appData.spatMode };
        switch (spatMode) {
        case SpatMode::hrtfVbap:
        case SpatMode::stereo:
            handleSpatModeChanged(mData.appData.spatMode);
            return;
        case SpatMode::vbap:
        case SpatMode::lbap:
            loadSpeakerSetup(mData.appData.lastSpeakerSetup, spatMode);
            return;
        }
        jassertfalse;
    };

    auto const initCommandManager = [&]() {
        auto & commandManager{ mMainWindow.getApplicationCommandManager() };
        commandManager.registerAllCommandsForTarget(this);
    };

    // Load app config
    showSplashScreen();
    initAppData();
    initGui();
    initAudioManager();
    initAudioProcessor();

    juce::ScopedLock const audioLock{ mAudioProcessor->getLock() };

    initProject();
    initSpeakerSetup();
    startOsc();
    initCommandManager();

    // End layout and start refresh timer.
    resized();
    startTimerHz(24);
}

//==============================================================================
MainContentComponent::~MainContentComponent()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mOscReceiver.reset();

    {
        juce::ScopedWriteLock const lock{ mLock };

        auto const bounds{ MainWindow::getMainAppWindow()->DocumentWindow::getBounds() };
        mData.appData.windowX = bounds.getX();
        mData.appData.windowY = bounds.getY();
        mData.appData.windowWidth = bounds.getWidth();
        mData.appData.windowHeight = bounds.getHeight();
        mData.appData.sashPosition = mVerticalLayout.getItemCurrentRelativeSize(0);
        mData.appData.cameraPosition = mSpeakerViewComponent->getCameraPosition();

        mConfiguration.save(mData.appData);
    }

    mSpeakerViewComponent.reset();
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
    JUCE_ASSERT_MESSAGE_THREAD;

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
    JUCE_ASSERT_MESSAGE_THREAD;

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
    JUCE_ASSERT_MESSAGE_THREAD;

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
    JUCE_ASSERT_MESSAGE_THREAD;

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
    JUCE_ASSERT_MESSAGE_THREAD;

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
    JUCE_ASSERT_MESSAGE_THREAD;

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
juce::ApplicationCommandTarget * MainContentComponent::getNextCommandTarget()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return findFirstTargetParentComponent();
}

//==============================================================================
// Menu item action handlers.
void MainContentComponent::handleNewProject()
{
    JUCE_ASSERT_MESSAGE_THREAD;

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

    loadProject(DEFAULT_PROJECT_FILE, LoadProjectOption::removeInvalidDirectOuts);
}

//==============================================================================
void MainContentComponent::loadProject(juce::File const & file, LoadProjectOption const loadProjectOption)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    jassert(file.existsAsFile());

    juce::XmlDocument xmlDoc{ file };
    auto const mainXmlElem{ xmlDoc.getDocumentElement() };

    if (!mainXmlElem) {
        // Formatting error
        juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon,
                                          "Error in Open Project !",
                                          "Your file is corrupted !\n" + file.getFullPathName() + "\n"
                                              + xmlDoc.getLastParseError());
        return;
    }

    if (mainXmlElem->hasTagName("SpeakerSetup") || mainXmlElem->hasTagName(SpeakerSetup::XmlTags::MAIN_TAG)) {
        // Wrong file type
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                          "Error in Open Project !",
                                          "You are trying to open a Speaker Setup instead of a project file !");
        return;
    }

    auto projectData{ SpatGrisProjectData::fromXml(*mainXmlElem) };
    if (!projectData) {
        // Missing params
        juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon,
                                          "Unable to read project file !",
                                          "One or more mandatory parameters are missing. Your file might be corrupt.");
        loadDefaultSpeakerSetup(SpatMode::vbap);
        return;
    }

    mData.project = std::move(*projectData);
    mData.appData.lastProject = file.getFullPathName();

    mNumSourcesTextEditor->setText(juce::String{ mData.project.sources.size() }, false);
    mMasterGainOutSlider->setValue(mData.project.masterGain.get(), juce::dontSendNotification);
    mInterpolationSlider->setValue(mData.project.spatGainsInterpolation, juce::dontSendNotification);

    if (loadProjectOption == LoadProjectOption::removeInvalidDirectOuts) {
        removeInvalidDirectOuts();
    }

    updateAudioProcessor();
    updateViewportConfig();
    setTitle();
    refreshSourceVuMeterComponents();
}

//==============================================================================
void MainContentComponent::handleOpenProject()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    juce::File const lastOpenProject{ mData.appData.lastProject };
    auto const dir{ lastOpenProject.getParentDirectory() };
    auto const filename{ lastOpenProject.getFileName() };

    juce::FileChooser fc("Choose a file to open...", dir.getFullPathName() + "/" + filename, "*.xml", true);

    if (!fc.browseForFileToOpen()) {
        return;
    }

    auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
    juce::AlertWindow alert("Open Project !",
                            "You want to load : " + chosen + "\nEverything not saved will be lost !",
                            juce::AlertWindow::WarningIcon);
    alert.setLookAndFeel(&mLookAndFeel);
    alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    alert.addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
    if (alert.runModalLoop() != 1) {
        return;
    }

    loadProject(chosen, LoadProjectOption::removeInvalidDirectOuts);
}

//==============================================================================
void MainContentComponent::handleSaveProject()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    juce::File const lastOpenProject{ mData.appData.lastProject };
    if (!lastOpenProject.existsAsFile() || lastOpenProject == DEFAULT_PROJECT_FILE) {
        handleSaveAsProject();
    }
    saveProject(lastOpenProject.getFullPathName());
}

//==============================================================================
void MainContentComponent::handleSaveAsProject()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    juce::File const lastOpenProject{ mData.appData.lastProject == DEFAULT_PROJECT_FILE.getFullPathName()
                                          ? juce::File::getSpecialLocation(
                                              juce::File::SpecialLocationType::userDesktopDirectory)
                                          : mData.appData.lastProject };

    juce::FileChooser fc{ "Choose a file to save...", lastOpenProject.getFullPathName(), "*.xml", true };

    if (fc.browseForFileToSave(true)) {
        auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
        saveProject(chosen);
    }
}

//==============================================================================
void MainContentComponent::handleOpenSpeakerSetup()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto const initialFile{ juce::File{ mData.appData.lastSpeakerSetup }.existsAsFile()
                                ? mData.appData.lastSpeakerSetup
                                : juce::File::getSpecialLocation(
                                    juce::File::SpecialLocationType::userDesktopDirectory) };

    juce::FileChooser fc{ "Choose a file to open...", initialFile, "*.xml", true };

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
            loadSpeakerSetup(chosen);
        }
    }
}

//==============================================================================
void MainContentComponent::handleSaveAsSpeakerSetup()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    switch (mData.appData.spatMode) {
    case SpatMode::hrtfVbap:
    case SpatMode::stereo:
        juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon,
                                          "Warning",
                                          "Binaural and stereo setups should not be modified.",
                                          "Ok",
                                          this);
        return;
    case SpatMode::lbap:
    case SpatMode::vbap:
        break;
    }

    auto const initialFile{ mData.appData.lastSpeakerSetup == DEFAULT_SPEAKER_SETUP_FILE.getFullPathName()
                                ? juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory)
                                : mData.appData.lastSpeakerSetup };

    juce::FileChooser fc{ "Choose a file to save...", initialFile, "*.xml", true };

    if (fc.browseForFileToSave(true)) {
        auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
        saveSpeakerSetup(chosen);
    }
}

//==============================================================================
void MainContentComponent::closeSpeakersConfigurationWindow()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mEditSpeakersWindow.reset();
}

//==============================================================================
void MainContentComponent::handleShowSpeakerEditWindow()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    switch (mData.appData.spatMode) {
    case SpatMode::hrtfVbap:
    case SpatMode::stereo:
        juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon,
                                          "Warning",
                                          "The Binaural and Stereo speaker setups cannot be modified.",
                                          "Ok",
                                          this);
        return;
    case SpatMode::lbap:
    case SpatMode::vbap:
        break;
    }

    juce::Rectangle<int> const result{ getScreenX() + mSpeakerViewComponent->getWidth() + 20,
                                       getScreenY() + 20,
                                       850,
                                       600 };
    if (mEditSpeakersWindow == nullptr) {
        auto const windowName = juce::String("Speakers Setup Edition - ")
                                + juce::String(SPAT_MODE_STRINGS[static_cast<int>(mData.appData.spatMode)]) + " - "
                                + mData.appData.lastSpeakerSetup;
        mEditSpeakersWindow
            = std::make_unique<EditSpeakersWindow>(windowName, mLookAndFeel, *this, mData.appData.lastProject);
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
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    if (mPropertiesWindow == nullptr) {
        mPropertiesWindow.reset(new SettingsWindow{ *this,
                                                    mData.appData.recordingOptions,
                                                    mData.project.lbapDistanceAttenuationData,
                                                    mData.project.oscPort,
                                                    mLookAndFeel });
    }
}

//==============================================================================
void MainContentComponent::handleShow2DView()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

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
void MainContentComponent::handleShowAbout()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (!mAboutWindow) {
        mAboutWindow.reset(new AboutWindow{ "About SpatGRIS", mLookAndFeel, *this });
    }
}

//==============================================================================
void MainContentComponent::handleOpenManual()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (MANUAL_FILE.exists()) {
        juce::Process::openDocument("file:" + MANUAL_FILE.getFullPathName(), juce::String());
    }
}

//==============================================================================
void MainContentComponent::handleShowNumbers()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSpeakerNumbers };
    var = !var;
    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSpeakers()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSpeakers };
    var = !var;
    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowTriplets()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::ScopedReadLock const readLock{ mLock };
    auto const newState{ !mData.appData.viewSettings.showSpeakerTriplets };
    if ((mData.appData.spatMode == SpatMode::lbap || mData.appData.spatMode == SpatMode::stereo) && newState) {
        juce::AlertWindow alert("Can't draw triplets !",
                                "Triplets are not effective with the CUBE or STEREO modes.",
                                juce::AlertWindow::InfoIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Close", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        return;
    }

    juce::ScopedWriteLock const writeLock{ mLock };
    mData.appData.viewSettings.showSpeakerTriplets = newState;
    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSourceLevel()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSourceActivity };
    var = !var;
    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSpeakerLevel()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSpeakerLevels };
    var = !var;
    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSphere()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSphereOrCube };
    var = !var;
    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::handleResetInputPositions()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & viewPortData{ mSpeakerViewComponent->getData() };
    auto & gainMatrix{ mAudioProcessor->getAudioData().spatGainMatrix };
    for (auto const source : mData.project.sources) {
        // reset positions
        source.value->position = tl::nullopt;
        source.value->vector = tl::nullopt;

        {
            // reset 3d view
            auto & exchanger{ viewPortData.sources[source.key] };
            auto * sourceTicket{ exchanger.acquire() };
            sourceTicket->get() = tl::nullopt;
            exchanger.setMostRecent(sourceTicket);
        }
        {
            // reset gain matrix
            auto & exchanger{ gainMatrix[source.key] };
            auto * gains{ exchanger.acquire() };
            gains->get() = SpeakersSpatGains{};
            exchanger.setMostRecent(gains);
        }
    }

    updateAudioProcessor();
}

//==============================================================================
void MainContentComponent::handleResetMeterClipping()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    for (auto vuMeter : mSourceVuMeterComponents) {
        vuMeter.value->resetClipping();
    }
    for (auto vuMeter : mSpeakerVuMeters) {
        vuMeter.value->resetClipping();
    }
}

//==============================================================================
void MainContentComponent::handleColorizeInputs()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    float hue{};
    auto const inc{ 1.0f / static_cast<float>(mData.project.sources.size() + 1) };
    for (auto source : mData.project.sources) {
        auto const colour{ juce::Colour::fromHSV(hue, 1, 0.75, 1) };
        source.value->colour = colour;
        mSourceVuMeterComponents[source.key].setSourceColour(colour);
        hue += inc;
    }
}

//==============================================================================
// Command manager methods.
void MainContentComponent::getAllCommands(juce::Array<juce::CommandID> & commands)
{
    JUCE_ASSERT_MESSAGE_THREAD;

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
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

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
        result.setTicked(mData.appData.viewSettings.showSpeakerNumbers);
        break;
    case MainWindow::ShowSpeakersID:
        result.setInfo("Show Speakers", "Show speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSpeakers);
        break;
    case MainWindow::ShowTripletsID:
        result.setInfo("Show Speaker Triplets", "Show speaker triplets on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('T', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSpeakerTriplets);
        break;
    case MainWindow::ShowSourceLevelID:
        result.setInfo("Show Source Activity", "Activate brightness on sources on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('A', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSourceActivity);
        break;
    case MainWindow::ShowSpeakerLevelID:
        result.setInfo("Show Speaker Level", "Activate brightness on speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('L', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSpeakerLevels);
        break;
    case MainWindow::ShowSphereID:
        result.setInfo("Show Sphere/Cube", "Show the sphere on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSphereOrCube);
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
    JUCE_ASSERT_MESSAGE_THREAD;

    if (MainWindow::getMainAppWindow()) {
        switch (info.commandID) {
        case MainWindow::NewProjectID:
            handleNewProject();
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
            handleColorizeInputs();
            break;
        case MainWindow::ResetInputPosID:
            handleResetInputPositions();
            break;
        case MainWindow::ResetMeterClipping:
            handleResetMeterClipping();
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
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const audioLock{ mAudioProcessor->getLock() };

    auto * currentAudioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
    jassert(currentAudioDevice);
    if (!currentAudioDevice) {
        return;
    }

    auto const deviceTypeName{ currentAudioDevice->getTypeName() };
    auto const setup{ AudioManager::getInstance().getAudioDeviceManager().getAudioDeviceSetup() };

    auto const sampleRate{ currentAudioDevice->getCurrentSampleRate() };
    auto const bufferSize{ currentAudioDevice->getCurrentBufferSizeSamples() };
    auto const inputCount{ currentAudioDevice->getActiveInputChannels().countNumberOfSetBits() };
    auto const outputCount{ currentAudioDevice->getActiveOutputChannels().countNumberOfSetBits() };

    juce::ScopedWriteLock const lock{ mLock };

    mData.appData.audioSettings.sampleRate = setup.sampleRate;
    mData.appData.audioSettings.bufferSize = setup.bufferSize;
    mData.appData.audioSettings.deviceType = deviceTypeName;
    mData.appData.audioSettings.inputDevice = setup.inputDeviceName;
    mData.appData.audioSettings.outputDevice = setup.outputDeviceName;

    AudioManager::getInstance().setBufferSize(bufferSize);

    mSampleRateLabel->setText(juce::String{ narrow<unsigned>(sampleRate) } + " Hz",
                              juce::NotificationType::dontSendNotification);
    mBufferSizeLabel->setText(juce::String{ bufferSize } + " samples", juce::NotificationType::dontSendNotification);
    mChannelCountLabel->setText("I : " + juce::String{ inputCount } + " - O : " + juce::String{ outputCount },
                                juce::dontSendNotification);
}

//==============================================================================
juce::PopupMenu MainContentComponent::getMenuForIndex(int /*menuIndex*/, const juce::String & menuName)
{
    JUCE_ASSERT_MESSAGE_THREAD;

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
        if (mSpatAlgorithm->hasTriplets()) {
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
    JUCE_ASSERT_MESSAGE_THREAD;
}

//==============================================================================
// Exit functions.
bool MainContentComponent::isProjectModified() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto const savedElement{ juce::XmlDocument{ juce::File{ mData.appData.lastProject } }.getDocumentElement() };
    jassert(savedElement);
    if (!savedElement) {
        return true;
    }
    auto const savedProject{ SpatGrisProjectData::fromXml(*savedElement) };
    jassert(savedProject);
    if (!savedProject) {
        return true;
    }
    return mData.project != *savedProject;
}

//==============================================================================
bool MainContentComponent::isSpeakerSetupModified() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    switch (mData.appData.spatMode) {
    case SpatMode::hrtfVbap:
    case SpatMode::stereo:
        // BINAURAL & STEREO are non-modifiable setups
        return false;
    case SpatMode::lbap:
    case SpatMode::vbap:
        break;
    default:
        jassertfalse;
    }

    auto const savedElement{ juce::XmlDocument{ juce::File{ mData.appData.lastSpeakerSetup } }.getDocumentElement() };
    jassert(savedElement);
    if (!savedElement) {
        return true;
    }

    auto const savedSpeakerSetup{ SpeakerSetup::fromXml(*savedElement) };
    jassert(savedSpeakerSetup);
    if (!savedSpeakerSetup) {
        return true;
    }
    return mData.speakerSetup != savedSpeakerSetup->first;
}

//==============================================================================
bool MainContentComponent::exitApp()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    static constexpr auto EXIT_APP = true;
    static constexpr auto DONT_EXIT_APP = false;

    juce::XmlDocument lastProjectDocument{ mData.appData.lastProject };
    auto lastProjectElement{ lastProjectDocument.getDocumentElement() };
    std::unique_ptr<juce::XmlElement> currentProjectElement{ mData.appData.toXml() };

    if (!isProjectModified()) {
        return EXIT_APP;
    }

    static constexpr auto BUTTON_SAVE = 1;
    static constexpr auto BUTTON_CANCEL = 0;
    static constexpr auto BUTTON_EXIT = 2;
    juce::AlertWindow alert("Exit SpatGRIS !",
                            "Do you want to save the current project ?",
                            juce::AlertWindow::InfoIcon);
    alert.setLookAndFeel(&mLookAndFeel);
    alert.addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    alert.addButton("Exit", 2, juce::KeyPress(juce::KeyPress::deleteKey));
    auto const action = alert.runModalLoop();

    if (action == BUTTON_CANCEL) {
        return DONT_EXIT_APP;
    }

    if (action == BUTTON_EXIT) {
        return EXIT_APP;
    }

    jassert(action == BUTTON_SAVE);
    alert.setVisible(false);
    juce::ModalComponentManager::getInstance()->cancelAllModalComponents();

    juce::FileChooser fc("Choose a file to save...", mData.appData.lastProject, "*.xml", true);
    if (!fc.browseForFileToSave(true)) {
        return DONT_EXIT_APP;
    }

    auto const chosen{ fc.getResults().getReference(0).getFullPathName() };
    saveProject(chosen);

    return EXIT_APP;
}

//==============================================================================
void MainContentComponent::updatePeaks()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto & viewportData{ mSpeakerViewComponent->getData() };

    auto & audioData{ mAudioProcessor->getAudioData() };
    auto *& sourcePeaksTicket{ mData.mostRecentSourcePeaks };
    audioData.sourcePeaks.getMostRecent(sourcePeaksTicket);
    if (!sourcePeaksTicket) {
        return;
    }
    auto const & sourcePeaks{ sourcePeaksTicket->get() };
    for (auto const sourceData : mData.project.sources) {
        auto const & peak{ sourcePeaks[sourceData.key] };
        auto const dbPeak{ dbfs_t::fromGain(peak) };
        mSourceVuMeterComponents[sourceData.key].setLevel(dbPeak);

        if (!sourceData.value->position) {
            continue;
        }
        if (!viewportData.sources.contains(sourceData.key)) {
            viewportData.sources.add(sourceData.key);
        }

        auto & exchanger{ viewportData.sources[sourceData.key] };
        auto * ticket{ exchanger.acquire() };
        ticket->get() = sourceData.value->toViewportData(gainToAlpha(peak));
        exchanger.setMostRecent(ticket);
    }

    auto *& speakerPeaksTicket{ mData.mostRecentSpeakerPeaks };
    audioData.speakerPeaks.getMostRecent(speakerPeaksTicket);
    if (speakerPeaksTicket == nullptr) {
        return;
    }
    auto const & speakerPeaks{ speakerPeaksTicket->get() };
    for (auto const speaker : mData.speakerSetup.speakers) {
        auto const & peak{ speakerPeaks[speaker.key] };
        auto const dbPeak{ dbfs_t::fromGain(peak) };
        mSpeakerVuMeters[speaker.key].setLevel(dbPeak);

        auto & exchanger{ viewportData.speakersAlpha[speaker.key] };
        auto * ticket{ exchanger.acquire() };
        ticket->get() = gainToAlpha(peak);
        exchanger.setMostRecent(ticket);
    }
}

//==============================================================================
void MainContentComponent::setLbapAttenuationDb(dbfs_t const attenuation)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.lbapDistanceAttenuationData.attenuation = attenuation;

    updateAudioProcessor();
}

//==============================================================================
void MainContentComponent::setLbapAttenuationFreq(hz_t const freq)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.lbapDistanceAttenuationData.freq = freq;

    updateAudioProcessor();
}

//==============================================================================
void MainContentComponent::refreshSourceVuMeterComponents()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    mSourcesLayout->clear();
    mSourceVuMeterComponents.clear();

    auto x{ 3 };
    for (auto source : mData.project.sources) {
        auto newVuMeter{ std::make_unique<SourceVuMeterComponent>(source.key,
                                                                  source.value->directOut,
                                                                  source.value->colour,
                                                                  *this,
                                                                  mSmallLookAndFeel) };
        mSourcesLayout->addSection(newVuMeter.get()).withChildMinSize();
        // juce::Rectangle<int> const bounds{ x, 5, VU_METER_WIDTH, 200 };
        // newVuMeter->setBounds(bounds);
        mSourceVuMeterComponents.add(source.key, std::move(newVuMeter));
        x += VU_METER_WIDTH;
    }

    resized();
}

//==============================================================================
void MainContentComponent::refreshSpeakerVuMeterComponents()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    mSpeakersLayout->clear();
    mSpeakerVuMeters.clear();

    auto x{ 3 };
    for (auto const outputPatch : mData.speakerSetup.order) {
        auto newVuMeter{ std::make_unique<SpeakerVuMeterComponent>(outputPatch, *this, mSmallLookAndFeel) };
        mSpeakersLayout->addSection(newVuMeter.get()).withChildMinSize();
        juce::Rectangle<int> const bounds{ x, 5, VU_METER_WIDTH, 200 };
        newVuMeter->setBounds(bounds);
        mSpeakerVuMeters.add(outputPatch, std::move(newVuMeter));
        x += VU_METER_WIDTH;
    }

    resized();
}

//==============================================================================
void MainContentComponent::removeInvalidDirectOuts()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    bool madeChange{};

    for (auto & source : mData.project.sources) {
        auto & directOut{ source.value->directOut };
        if (!directOut) {
            continue;
        }

        if (mData.speakerSetup.speakers.contains(*directOut)) {
            continue;
        }

        directOut = tl::nullopt;
        madeChange = true;
    }

    if (madeChange) {
        refreshSourceVuMeterComponents();
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::AlertIconType::WarningIcon,
            "Dropped direct out",
            "One or more direct outs were disabled because the speakers where not available anymore.",
            "Ok",
            this);
    }
}

//==============================================================================
void MainContentComponent::handleSourcePositionChanged(source_index_t const sourceIndex,
                                                       radians_t const azimuth,
                                                       radians_t const elevation,
                                                       float const length,
                                                       float const newAzimuthSpan,
                                                       float const newZenithSpan)
{
    jassert(juce::Thread::getCurrentThread()->getThreadName() == "JUCE OSC server");
    juce::ScopedWriteLock const lock{ mLock };

    if (!mData.project.sources.contains(sourceIndex)) {
        return;
    }

    auto const getCorrectPosition = [&]() {
        switch (mData.appData.spatMode) {
        case SpatMode::vbap:
        case SpatMode::hrtfVbap:
        case SpatMode::stereo:
            return PolarVector{ azimuth, elevation, length }.normalized();
        case SpatMode::lbap: {
            return LegacyLbapPosition{ azimuth, elevation, length }.toPolar();
        }
        }
        jassertfalse;
        return PolarVector{};
    };

    auto const correctedPosition{ getCorrectPosition() };
    auto & source{ mData.project.sources[sourceIndex] };

    if (correctedPosition == source.vector && newAzimuthSpan == source.azimuthSpan
        && newZenithSpan == source.zenithSpan) {
        return;
    }

    source.vector = correctedPosition;
    source.position = source.vector->toCartesian();
    source.azimuthSpan = newAzimuthSpan;
    source.zenithSpan = newZenithSpan;

    auto & audioData{ mAudioProcessor->getAudioData() };
    auto & exchanger{ audioData.spatGainMatrix[sourceIndex] };
    auto * ticket{ exchanger.acquire() };
    mSpatAlgorithm->computeSpeakerGains(source, ticket->get());
    exchanger.setMostRecent(ticket);

    if (mData.appData.spatMode == SpatMode::lbap) {
        mAudioProcessor->getAudioData().lbapSourceDistances[sourceIndex] = length;
    }
}

//==============================================================================
void MainContentComponent::resetSourcePosition(source_index_t const sourceIndex)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.sources[sourceIndex].position = tl::nullopt;
    mData.project.sources[sourceIndex].vector = tl::nullopt;
}

//==============================================================================
void MainContentComponent::loadDefaultSpeakerSetup(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    switch (spatMode) {
    case SpatMode::vbap:
    case SpatMode::lbap:
        loadSpeakerSetup(DEFAULT_SPEAKER_SETUP_FILE, spatMode);
        return;
    case SpatMode::hrtfVbap:
        loadSpeakerSetup(BINAURAL_SPEAKER_SETUP_FILE, spatMode);
        return;
    case SpatMode::stereo:
        loadSpeakerSetup(STEREO_SPEAKER_SETUP_FILE, spatMode);
        return;
    }
    jassertfalse;
}

//==============================================================================
void MainContentComponent::setRecordingFormat(RecordingFormat const format)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.appData.recordingOptions.format = format;
}

//==============================================================================
void MainContentComponent::setRecordingFileType(RecordingFileType const fileType)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.appData.recordingOptions.fileType = fileType;
}

//==============================================================================
void MainContentComponent::handleSpeakerOnlyDirectOutChanged(output_patch_t const outputPatch, bool const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & val{ mData.speakerSetup.speakers[outputPatch].isDirectOutOnly };
    if (state != val) {
        val = state;
        updateAudioProcessor();
        updateViewportConfig();
    }
}

//==============================================================================
void MainContentComponent::handleSpeakerOutputPatchChanged(output_patch_t const oldOutputPatch,
                                                           output_patch_t const newOutputPatch)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & speakers{ mData.speakerSetup.speakers };
    speakers.add(newOutputPatch, std::make_unique<SpeakerData>(speakers[oldOutputPatch]));
    speakers.remove(oldOutputPatch);

    auto & order{ mData.speakerSetup.order };
    order.set(order.indexOf(oldOutputPatch), newOutputPatch);

    refreshSpeakerVuMeterComponents();
}

//==============================================================================
void MainContentComponent::handleSetSpeakerGain(output_patch_t const outputPatch, dbfs_t const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.speakerSetup.speakers[outputPatch].gain = gain;

    updateAudioProcessor();
}

//==============================================================================
void MainContentComponent::handleSetSpeakerHighPassFreq(output_patch_t const outputPatch, hz_t const freq)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    if (freq == hz_t{ 0.0f }) {
        mData.speakerSetup.speakers[outputPatch].highpassData.reset();
    } else {
        mData.speakerSetup.speakers[outputPatch].highpassData = SpeakerHighpassData{ freq };
    }
}

//==============================================================================
void MainContentComponent::handlePinkNoiseGainChanged(tl::optional<dbfs_t> const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    if (gain != mData.pinkNoiseLevel) {
        mData.pinkNoiseLevel = gain;
        mAudioProcessor->setAudioConfig(mData.toAudioConfig());
    }
}

//==============================================================================
void MainContentComponent::handleSourceColorChanged(source_index_t const sourceIndex, juce::Colour const colour)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.sources[sourceIndex].colour = colour;
    mSourceVuMeterComponents[sourceIndex].setSourceColour(colour);
}

//==============================================================================
void MainContentComponent::handleSourceStateChanged(source_index_t const sourceIndex, PortState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.sources[sourceIndex].state = state;

    auto const isAtLeastOneSourceSolo{ std::any_of(
        mData.project.sources.cbegin(),
        mData.project.sources.cend(),
        [](SourcesData::ConstNode const & node) { return node.value->state == PortState::solo; }) };

    updateAudioProcessor();
    mSourceVuMeterComponents[sourceIndex].setState(state, isAtLeastOneSourceSolo);
}

//==============================================================================
void MainContentComponent::handleSpeakerSelected(juce::Array<output_patch_t> const selection)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    for (auto const speaker : mData.speakerSetup.speakers) {
        auto const isSelected{ selection.contains(speaker.key) };

        if (speaker.value->isSelected == isSelected) {
            continue;
        }

        speaker.value->isSelected = isSelected;
        mSpeakerVuMeters[speaker.key].setSelected(isSelected);

        if (!isSelected || !mEditSpeakersWindow) {
            continue;
        }

        mEditSpeakersWindow->selectSpeaker(speaker.key);
    }

    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::handleSpeakerStateChanged(output_patch_t const outputPatch, PortState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.speakerSetup.speakers[outputPatch].state = state;
    updateAudioProcessor();

    auto const isAtLeastOneSpeakerSolo{ std::any_of(
        mData.speakerSetup.speakers.cbegin(),
        mData.speakerSetup.speakers.cend(),
        [](SpeakersData::ConstNode const & node) { return node.value->state == PortState::solo; }) };

    mSpeakerVuMeters[outputPatch].setState(state, isAtLeastOneSpeakerSolo);
    // TODO : update 3D view ?
}

//==============================================================================
void MainContentComponent::handleSourceDirectOutChanged(source_index_t const sourceIndex,
                                                        tl::optional<output_patch_t> const outputPatch)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.sources[sourceIndex].directOut = outputPatch;

    updateAudioProcessor();
    mSourceVuMeterComponents[sourceIndex].setDirectOut(outputPatch);
}

//==============================================================================
void MainContentComponent::handleSpatModeChanged(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    jassert(narrow<int>(spatMode) >= 0 && narrow<int>(spatMode) <= narrow<int>(SpatMode::stereo));

    if (mData.appData.spatMode == spatMode && mSpatAlgorithm) {
        return;
    }

    if (isSpeakerSetupModified()) {
        juce::AlertWindow alert(
            "The speaker configuration has changed!    ",
            "Save your changes or close the speaker configuration window before switching mode...    ",
            juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Ok", 0, juce::KeyPress(juce::KeyPress::returnKey));
        alert.runModalLoop();
        mSpatModeComponent.setSpatMode(mData.appData.spatMode);
        return;
    }

    mSpatModeComponent.setSpatMode(spatMode);

    // handle speaker setup change
    auto const getForcedSpeakerSetup = [&]() -> tl::optional<juce::File> {
        switch (spatMode) {
        case SpatMode::hrtfVbap:
            return BINAURAL_SPEAKER_SETUP_FILE;
        case SpatMode::stereo:
            return STEREO_SPEAKER_SETUP_FILE;
        case SpatMode::lbap:
        case SpatMode::vbap:
            switch (mData.appData.spatMode) {
            case SpatMode::hrtfVbap:
            case SpatMode::stereo:
                return mData.appData.lastSpeakerSetup;
            case SpatMode::lbap:
            case SpatMode::vbap:
                // keep the current setup and adapt it to the new spatMode
                return tl::nullopt;
            }
        }
        jassertfalse;
        return tl::nullopt;
    };

    mData.appData.viewSettings.showSpeakerTriplets = false;

    auto const forcedSpeakerSetup{ getForcedSpeakerSetup() };
    if (forcedSpeakerSetup) {
        loadSpeakerSetup(*forcedSpeakerSetup, spatMode);
        mAudioProcessor->resetHrtf();
        return;
    }

    // speaker setup stays the same
    mData.appData.spatMode = spatMode;
    mSpatAlgorithm = AbstractSpatAlgorithm::make(spatMode, mData.speakerSetup.speakers);

    updateAudioProcessor();
    updateViewportConfig();

    if (mEditSpeakersWindow != nullptr) {
        auto const windowName{ juce::String("Speakers Setup Edition - ")
                               + juce::String(SPAT_MODE_STRINGS[static_cast<int>(spatMode)]) + juce::String(" - ")
                               + mData.appData.lastSpeakerSetup };
        mEditSpeakersWindow->setName(windowName);
    }
}

//==============================================================================
void MainContentComponent::handleMasterGainChanged(dbfs_t const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.masterGain = gain;
    mMasterGainOutSlider->setValue(gain.get(), juce::dontSendNotification);

    updateAudioProcessor();
}

//==============================================================================
void MainContentComponent::handleGainInterpolationChanged(float const interpolation)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.spatGainsInterpolation = interpolation;
    mInterpolationSlider->setValue(interpolation, juce::dontSendNotification);

    updateAudioProcessor();
}

//==============================================================================
void MainContentComponent::handleNewSpeakerPosition(output_patch_t const outputPatch, CartesianVector const & position)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & speaker{ mData.speakerSetup.speakers[outputPatch] };
    speaker.vector = PolarVector::fromCartesian(position);
    speaker.position = position;

    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::handleNewSpeakerPosition(output_patch_t const outputPatch, PolarVector const & position)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & speaker{ mData.speakerSetup.speakers[outputPatch] };
    speaker.vector = position;
    speaker.position = position.toCartesian();

    // TODO : re-init spat algorithm?
}

//==============================================================================
void MainContentComponent::updateAudioProcessor() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    mAudioProcessor->setAudioConfig(mData.toAudioConfig());
}

//==============================================================================
void MainContentComponent::updateViewportConfig() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto const newConfig{ mData.toViewportConfig() };
    if (newConfig.viewSettings.showSpeakerTriplets && mSpatAlgorithm && mSpatAlgorithm->hasTriplets()) {
        mSpeakerViewComponent->setTriplets(mSpatAlgorithm->getTriplets());
    }
    mSpeakerViewComponent->setConfig(newConfig, mData.project.sources);
}

//==============================================================================
void MainContentComponent::handleSetShowTriplets(bool const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.appData.viewSettings.showSpeakerTriplets = state;
    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::setSourceState(source_index_t const sourceIndex, PortState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.sources[sourceIndex].state = state;
    updateAudioProcessor();
    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::setSpeakerState(output_patch_t const outputPatch, PortState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.speakerSetup.speakers[outputPatch].state = state;
    updateAudioProcessor();
    updateViewportConfig();
}

//==============================================================================
void MainContentComponent::reorderSpeakers(juce::Array<output_patch_t> newOrder)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & order{ mData.speakerSetup.order };
    jassert(newOrder.size() == order.size());
    order = std::move(newOrder);
    refreshSpeakerVuMeterComponents();
}

//==============================================================================
output_patch_t MainContentComponent::getMaxSpeakerOutputPatch() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(!mData.speakerSetup.order.isEmpty());
    juce::ScopedReadLock const lock{ mLock };

    auto const & patches{ mData.speakerSetup.order };
    auto const maxPatch{ *std::max_element(patches.begin(), patches.end()) };
    return maxPatch;
}

//==============================================================================
output_patch_t MainContentComponent::addSpeaker()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto const newOutputPatch{ ++getMaxSpeakerOutputPatch() };
    mData.speakerSetup.speakers.add(newOutputPatch, std::make_unique<SpeakerData>());
    mData.speakerSetup.order.add(newOutputPatch);

    refreshSpeakerVuMeterComponents();

    return newOutputPatch;
}

//==============================================================================
output_patch_t MainContentComponent::insertSpeaker(int const position)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto const newPosition{ position };
    auto const newOutputPatch{ addSpeaker() };
    auto & order{ mData.speakerSetup.order };
    [[maybe_unused]] auto const lastAppenedOutputPatch{ mData.speakerSetup.order.getLast() };
    jassert(newOutputPatch == lastAppenedOutputPatch);
    order.removeLast();
    order.insert(newPosition, newOutputPatch);

    refreshSpeakerVuMeterComponents(); // TODO : this will be done twice sometimes

    return newOutputPatch;
}

//==============================================================================
void MainContentComponent::removeSpeaker(output_patch_t const outputPatch)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.speakerSetup.order.removeFirstMatchingValue(outputPatch);
    mData.speakerSetup.speakers.remove(outputPatch);

    removeInvalidDirectOuts();
    updateViewportConfig();
    updateAudioProcessor();
}

//==============================================================================
bool MainContentComponent::isRadiusNormalized() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    return mData.appData.spatMode == SpatMode::vbap || mData.appData.spatMode == SpatMode::hrtfVbap;
}

//==============================================================================
void MainContentComponent::handleNumSourcesChanged(int const numSources)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(numSources >= 1 && numSources <= MAX_NUM_SOURCES);
    juce::ScopedWriteLock const lock{ mLock };

    mNumSourcesTextEditor->setText(juce::String{ numSources }, false);

    if (numSources > mData.project.sources.size()) {
        source_index_t const firstNewIndex{ mData.project.sources.size() + 1 };
        source_index_t const lastNewIndex{ numSources };
        for (auto index{ firstNewIndex }; index <= lastNewIndex; ++index) {
            mData.project.sources.add(index, std::make_unique<SourceData>());
        }
    } else if (numSources < mData.project.sources.size()) {
        // remove some inputs
        while (mData.project.sources.size() > numSources) {
            source_index_t const index{ mData.project.sources.size() };
            mData.project.sources.remove(index);
        }
    }

    updateAudioProcessor();
    refreshSourceVuMeterComponents();
    unfocusAllComponents();
}

//==============================================================================
bool MainContentComponent::refreshSpeakers()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    removeInvalidDirectOuts();

    auto const & speakers{ mData.speakerSetup.speakers };
    auto const numActiveSpeakers{ std::count_if(
        speakers.cbegin(),
        speakers.cend(),
        [](SpeakersData::ConstNode const speaker) { return !speaker.value->isDirectOutOnly; }) };

    // Ensure there is enough speakers
    auto const showNotEnoughSpeakersError = [&]() {
        juce::AlertWindow alert("Not enough speakers !    ",
                                "Do you want to reload the default setup ?    ",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("No", 0);
        alert.addButton("Yes", 1, juce::KeyPress(juce::KeyPress::returnKey));
        if (alert.runModalLoop() != 0) {
            loadSpeakerSetup(DEFAULT_SPEAKER_SETUP_FILE);
        }
    };

    if (numActiveSpeakers < 2) {
        showNotEnoughSpeakersError();
        return false;
    }

    auto const getVbapDimensions = [&]() {
        auto const firstSpeaker{ *mData.speakerSetup.speakers.begin() };
        auto const firstZenith{ firstSpeaker.value->vector.elevation };
        auto const minZenith{ firstZenith - degrees_t{ 4.9f } };
        auto const maxZenith{ firstZenith + degrees_t{ 4.9f } };

        auto const areSpeakersOnSamePlane{ std::all_of(mData.speakerSetup.speakers.cbegin(),
                                                       mData.speakerSetup.speakers.cend(),
                                                       [&](SpeakersData::ConstNode const node) {
                                                           auto const zenith{ node.value->vector.elevation };
                                                           return zenith < maxZenith && zenith > minZenith;
                                                       }) };
        return areSpeakersOnSamePlane ? VbapType::twoD : VbapType::threeD;
    };

    auto const lbapDimensions{ getVbapDimensions() };
    if (lbapDimensions == VbapType::twoD) {
        mData.appData.viewSettings.showSpeakerTriplets = false;
    } else if (mData.speakerSetup.speakers.size() < 3) {
        showNotEnoughSpeakersError();
        return false;
    }

    // Test for duplicated output patch.
    auto const testDuplicatedOutputPatch = [&]() {
        jassert(mData.speakerSetup.order.size() == speakers.size());
        auto outputPatches{ mData.speakerSetup.order };
        std::sort(outputPatches.begin(), outputPatches.end());
        auto const duplicate{ std::adjacent_find(outputPatches.begin(), outputPatches.end()) };
        return duplicate != outputPatches.end();
    };

    if (testDuplicatedOutputPatch()) {
        juce::AlertWindow alert{ "Duplicated Output Numbers!    ",
                                 "Some output numbers are used more than once. Do you want to continue anyway?    "
                                 "\nIf you continue, you may have to fix your speaker setup before using it!   ",
                                 juce::AlertWindow::WarningIcon };
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Load default setup", 0);
        alert.addButton("Keep current setup", 1);
        if (alert.runModalLoop() == 0) {
            loadSpeakerSetup(DEFAULT_SPEAKER_SETUP_FILE);
        }
        return false;
    }

    mSpatAlgorithm = AbstractSpatAlgorithm::make(mData.appData.spatMode, mData.speakerSetup.speakers);
    updateAudioProcessor();

    refreshSpeakerVuMeterComponents();
    updateViewportConfig();
    if (mEditSpeakersWindow != nullptr) {
        mEditSpeakersWindow->updateWinContent();
    }

    return true;
}

//==============================================================================
void MainContentComponent::reloadXmlFileSpeaker()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    loadSpeakerSetup(mData.appData.lastSpeakerSetup, tl::nullopt);
}

//==============================================================================
void MainContentComponent::loadSpeakerSetup(juce::File const & file, tl::optional<SpatMode> const forceSpatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (!file.existsAsFile()) {
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                          "Error in Load Speaker Setup !",
                                          "Cannot find file " + file.getFullPathName() + ", loading default setup.");
        loadSpeakerSetup(DEFAULT_SPEAKER_SETUP_FILE);
        return;
    }

    juce::XmlDocument xmlDoc{ file };
    auto const mainXmlElem(xmlDoc.getDocumentElement());
    if (!mainXmlElem) {
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                          "Error in Load Speaker Setup !",
                                          "Your file is corrupted !\n" + xmlDoc.getLastParseError());
        loadSpeakerSetup(DEFAULT_SPEAKER_SETUP_FILE);
        return;
    }

    auto speakerSetup{ SpeakerSetup::fromXml(*mainXmlElem) };

    if (!speakerSetup) {
        auto const msg{ mainXmlElem->hasTagName("ServerGRIS_Preset")
                                || mainXmlElem->hasTagName(SpatGrisProjectData::XmlTags::MAIN_TAG)
                            ? "You are trying to open a Server document, and not a Speaker Setup !"
                            : "Your file is corrupted !\n" + xmlDoc.getLastParseError() };
        juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon, "Error in Load Speaker Setup !", msg);
        loadSpeakerSetup(DEFAULT_SPEAKER_SETUP_FILE);
        return;
    }

    juce::ScopedWriteLock const lock{ mLock };
    mData.speakerSetup = std::move(speakerSetup->first);
    mData.appData.spatMode = forceSpatMode.value_or(speakerSetup->second);
    mSpatModeComponent.setSpatMode(mData.appData.spatMode);

    switch (mData.appData.spatMode) {
    case SpatMode::lbap:
    case SpatMode::vbap:
        mData.appData.lastSpeakerSetup = file.getFullPathName();
        break;
    case SpatMode::hrtfVbap:
    case SpatMode::stereo:
        break;
    }

    refreshSpeakers();
}

//==============================================================================
void MainContentComponent::setTitle() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    juce::File const currentProject{ mData.appData.lastProject };
    auto const title{ juce::String{ "SpatGRIS v" } + juce::JUCEApplication::getInstance()->getApplicationVersion()
                      + " - " + currentProject.getFileName() };
    mMainWindow.DocumentWindow::setName(title);
}

//==============================================================================
void MainContentComponent::handleTimer(bool const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (state) {
        startTimerHz(24);
    } else {
        stopTimer();
    }
}

//==============================================================================
void MainContentComponent::saveProject(juce::String const & path)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    juce::File const xmlFile{ path };
    auto const xml{ mData.project.toXml() };
    [[maybe_unused]] auto success{ xml->writeTo(xmlFile) };
    jassert(success);
    success = xmlFile.create();
    jassert(success);
    mData.appData.lastProject = path;

    setTitle();
}

//==============================================================================
void MainContentComponent::saveSpeakerSetup(juce::String const & path)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    juce::File const xmlFile{ path };
    auto const xml{ mData.speakerSetup.toXml(mData.appData.spatMode) };

    [[maybe_unused]] auto success{ xml->writeTo(xmlFile) };
    jassert(success);
    success = xmlFile.create();
    jassert(success);

    mData.appData.lastSpeakerSetup = path;
    // TODO : some gui to update?
}

//==============================================================================
void MainContentComponent::timerCallback()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    // Update levels
    updatePeaks();

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

    if (cpuLoad >= 100) {
        mCpuUsageValue->setColour(juce::Label::backgroundColourId, juce::Colours::darkred);
    } else {
        mCpuUsageValue->setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
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
    JUCE_ASSERT_MESSAGE_THREAD;

    g.fillAll(mLookAndFeel.getWinBackgroundColour());
}

//==============================================================================
void MainContentComponent::textEditorFocusLost(juce::TextEditor & textEditor)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    textEditorReturnKeyPressed(textEditor);
}

//==============================================================================
void MainContentComponent::textEditorReturnKeyPressed([[maybe_unused]] juce::TextEditor & textEditor)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    if (&textEditor != mNumSourcesTextEditor.get()) {
        return;
    }

    auto const unclippedValue{ mNumSourcesTextEditor->getTextValue().toString().getIntValue() };
    auto const numOfInputs{ std::clamp(unclippedValue, 1, MAX_NUM_SOURCES) };
    handleNumSourcesChanged(numOfInputs);
}

//==============================================================================
void MainContentComponent::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

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
    JUCE_ASSERT_MESSAGE_THREAD;

    if (slider == mMasterGainOutSlider.get()) {
        dbfs_t const value{ static_cast<float>(mMasterGainOutSlider->getValue()) };
        handleMasterGainChanged(value);
    } else if (slider == mInterpolationSlider.get()) {
        auto const value{ static_cast<float>(mInterpolationSlider->getValue()) };
        handleGainInterpolationChanged(value);
    }
}

//==============================================================================
void MainContentComponent::comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };
}

//==============================================================================
juce::StringArray MainContentComponent::getMenuBarNames()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    char const * names[] = { "File", "View", "Help", nullptr };
    return juce::StringArray{ names };
}

//==============================================================================
bool MainContentComponent::initRecording()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    juce::File dir{ mData.appData.lastRecordingDirectory };
    if (!(dir.exists() && !dir.existsAsFile())) {
        dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
    }
    juce::String extF;
    juce::String extChoice;

    auto const recordingFormat{ mData.appData.recordingOptions.format };

    if (recordingFormat == RecordingFormat::wav) {
        extF = ".wav";
        extChoice = "*.wav,*.aif";
    } else {
        extF = ".aif";
        extChoice = "*.aif,*.wav";
    }

    auto const initialFile{ dir.getFullPathName() + "/recording" + extF };

    juce::FileChooser fileChooser{ "Choose a file to save...", initialFile, extChoice, true };

    if (!fileChooser.browseForFileToSave(true)) {
        return false;
    }
    jassert(!fileChooser.getResults().isEmpty());

    auto const fileToRecord{ fileChooser.getResults().getReference(0) };
    mData.appData.lastRecordingDirectory = fileToRecord.getParentDirectory().getFullPathName();

    auto const getSpeakersToRecord = [&]() {
        juce::Array<output_patch_t> result{};
        switch (mData.appData.spatMode) {
        case SpatMode::lbap:
        case SpatMode::vbap:
            result = mData.speakerSetup.order;
            result.sort();
            break;
        case SpatMode::hrtfVbap:
        case SpatMode::stereo:
            result.add(output_patch_t{ 1 });
            result.add(output_patch_t{ 2 });
            break;
        }

        return result;
    };

    auto speakersToRecord{ getSpeakersToRecord() };

    AudioManager::RecordingParameters const recordingParams{ fileToRecord.getFullPathName(),
                                                             mData.appData.recordingOptions,
                                                             mData.appData.audioSettings.sampleRate,
                                                             std::move(speakersToRecord) };
    return AudioManager::getInstance().prepareToRecord(recordingParams);
}

//==============================================================================
void MainContentComponent::resized()
{
    static constexpr auto MENU_BAR_HEIGHT = 20;
    static constexpr auto PADDING = 10;

    JUCE_ASSERT_MESSAGE_THREAD;

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
    mMainLayout->setBounds(newMainUiBoxBounds);
    /*mMainUiComponent.setSize(getWidth() - mSpeakerViewComponent->getWidth() - 6, 610);*/

    juce::Rectangle<int> const newInputsUiBoxBounds{ 0,
                                                     2,
                                                     getWidth() - (mSpeakerViewComponent->getWidth() + PADDING),
                                                     231 };
    mSourcesLayout->setBounds(newInputsUiBoxBounds);
    // mSourcesVuMetersViewport.getViewedComponent()->setSize(mData.project.sources.size() * VU_METER_WIDTH + 4, 200);

    juce::Rectangle<int> const newOutputsUiBoxBounds{ 0,
                                                      233,
                                                      getWidth() - (mSpeakerViewComponent->getWidth() + PADDING),
                                                      210 };
    mSpeakersLayout->setBounds(newOutputsUiBoxBounds);
    /*mSpeakersVuMetersViewport.getViewedComponent()->setSize(mData.speakerSetup.speakers.size() * VU_METER_WIDTH + 4,
                                                            180);*/

    juce::Rectangle<int> const newControlUiBoxBounds{ 0,
                                                      443,
                                                      getWidth() - (mSpeakerViewComponent->getWidth() + PADDING),
                                                      145 };
    mControlUiBox->setBounds(newControlUiBoxBounds);
    mControlUiBox->correctSize(410, 145);
}
