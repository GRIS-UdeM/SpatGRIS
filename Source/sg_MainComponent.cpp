/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_MainComponent.hpp"

#include "sg_AudioManager.hpp"
#include "sg_CommandId.hpp"
#include "sg_ControlPanel.hpp"
#include "sg_FatalError.hpp"
#include "sg_GrisLookAndFeel.hpp"
#include "sg_LegacyLbapPosition.hpp"
#include "sg_MainWindow.hpp"
#include "sg_ScopeGuard.hpp"
#include "sg_TitledComponent.hpp"
#include "sg_constants.hpp"

namespace gris
{
namespace
{
constexpr auto BUTTON_CANCEL = 0;
constexpr auto BUTTON_OK = 1;
constexpr auto BUTTON_DISCARD = 2;

//==============================================================================
float gainToSpeakerAlpha(float const gain)
{
    static constexpr auto MIN_ALPHA{ 0.1f };
    static constexpr auto MAX_ALPHA{ 1.0f };
    static constexpr auto ALPHA_RANGE{ MAX_ALPHA - MIN_ALPHA };
    static constexpr auto ALPHA_CURVE{ 0.7f };

    static constexpr dbfs_t MIN_DB{ -60.0f };
    static constexpr dbfs_t MAX_DB{ 0.0f };
    static constexpr dbfs_t DB_RANGE{ MAX_DB - MIN_DB };

    auto const clippedGain{ std::clamp(dbfs_t::fromGain(gain), MIN_DB, MAX_DB) };
    // Using abs() because of -ffast-math compiler flag (Projucer Relax IEEE Compliance is Enabled)
    // ratio value can be -0.0f, which makes result nan (and cause problems to SpeakerView)
    auto const ratio{ std::abs((clippedGain - MIN_DB) / DB_RANGE) };
    jassert(ratio >= 0.0f && ratio <= 1.0f);
    auto const result{ std::pow(ratio, ALPHA_CURVE) * ALPHA_RANGE + MIN_ALPHA };

    return result;
}

//==============================================================================
float gainToSourceAlpha(source_index_t const sourceIndex, float const gain)
{
    static constexpr auto RAMP = 0.12f;
    static constexpr auto OFF = 0.0f;
    static constexpr auto ON = 0.8f;
    static constexpr dbfs_t MIN_GAIN{ -80.0f };

    // TODO : static variables bad
    static StrongArray<source_index_t, float, MAX_NUM_SOURCES> lastAlphas{};
    auto & lastAlpha{ lastAlphas[sourceIndex] };

    if (dbfs_t::fromGain(gain) > MIN_GAIN) {
        lastAlpha = ON;
        return lastAlpha;
    }

    if (lastAlpha <= OFF) {
        return OFF;
    }

    lastAlpha = std::max(lastAlpha - RAMP, OFF);
    return lastAlpha;
}

} // namespace

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

    //==============================================================================
    auto const showSplashScreen = [&]() {
#if NDEBUG
        if (SPLASH_SCREEN_FILE.exists()) {
            auto splashScreen{ std::make_unique<juce::SplashScreen>("SpatGRIS3",
                                                                    juce::ImageFileFormat::loadFrom(SPLASH_SCREEN_FILE),
                                                                    true) };
            splashScreen->deleteAfterDelay(juce::RelativeTime::seconds(3.5), true);
            splashScreen.release();
        }
#endif
    };

    //==============================================================================
    auto const initAppData = [&]() { mData.appData = mConfiguration.load(); };

    //==============================================================================
    auto const initGui = [&]() {
        // Set look and feel
        juce::LookAndFeel::setDefaultLookAndFeel(&grisLookAndFeel);

        // Create the menu bar.
        mMenuBar.reset(new juce::MenuBarComponent(this));
        addAndMakeVisible(mMenuBar.get());

        // SpeakerViewComponent 3D view
        mSpeakerViewComponent.reset(new SpeakerViewComponent(*this));

        // Box Main
        mMainLayout
            = std::make_unique<LayoutComponent>(LayoutComponent::Orientation::vertical, false, true, grisLookAndFeel);

        // info panel
        mInfoPanel = std::make_unique<InfoPanel>(*this, mLookAndFeel);

        // Control panel
        mControlPanel = std::make_unique<ControlPanel>(*this, mLookAndFeel);
        mControlsSection = std::make_unique<TitledComponent>("Controls", mControlPanel.get(), mLookAndFeel);
        mControlPanel->setSpatMode(mData.project.spatMode);
        mControlPanel->setCubeAttenuationDb(mData.project.mbapDistanceAttenuationData.attenuation);
        mControlPanel->setCubeAttenuationHz(mData.project.mbapDistanceAttenuationData.freq);
        mControlPanel->setStereoMode(mData.appData.stereoMode);
        mControlPanel->setStereoRouting(mData.appData.stereoRouting);

        // Source panel
        mSourcesInnerLayout
            = std::make_unique<LayoutComponent>(LayoutComponent::Orientation::horizontal, true, false, grisLookAndFeel);
        mSourcesOuterLayout = std::make_unique<LayoutComponent>(LayoutComponent::Orientation::horizontal,
                                                                false,
                                                                false,
                                                                grisLookAndFeel);
        mSourcesOuterLayout->addSection(*mSourcesInnerLayout).withRelativeSize(1.0f);
        mSourcesOuterLayout->addSection(mAddRemoveSourcesButton).withChildMinSize();
        mSourcesSection = std::make_unique<TitledComponent>("Sources", mSourcesOuterLayout.get(), grisLookAndFeel);

        // Speaker panel
        mSpeakersLayout
            = std::make_unique<LayoutComponent>(LayoutComponent::Orientation::horizontal, true, false, grisLookAndFeel);
        mSpeakersSection = std::make_unique<TitledComponent>("Speakers", mSpeakersLayout.get(), mLookAndFeel);

        // main sections
        mMainLayout->addSection(mInfoPanel.get()).withChildMinSize().withRightPadding(5);
        mMainLayout->addSection(mSourcesSection.get()).withRelativeSize(1.0f).withRightPadding(5);
        mMainLayout->addSection(mSpeakersSection.get()).withRelativeSize(1.0f).withRightPadding(5);
        mMainLayout->addSection(mControlsSection.get()).withChildMinSize().withBottomPadding(5).withRightPadding(5);

        addAndMakeVisible(mMainLayout.get());

        // Set up the layout and resize bars.
        // Since SpeakerView, there is only one pane in the vertical layout.
        mVerticalLayout.setItemLayout(
            0,
            150.0,
            -1.0,
            -0.565); // right panes must be at least 150 pixels wide, preferably 50% of the total width

        // Default application window size.
        setSize(1285, 610);

        // Restore last vertical divider position and speaker view cam distance.
        // TODO: sashPosition not needed anymore since SpeakerView.
        auto const sashPosition{ mData.appData.sashPosition };
        auto const trueSize{ narrow<int>(std::round(narrow<double>(getWidth() - 3) * std::abs(sashPosition))) };
        mVerticalLayout.setItemPosition(1, trueSize);

        mSpeakerViewComponent->setCameraPosition(mData.appData.cameraPosition);
        handleShowSpeakerViewWindow();
    };

    //==============================================================================
    auto const initProject = [&]() {
        auto const success{ loadProject(mData.appData.lastProject, true) };
        if (!success) {
            if (!loadProject(DEFAULT_PROJECT_FILE, true)) {
                fatalError("Unable to load the default project file.", this);
            }
        }
    };

    //==============================================================================
    auto const initSpeakerSetup = [&]() {
        if (!loadSpeakerSetup(mData.appData.lastSpeakerSetup, LoadSpeakerSetupOption::allowDiscardingUnsavedChanges)) {
            if (!loadSpeakerSetup(DEFAULT_SPEAKER_SETUP_FILE, LoadSpeakerSetupOption::allowDiscardingUnsavedChanges)) {
                fatalError("Unable to load the default speaker setup", this);
            }
        }
    };

    //==============================================================================
    auto const initAudioManager = [&]() {
        auto const & audioSettings{ mData.appData.audioSettings };
        AudioManager::init(audioSettings.deviceType,
                           audioSettings.inputDevice,
                           audioSettings.outputDevice,
                           audioSettings.sampleRate,
                           audioSettings.bufferSize,
                           mData.appData.stereoMode ? tl::make_optional(mData.appData.stereoRouting) : tl::nullopt);
    };

    //==============================================================================
    auto const initAudioProcessor = [&]() {
        mAudioProcessor = std::make_unique<AudioProcessor>();
        juce::ScopedLock const audioLock{ mAudioProcessor->getLock() };
        auto & audioManager{ AudioManager::getInstance() };
        audioManager.registerAudioProcessor(mAudioProcessor.get());
        AudioManager::getInstance().getAudioDeviceManager().addChangeListener(this);
        audioParametersChanged(); // size of buffers not initialized here...
    };

    //==============================================================================
    auto const initCommandManager = [&]() {
        auto & commandManager{ mMainWindow.getApplicationCommandManager() };
        commandManager.registerAllCommandsForTarget(this);
    };

    // Load app config
    showSplashScreen();
    initAppData();
    initGui();
    initProject();
    initSpeakerSetup();
    initAudioManager();
    initAudioProcessor();

    // juce::ScopedLock const audioLock{ mAudioProcessor->getLock() };

    startOsc();
    initCommandManager();

    // End layout and start refresh timer.
    resized();
    startTimerHz(24);

    // init buffers properly
    audioParametersChanged();
}

//==============================================================================
MainContentComponent::~MainContentComponent()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mOscInput.reset();

    {
        juce::ScopedWriteLock const lock{ mLock };

        auto const bounds{ MainWindow::getMainAppWindow()->DocumentWindow::getBounds() };
        mData.appData.windowX = bounds.getX();
        mData.appData.windowY = bounds.getY();
        mData.appData.windowWidth = bounds.getWidth();
        mData.appData.windowHeight = bounds.getHeight();
        mData.appData.sashPosition = mVerticalLayout.getItemCurrentRelativeSize(0);
        mData.appData.cameraPosition = mSpeakerViewComponent->getCameraPosition().getCartesian();

        mConfiguration.save(mData.appData);
    }

    if (isSpeakerViewProcessRunning()) {
        // instead of mSpeakerViewProcess.kill(), we ask SpeakerView to quit itself
        mSpeakerViewComponent->shouldKillSpeakerViewProcess(true);
    }

    mSpeakerViewComponent.reset();
}

//==============================================================================
juce::ApplicationCommandTarget * MainContentComponent::getNextCommandTarget()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return findFirstTargetParentComponent();
}

//==============================================================================
void MainContentComponent::handleNewProject()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const success{ loadProject(DEFAULT_PROJECT_FILE, false) };
    if (!success) {
        if (!DEFAULT_PROJECT_FILE.existsAsFile()) {
            fatalError("Unable to load the default project file.", this);
        }
        return;
    }
}

//==============================================================================
bool MainContentComponent::loadProject(juce::File const & file, bool const discardCurrentProject)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    if (!discardCurrentProject) {
        if (!makeSureProjectIsSavedToDisk()) {
            return false;
        }
    }

    auto const displayError = [&](juce::String const & error) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "Unable to load SpatGRIS project.",
                                               error,
                                               "OK",
                                               this,
                                               nullptr);
    };

    auto const informSpeakerSetupAlgoHasChanged = [&]() {
        if (mData.speakerSetup.spatMode == SpatMode::invalid)
            return;

        auto const newSpatMode{ mData.project.spatMode == SpatMode::mbap ? SpatMode::mbap : SpatMode::vbap };

        if (newSpatMode == mData.speakerSetup.spatMode)
            return;

        if (mData.speakerSetup.spatMode == SpatMode::mbap && !mData.speakerSetup.isDomeLike()
            && newSpatMode == SpatMode::vbap)
            return;

        const juce::String message{ "Your current speaker setup has been changed to " + spatModeToString(newSpatMode)
                                    + "." };
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::InfoIcon,
                                               "Modified Speaker Setup",
                                               message,
                                               "Ok",
                                               nullptr);
    };

    if (!file.existsAsFile()) {
        displayError("File \"" + file.getFullPathName() + "\" does not exist.");
        return false;
    }

    juce::XmlDocument xmlDoc{ file };
    auto const mainXmlElem{ xmlDoc.getDocumentElement() };

    if (!mainXmlElem) {
        displayError("File \"" + file.getFullPathName() + "\" is corrupted.\n" + xmlDoc.getLastParseError());
        return false;
    }

    if (mainXmlElem->hasTagName("SpeakerSetup") || mainXmlElem->hasTagName(SpeakerSetup::XmlTags::MAIN_TAG)) {
        // Wrong file type
        displayError("File \"" + file.getFullPathName() + "\" is a Speaker Setup, not a project.");
        return false;
    }

    auto projectData{ ProjectData::fromXml(*mainXmlElem) };
    if (!projectData) {
        auto const version{ SpatGrisVersion::fromString(
            mainXmlElem->getStringAttribute(ProjectData::XmlTags::VERSION)) };
        if (version.compare(SPAT_GRIS_VERSION) > 0) {
            displayError("This project was created using a newer version of SpatGRIS that is not compatible with this "
                         "one.\nPlease upgrade to the latest version.");
            return false;
        }
        // Missing params
        displayError("File \"" + file.getFullPathName()
                     + "\" is missing one more mandatory parameters.\nYour file might be corrupted.");
        return false;
    }

    mIsLoadingSpeakerSetupOrProjectFile = true;

    auto const currentSpatMode{ mData.project.spatMode };

    mData.project = std::move(*projectData);
    mData.appData.lastProject = file.getFullPathName();

    // for project prior to SG 3.1.8 (hybrid is redirected to vbap)
    if (mData.project.spatMode == SpatMode::invalid) {
        mData.project.spatMode = mData.speakerSetup.spatMode;
    }

    // keep spat mode when loading Player
    if (mPlayerWindow != nullptr && file == DEFAULT_PROJECT_FILE) {
        mData.project.spatMode = currentSpatMode;
    }

    if (!discardCurrentProject) {
        informSpeakerSetupAlgoHasChanged();
    }

    setSpatMode(mData.project.spatMode);

    if (mData.project.spatMode == SpatMode::mbap)
        mData.appData.viewSettings.showSpeakerTriplets = false;

    mControlPanel->setMasterGain(mData.project.masterGain);
    mControlPanel->setInterpolation(mData.project.spatGainsInterpolation);
    mControlPanel->setCubeAttenuationBypass(mData.project.mbapDistanceAttenuationData.attenuationBypassState);
    mControlPanel->setCubeAttenuationDb(mData.project.mbapDistanceAttenuationData.attenuation);
    mControlPanel->setCubeAttenuationHz(mData.project.mbapDistanceAttenuationData.freq);

    refreshAudioProcessor();
    refreshViewportConfig();
    refreshSourceSlices();

    mIsLoadingSpeakerSetupOrProjectFile = false;

    return true;
}

//==============================================================================
void MainContentComponent::handleOpenProject()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    juce::File const & lastProject{ mData.appData.lastProject };
    juce::File const initialPath{ lastProject.isAChildOf(CURRENT_WORKING_DIR) ? juce::File::getSpecialLocation(
                                      juce::File::SpecialLocationType::userDocumentsDirectory)
                                                                              : lastProject };

    juce::FileChooser fc{ "Choose a file to open...", initialPath, "*.xml", true, false, this };

    if (!fc.browseForFileToOpen()) {
        return;
    }

    auto const chosen{ fc.getResult() };

    [[maybe_unused]] auto const success{ loadProject(chosen, false) };
    jassert(success);
}

//==============================================================================
void MainContentComponent::handleSaveProject()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    [[maybe_unused]] auto const success{ saveProject(mData.appData.lastProject) };
}

//==============================================================================
void MainContentComponent::handleSaveProjectAs()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    [[maybe_unused]] auto const success{ saveProject(tl::nullopt) };
}

//==============================================================================
void MainContentComponent::handleOpenSpeakerSetup()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    juce::File const lastSetup{ mData.appData.lastSpeakerSetup };

    auto const initialFile{ lastSetup.isAChildOf(CURRENT_WORKING_DIR) ? juce::File::getSpecialLocation(
                                juce::File::SpecialLocationType::userDocumentsDirectory)
                                                                      : lastSetup };

    juce::FileChooser fc{ "Choose a file to open...", initialFile, "*.xml", true };

    if (!fc.browseForFileToOpen()) {
        return;
    }

    auto const chosen{ fc.getResult() };
    [[maybe_unused]] auto const success{ loadSpeakerSetup(chosen,
                                                          LoadSpeakerSetupOption::disallowDiscardingUnsavedChanges) };
}

//==============================================================================
void MainContentComponent::handleSaveSpeakerSetupAs()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    [[maybe_unused]] auto const success{ saveSpeakerSetup(tl::nullopt) };
}

//==============================================================================
void MainContentComponent::closeSpeakersConfigurationWindow()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto const savedElement{ juce::XmlDocument{ juce::File{ mData.appData.lastSpeakerSetup } }.getDocumentElement() };
    jassert(savedElement);
    if (!savedElement) {
        return;
    }

    auto savedSpeakerSetup{ SpeakerSetup::fromXml(*savedElement) };
    jassert(savedSpeakerSetup);
    if (!savedSpeakerSetup) {
        return;
    }

    auto const speakerSetupKeepChanges = [&]() {
        auto const result{ juce::AlertWindow::showYesNoCancelBox(
            juce::MessageBoxIconType::QuestionIcon,
            "Modified Speaker Setup",
            "The Speaker Setup has been modified. Do you want to save your changes ?",
            "Yes",
            "No",
            "Cancel",
            nullptr,
            nullptr) };

        // 0 = Cancel, 1 = Yes, 2 = No
        if (result == 0) {
            return;
        } else if (result == 1) {
            if (!saveSpeakerSetup(mData.appData.lastSpeakerSetup)) {
                return;
            }
        } else if (result == 2) {
            auto const spatMode{ mData.project.spatMode };
            loadSpeakerSetup(mData.appData.lastSpeakerSetup, LoadSpeakerSetupOption::allowDiscardingUnsavedChanges);
            setSpatMode(spatMode);
        }

        refreshSpeakers();
        mEditSpeakersWindow.reset();
    };

    if (!isSpeakerSetupModified()) {
        mEditSpeakersWindow.reset();
        return;
    } else {
        speakerSetupKeepChanges();
    }
}

//==============================================================================
void MainContentComponent::saveAsEditedSpeakerSetup()
{
    saveSpeakerSetup(tl::nullopt);
}

//==============================================================================
void MainContentComponent::saveEditedSpeakerSetup()
{
    handleSaveSpeakerSetup();
}

//==============================================================================
void MainContentComponent::closePlayerWindow()
{
    mPlayerWindow.reset();
    mData.appData.playerExists = false;
    startOsc();
    handleResetSourcesPositions();
    mAddRemoveSourcesButton.setEnabled(true);
    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::handleShowSpeakerEditWindow()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    if (mEditSpeakersWindow == nullptr) {
        auto const windowName = juce::String{ "Speaker Setup Edition - " }
                                + spatModeToString(mData.speakerSetup.spatMode) + " - "
                                + juce::File{ mData.appData.lastSpeakerSetup }.getFileNameWithoutExtension();
        mEditSpeakersWindow
            = std::make_unique<EditSpeakersWindow>(windowName, mLookAndFeel, *this, mData.appData.lastProject);
        mEditSpeakersWindow->initComp();
    }
}

//==============================================================================
void MainContentComponent::handleShowPreferences()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    if (mPropertiesWindow == nullptr) {
        mPropertiesWindow.reset(new SettingsWindow{ *this, mData.project.oscPort, mLookAndFeel });
        mPropertiesWindow->centreAroundComponent(this, mPropertiesWindow->getWidth(), mPropertiesWindow->getHeight());
    }
}

//==============================================================================
void MainContentComponent::handleShowOscMonitorWindow()
{
    if (mOscMonitorWindow == nullptr) {
        mOscMonitorWindow = std::make_unique<OscMonitorWindow>(mLogBuffer, *this, mLookAndFeel);
    } else {
        mOscMonitorWindow->toFront(true);
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
        mFlatViewWindowRect.setBounds(getScreenX() + 22, getScreenY() + 100, 500, 500);
    }

    mFlatViewWindow->setBounds(mFlatViewWindowRect);
    mFlatViewWindow->setResizable(true, true);
    mFlatViewWindow->setUsingNativeTitleBar(true);
    mFlatViewWindow->setVisible(true);
}

//==============================================================================
void MainContentComponent::handleShowPlayerWindow()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mPlayerWindow) {
        mPlayerWindow->toFront(true);
        return;
    }

    juce::ScopedWriteLock const dataLock{ mLock };

    stopOsc();
    handleResetSourcesPositions();
    refreshAudioProcessor();
    mAddRemoveSourcesButton.setEnabled(false);
    mPlayerWindow = std::make_unique<PlayerWindow>(*this, mLookAndFeel);
    mData.appData.playerExists = true;
    refreshAudioProcessor();

    if (!makeSureProjectIsSavedToDisk()) {
        closePlayerWindow();
    }
}

//==============================================================================
void MainContentComponent::handleShowSpeakerViewWindow()
{
    if (isSpeakerViewProcessRunning()) {
        if (!mSpeakerViewComponent->isSpeakerViewNetworkingRunning()) {
            mSpeakerViewComponent->stopSpeakerViewNetworking();
            mSpeakerViewComponent->startSpeakerViewNetworking();
        }
        mSpeakerViewShouldGrabFocus = true;
        return;
    }

    auto const displayError = [&](juce::String const & title, juce::String const & error) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               title,
                                               error,
                                               "OK",
                                               this,
                                               nullptr);
    };

    bool speakerViewFirstLaunch{ mData.appData.speakerViewWindowPosition == juce::Point<int>{} };

    mSpeakerViewComponent->stopSpeakerViewNetworking();
    mSpeakerViewComponent->shouldKillSpeakerViewProcess(false);

    juce::StringArray launchCommand;
    auto const spatGrisDirectory{ juce::File::getSpecialLocation(
        juce::File::SpecialLocationType::currentExecutableFile) };
    auto const spatGrisMacOSAppDirectory{ juce::File::getSpecialLocation(
        juce::File::SpecialLocationType::currentApplicationFile) };

    auto const speakerViewExecWindows{ spatGrisDirectory.getSiblingFile("SpeakerView.exe") };
    auto const speakerViewExecMacOS{ spatGrisMacOSAppDirectory.getSiblingFile("SpeakerView.app") };
    auto const speakerViewExecLinux{ spatGrisDirectory.getSiblingFile("SpeakerView.x86_64") };
    auto const speakerViewPckWindowsAndLinux{ spatGrisDirectory.getSiblingFile("SpeakerView.pck") };

    bool speakerViewExecExists{};
    bool speakerViewPckExists{ speakerViewPckWindowsAndLinux.existsAsFile() };

    if (speakerViewExecWindows.existsAsFile()) {
        launchCommand.add(speakerViewExecWindows.getFullPathName());
        speakerViewExecExists = true;
    } else if (speakerViewExecMacOS.exists()) {
        launchCommand.add("/usr/bin/open");
        launchCommand.add(speakerViewExecMacOS.getFullPathName());
        launchCommand.add("--args");
        speakerViewExecExists = true;
        speakerViewPckExists = true;
    } else if (speakerViewExecLinux.existsAsFile()) {
        // TODO: test if SpeakerView is in the PATH
        launchCommand.add(speakerViewExecLinux.getFullPathName());
        speakerViewExecExists = true;
    }

    if (!speakerViewExecExists) {
        const juce::String title{ "Unable to find SpeakerView executable." };
        const juce::String message{ "Make sure the SpeakerView executable is in the same folder as SpatGris." };
        displayError(title, message);
        return;
    }

    if (!speakerViewPckExists) {
        const juce::String title{ "Unable to find SpeakerView pck file." };
        const juce::String message{ "Make sure the SpeakerView.pck file is in the same folder as SpatGris." };
        displayError(title, message);
        return;
    }

    launchCommand.add("--");
    launchCommand.add("launchedBySG=true");

    if (speakerViewFirstLaunch) {
        launchCommand.add(juce::String("firstLaunchBySG=true"));
    } else {
        launchCommand.add(juce::String("firstLaunchBySG=false"));
    }

    // SpeakerView window position
    if (mData.appData.speakerViewWindowPosition != juce::Point<int>{}) {
        auto xPos{ juce::String(mData.appData.speakerViewWindowPosition.getX()) };
        auto yPos{ juce::String(mData.appData.speakerViewWindowPosition.getY()) };
        launchCommand.add(juce::String("winPosition=" + xPos + "," + yPos));
    }

    // SpeakerView window size
    if (mData.appData.speakerViewWindowSize != juce::Point<int>{}) {
        auto xSize{ juce::String(mData.appData.speakerViewWindowSize.getX()) };
        auto ySize{ juce::String(mData.appData.speakerViewWindowSize.getY()) };
        launchCommand.add(juce::String("winSize=" + xSize + "," + ySize));
    }

    // SpeakerView camera position
    auto const & camPos{ mSpeakerViewComponent->getCameraPosition().getPolar() };
    auto azi = -camPos.azimuth; // azimuth is inverted in SpeakerView
    auto aziDeg = juce::radiansToDegrees(azi.get());
    auto elev = camPos.elevation;
    auto elevDeg = juce::radiansToDegrees(elev.get());
    elevDeg = std::clamp(elevDeg, -89.0f, 89.0f);
    auto len = camPos.length * 10.0f;
    len = std::clamp(len, 6.0f, 70.0f);

    juce::String cmd;
    if (std::isnan(azi.get()) || std::isnan(elev.get()) || std::isnan(len)) {
        cmd = juce::String("camPos=45.0,45.0,20.0");
    } else {
        cmd = juce::String("camPos=" + juce::String(aziDeg) + "," + juce::String(elevDeg) + "," + juce::String(len));
    }

    launchCommand.add(cmd);

    auto res{ mSpeakerViewProcess.start(launchCommand) };
    jassert(res);

    // Start SpeakerView networking
    if (res) {
        mSpeakerViewComponent->startSpeakerViewNetworking();
    }
}

//==============================================================================
void MainContentComponent::handleKeepSpeakerViewWindowOnTop()
{
    mData.appData.viewSettings.keepSpeakerViewWindowOnTop = !mData.appData.viewSettings.keepSpeakerViewWindowOnTop;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowHall()
{
    mData.appData.viewSettings.showHall = !mData.appData.viewSettings.showHall;
    refreshViewportConfig();
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
void MainContentComponent::handleOpenManualEN()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (MANUAL_FILE_EN.exists()) {
        juce::Process::openDocument("file:" + MANUAL_FILE_EN.getFullPathName(), juce::String());
    }
}

//==============================================================================
void MainContentComponent::handleOpenManualFR()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (MANUAL_FILE_FR.exists()) {
        juce::Process::openDocument("file:" + MANUAL_FILE_FR.getFullPathName(), juce::String());
    }
}
//==============================================================================
void MainContentComponent::handleShowSpeakerOrientation()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    {
        juce::ScopedWriteLock const lock{ mLock };

        auto & var{ mData.appData.viewSettings.keepSpeakersOriginOriented };
        var = !var;
    }
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::masterGainChanged(dbfs_t const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.masterGain = gain;
    mControlPanel->setMasterGain(gain);

    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::interpolationChanged(float const interpolation)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.spatGainsInterpolation = interpolation;
    mControlPanel->setInterpolation(interpolation);

    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::setSpatMode(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto const ensureVbapIsDomeLike = [&]() {
        if (spatMode != SpatMode::mbap && !mData.speakerSetup.isDomeLike()) {
            auto const result{ juce::AlertWindow::showOkCancelBox(
                juce::AlertWindow::InfoIcon,
                "Converting to DOME",
                "A CUBE speaker setup will be converted to a DOME structure.\nThis "
                "will not affect the original file.",
                "Ok",
                "Cancel",
                this) };
            if (!result) {
                return false;
            }

            for (auto & node : mData.speakerSetup.speakers) {
                auto & speaker{ *node.value };
                if (speaker.isDirectOutOnly) {
                    continue;
                }
                speaker.position = speaker.position.normalized();
            }
        }
        return true;
    };

    auto newSpatMode{ spatMode };

    if (!ensureVbapIsDomeLike())
        newSpatMode = SpatMode::mbap;

    // Speaker setup must be Dome or Cube, never Hybrid
    mData.speakerSetup.spatMode = newSpatMode == SpatMode::mbap ? SpatMode::mbap : SpatMode::vbap;
    mData.project.spatMode = newSpatMode;
    mControlPanel->setSpatMode(newSpatMode);
    setTitles();
    refreshSpeakers();
}

//==============================================================================
void MainContentComponent::setStereoMode(tl::optional<StereoMode> const stereoMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.appData.viewSettings.showSpeakerTriplets = false;
    mData.appData.stereoMode = stereoMode;

    AudioManager::getInstance().setStereoRouting(stereoMode ? tl::make_optional(mData.appData.stereoRouting)
                                                            : tl::nullopt);

    refreshSpatAlgorithm();
    refreshSpeakerSlices();
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::setStereoRouting(StereoRouting const & routing)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.appData.stereoRouting = routing;
    AudioManager::getInstance().setStereoRouting(mData.appData.stereoMode ? tl::make_optional(routing) : tl::nullopt);
}

//==============================================================================
void MainContentComponent::cubeAttenuationDbChanged(dbfs_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.mbapDistanceAttenuationData.attenuation = value;
    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::cubeAttenuationHzChanged(hz_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.mbapDistanceAttenuationData.freq = value;
    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::cubeAttenuationBypassState(AttenuationBypassSate state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.mbapDistanceAttenuationData.attenuationBypassState = state;
    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::numSourcesChanged(int const numSources)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(numSources >= 1 && numSources <= MAX_NUM_SOURCES);
    juce::ScopedWriteLock const lock{ mLock };

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

    refreshAudioProcessor();
    audioParametersChanged(); // Make sure the size of the buffers does not reset to MAX_NUM_SAMPLES
    refreshSourceSlices();
    unfocusAllComponents();
}

//==============================================================================
void MainContentComponent::generalMuteButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto const newSliceState{ mControlPanel->getGeneralMuteButtonState() == GeneralMuteButton::State::allUnmuted
                             ? SliceState::muted
                             : SliceState::normal };
    auto const newGeneralMuteButtonState{ mControlPanel->getGeneralMuteButtonState()
                                                  == GeneralMuteButton::State::allUnmuted
                                              ? GeneralMuteButton::State::allMuted
                                              : GeneralMuteButton::State::allUnmuted };
    auto const generalMute{ newGeneralMuteButtonState == GeneralMuteButton::State::allMuted };

    mControlPanel->setGeneralMuteButtonState(newGeneralMuteButtonState);
    for (auto const outputPatch : mData.speakerSetup.ordering) {
        mData.speakerSetup.speakers[outputPatch].state = newSliceState;
    }
    mData.speakerSetup.generalMute = generalMute;

    refreshAudioProcessor();
    refreshSpeakerSlices();
}

//==============================================================================
void MainContentComponent::recordButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    if (AudioManager::getInstance().isRecording()) {
        AudioManager::getInstance().stopRecording();
        mControlPanel->setRecordButtonState(RecordButton::State::ready);
        return;
    }

    if (mPrepareToRecordWindow) {
        return;
    }

    mPrepareToRecordWindow = std::make_unique<PrepareToRecordWindow>(mData.appData.lastRecordingDirectory,
                                                                     mData.appData.recordingOptions,
                                                                     *this,
                                                                     mLookAndFeel);
}

//==============================================================================
juce::Component * MainContentComponent::getControlsComponent() const
{
    return mMainLayout.get();
}

//==============================================================================
void MainContentComponent::handleShowSourceNumbers()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSourceNumbers };
    var = !var;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSpeakerNumbers()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    {
        juce::ScopedWriteLock const lock{ mLock };

        auto & var{ mData.appData.viewSettings.showSpeakerNumbers };
        var = !var;
    }
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSpeakers()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    {
        juce::ScopedWriteLock const lock{ mLock };

        auto & var{ mData.appData.viewSettings.showSpeakers };
        var = !var;
    }
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowTriplets()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const readLock{ mLock };

    auto const newState{ !mData.appData.viewSettings.showSpeakerTriplets };
    if (newState && !mAudioProcessor->getSpatAlgorithm()->hasTriplets()) {
        jassertfalse;
        return;
    }

    juce::ScopedWriteLock const writeLock{ mLock };
    mData.appData.viewSettings.showSpeakerTriplets = newState;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSourceLevel()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSourceActivity };
    var = !var;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSpeakerLevel()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSpeakerLevels };
    var = !var;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSphere()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSphereOrCube };
    var = !var;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleResetSourcesPositions()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::ScopedWriteLock const lock{ mLock };

    auto & viewPortData{ mSpeakerViewComponent->getData() };
    auto & spatAlgorithm{ *mAudioProcessor->getSpatAlgorithm() };
    for (auto const source : mData.project.sources) {
        // reset positions
        source.value->position = tl::nullopt;

        {
            // reset 3d view
            auto & exchanger{ viewPortData.hotSourcesDataUpdaters[source.key] };
            auto * sourceTicket{ exchanger.acquire() };
            sourceTicket->get() = tl::nullopt;
            exchanger.setMostRecent(sourceTicket);
        }

        if (mFlatViewWindow) {
            // reset 2d view
            auto & exchanger{ mFlatViewWindow->getSourceDataQueues()[source.key] };
            auto * sourceTicket{ exchanger.acquire() };
            sourceTicket->get() = tl::nullopt;
            exchanger.setMostRecent(sourceTicket);
        }

        // spatData
        spatAlgorithm.updateSpatData(source.key, *source.value);
    }

    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::handleResetMeterClipping()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    for (auto const & slice : mSourceSliceComponents) {
        slice.value->resetClipping();
    }
    for (auto const & slice : mSpeakerSliceComponents) {
        slice.value->resetClipping();
    }
    for (auto const & slice : mStereoSliceComponents) {
        slice->resetClipping();
    }
}

//==============================================================================
void MainContentComponent::handleColorizeInputs()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    float hue{};
    auto const inc{ 1.0f / static_cast<float>(mData.project.sources.size() + 1) };
    for (auto const & source : mData.project.sources) {
        auto const colour{ juce::Colour::fromHSV(hue, 1, 0.75, 1) };
        source.value->colour = colour;
        mSourceSliceComponents[source.key].setSourceColour(colour);
        hue += inc;
    }
}

//==============================================================================
// Command manager methods.
void MainContentComponent::getAllCommands(juce::Array<juce::CommandID> & commands)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    constexpr std::array<CommandId, 31> ids{ CommandId::newProjectId,
                                             CommandId::openProjectId,
                                             CommandId::saveProjectId,
                                             CommandId::saveProjectAsId,
                                             CommandId::openSpeakerSetupId,
                                             CommandId::saveSpeakerSetupId,
                                             CommandId::saveSpeakerSetupAsId,
                                             CommandId::showSpeakerEditId,
                                             CommandId::show2DViewId,
                                             CommandId::showPlayerWindowId,
                                             CommandId::showOscMonitorId,
                                             CommandId::showSpeakerViewId,
                                             CommandId::keepSpeakerViewOnTopId,
                                             CommandId::showHallId,
                                             CommandId::showSourceNumbersId,
                                             CommandId::showSpeakerNumbersId,
                                             CommandId::showSpeakersId,
                                             CommandId::showTripletsId,
                                             CommandId::showSourceActivityId,
                                             CommandId::showSpeakerActivityId,
                                             CommandId::showSphereId,
                                             CommandId::colorizeInputsId,
                                             CommandId::resetInputPosId,
                                             CommandId::resetMeterClipping,
                                             CommandId::muteAllSpeakers,
                                             CommandId::openSettingsWindowId,
                                             CommandId::quitId,
                                             CommandId::aboutId,
                                             CommandId::openManualENId,
                                             CommandId::openManualFRId,
                                             CommandId::playerPlayStopId,
                                             CommandId::keepSpeakersOriginOrientatedId};

    commands.addArray(ids.data(), narrow<int>(ids.size()));

    auto const addTemplate = [&](auto const & templates) {
        for (auto const & speakerTemplate : templates) {
            commands.add(speakerTemplate.commandId);
        }
    };

    auto const addProjectTemplate = [&](auto const & templates) {
        for (auto const & projectTemplate : templates) {
            commands.add(projectTemplate.commandId);
        }
    };

    addTemplate(SPEAKER_SETUP_TEMPLATES.dome);
    addTemplate(SPEAKER_SETUP_TEMPLATES.cube);
    addProjectTemplate(PROJECT_TEMPLATES.dome);
    addProjectTemplate(PROJECT_TEMPLATES.cube);
    addProjectTemplate(PROJECT_TEMPLATES.hybrid);
}

//==============================================================================
void MainContentComponent::getCommandInfo(juce::CommandID const commandId, juce::ApplicationCommandInfo & result)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    const juce::String generalCategory("General");

    switch (commandId) {
    case CommandId::newProjectId:
        result.setInfo("New Project", "Close the current project and open the default.", generalCategory, 0);
        result.addDefaultKeypress('N', juce::ModifierKeys::commandModifier);
        return;
    case CommandId::openProjectId:
        result.setInfo("Open Project", "Choose a new project on disk.", generalCategory, 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::commandModifier);
        return;
    case CommandId::saveProjectId:
        result.setInfo("Save Project", "Save the current project on disk.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::commandModifier);
        result.setActive(isProjectModified());
        return;
    case CommandId::saveProjectAsId:
        result.setInfo("Save Project As...", "Save the current project under a new name on disk.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::shiftModifier | juce::ModifierKeys::commandModifier);
        return;
    case CommandId::openSpeakerSetupId:
        result.setInfo("Open Speaker Setup", "Choose a new speaker setup on disk.", generalCategory, 0);
        result.addDefaultKeypress('L', juce::ModifierKeys::commandModifier);
        return;
    case CommandId::saveSpeakerSetupId:
        result.setInfo("Save Speaker Setup", "Save the current speaker setup on disk.", generalCategory, 0);
        result.addDefaultKeypress('E', juce::ModifierKeys::commandModifier);
        result.setActive(isSpeakerSetupModified());
        return;
    case CommandId::saveSpeakerSetupAsId:
        result.setInfo("Save Speaker Setup As...",
                       "Save the current speaker setup under a new name on disk.",
                       generalCategory,
                       0);
        result.addDefaultKeypress('E', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
        return;
    case CommandId::showSpeakerEditId:
        result.setInfo("Speaker Setup Edition", "Edit the current speaker setup.", generalCategory, 0);
        result.addDefaultKeypress('W', juce::ModifierKeys::altModifier);
        return;
    case CommandId::show2DViewId:
        result.setInfo("Show 2D View", "Show the 2D action window.", generalCategory, 0);
        result.addDefaultKeypress('D', juce::ModifierKeys::altModifier);
        return;
    case CommandId::showPlayerWindowId:
        result.setInfo("Show Player View", "Show the player window.", generalCategory, 0);
        result.addDefaultKeypress('P', juce::ModifierKeys::altModifier);
        return;
    case CommandId::showOscMonitorId:
        result.setInfo("Show OSC monitor", "Show the OSC monitor window", generalCategory, 0);
        return;
    case CommandId::showSpeakerViewId:
        result.setInfo("Show Speaker View", "Show the speaker window.", generalCategory, 0);
        result.addDefaultKeypress('V', juce::ModifierKeys::altModifier);
        return;
    case CommandId::keepSpeakerViewOnTopId:
        result.setInfo("Keep Speaker View On Top",
                       "Keep the speaker window on top of other windows when SpatGris has focus.",
                       generalCategory,
                       0);
        result.addDefaultKeypress('V', juce::ModifierKeys::altModifier | juce::ModifierKeys::shiftModifier);
        result.setTicked(mData.appData.viewSettings.keepSpeakerViewWindowOnTop);
        return;
    case CommandId::showHallId:
        result.setInfo("Show Hall", "Show the hall in the speaker view.", generalCategory, 0);
        result.addDefaultKeypress('H', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showHall);
        return;
    case CommandId::showSourceNumbersId:
        result.setInfo("Show Source Numbers", "Show source numbers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('N', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSourceNumbers);
        return;
    case CommandId::showSpeakerNumbersId:
        result.setInfo("Show Speaker Numbers", "Show speaker numbers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('Z', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSpeakerNumbers);
        return;
    case CommandId::showSpeakersId:
        result.setInfo("Show Speakers", "Show speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSpeakers);
        return;
    case CommandId::showTripletsId:
        result.setInfo("Show Speaker Triplets", "Show speaker triplets on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('T', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSpeakerTriplets);
        result.setActive(mAudioProcessor->getSpatAlgorithm()->hasTriplets());
        return;
    case CommandId::showSourceActivityId:
        result.setInfo("Show Source Activity", "Activate brightness on sources on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('A', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSourceActivity);
        return;
    case CommandId::showSpeakerActivityId:
        result.setInfo("Show Speaker Level", "Activate brightness on speakers on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('L', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSpeakerLevels);
        return;
    case CommandId::showSphereId:
        result.setInfo("Show Sphere/Cube", "Show the sphere on the 3D view.", generalCategory, 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::altModifier);
        result.setTicked(mData.appData.viewSettings.showSphereOrCube);
        return;
    case CommandId::colorizeInputsId:
        result.setInfo("Colorize Sources",
                       "Spread the colour of the sources over the colour range.",
                       generalCategory,
                       0);
        result.addDefaultKeypress('C', juce::ModifierKeys::altModifier);
        return;
    case CommandId::resetInputPosId:
        result.setInfo("Reset Sources Positions", "Reset the positions of the input sources.", generalCategory, 0);
        result.addDefaultKeypress('R', juce::ModifierKeys::altModifier);
        return;
    case CommandId::resetMeterClipping:
        result.setInfo("Reset Meter Clipping", "Reset clipping for all meters.", generalCategory, 0);
        result.addDefaultKeypress('M', juce::ModifierKeys::altModifier);
        return;
    case CommandId::muteAllSpeakers:
        result.setInfo("Mute/Unmute All Speakers", "Mute or unmute all speakers.", generalCategory, 0);
        result.addDefaultKeypress('Q', juce::ModifierKeys::altModifier);
        return;
    case CommandId::openSettingsWindowId:
        result.setInfo("Settings...", "Open the settings window.", generalCategory, 0);
        result.addDefaultKeypress(',', juce::ModifierKeys::commandModifier);
        return;
    case CommandId::quitId:
        result.setInfo("Quit", "Quit the SpatGRIS.", generalCategory, 0);
        result.addDefaultKeypress('Q', juce::ModifierKeys::commandModifier);
        return;
    case CommandId::aboutId:
        result.setInfo("About SpatGRIS", "Open the about window.", generalCategory, 0);
        return;
    case CommandId::openManualENId:
        result.setInfo("Open Documentation (EN)", "Open the manual in pdf viewer.", generalCategory, 0);
        return;
    case CommandId::openManualFRId:
        result.setInfo("Ouvrir Documentation (FR)", "Ouvrir le manuel dans le lecteur pdf.", generalCategory, 0);
        return;
    case CommandId::playerPlayStopId:
        result.setInfo("Play Stop", "Play or Stop Player Playback", generalCategory, 0);
        result.addDefaultKeypress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers);
        return;
    case CommandId::keepSpeakersOriginOrientatedId:
        result.setInfo("Keep Speakers Orientated Towards Origin", "Toggle the Speakers Orientation", generalCategory, 0);
        result.setTicked(mData.appData.viewSettings.keepSpeakersOriginOriented);
        return;
    }

    // probably a template
    auto const templateInfo{ commandIdToTemplate(commandId) };
    jassert(templateInfo);
    auto const longName{ juce::String{ "Open template " } + templateInfo->name };
    result.setInfo(longName, templateInfo->name, generalCategory, 0);
}

//==============================================================================
bool MainContentComponent::perform(InvocationInfo const & info)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (MainWindow::getMainAppWindow()) {
        switch (info.commandID) {
        case CommandId::newProjectId:
            handleNewProject();
            break;
        case CommandId::openProjectId:
            handleOpenProject();
            break;
        case CommandId::saveProjectId:
            handleSaveProject();
            break;
        case CommandId::saveProjectAsId:
            handleSaveProjectAs();
            break;
        case CommandId::openSpeakerSetupId:
            handleOpenSpeakerSetup();
            break;
        case CommandId::saveSpeakerSetupId:
            handleSaveSpeakerSetup();
            break;
        case CommandId::saveSpeakerSetupAsId:
            handleSaveSpeakerSetupAs();
            break;
        case CommandId::showSpeakerEditId:
            handleShowSpeakerEditWindow();
            break;
        case CommandId::show2DViewId:
            handleShow2DView();
            break;
        case CommandId::showPlayerWindowId:
            handleShowPlayerWindow();
            break;
        case CommandId::showOscMonitorId:
            handleShowOscMonitorWindow();
            break;
        case CommandId::showSpeakerViewId:
            handleShowSpeakerViewWindow();
            break;
        case CommandId::keepSpeakerViewOnTopId:
            handleKeepSpeakerViewWindowOnTop();
            break;
        case CommandId::showHallId:
            handleShowHall();
            break;
        case CommandId::showSourceNumbersId:
            handleShowSourceNumbers();
            break;
        case CommandId::showSpeakerNumbersId:
            handleShowSpeakerNumbers();
            break;
        case CommandId::showSpeakersId:
            handleShowSpeakers();
            break;
        case CommandId::showTripletsId:
            handleShowTriplets();
            break;
        case CommandId::showSourceActivityId:
            handleShowSourceLevel();
            break;
        case CommandId::showSpeakerActivityId:
            handleShowSpeakerLevel();
            break;
        case CommandId::showSphereId:
            handleShowSphere();
            break;
        case CommandId::colorizeInputsId:
            handleColorizeInputs();
            break;
        case CommandId::resetInputPosId:
            handleResetSourcesPositions();
            break;
        case CommandId::resetMeterClipping:
            handleResetMeterClipping();
            break;
        case CommandId::muteAllSpeakers:
            generalMuteButtonPressed();
            break;
        case CommandId::openSettingsWindowId:
            handleShowPreferences();
            break;
        case CommandId::quitId:
            dynamic_cast<MainWindow *>(&mMainWindow)->closeButtonPressed();
            break;
        case CommandId::aboutId:
            handleShowAbout();
            break;
        case CommandId::openManualENId:
            handleOpenManualEN();
            break;
        case CommandId::openManualFRId:
            handleOpenManualFR();
            break;
        case CommandId::playerPlayStopId:
            handlePlayerPlayStop();
            break;
        case CommandId::keepSpeakersOriginOrientatedId:
            handleShowSpeakerOrientation();
            break;
        default:
            // open a template
            auto const templateInfo{ commandIdToTemplate(info.commandID) };
            if (!templateInfo) {
                return false;
            }
            if (templateInfo->path.isAChildOf(SPEAKER_TEMPLATES_DIR)) {
                loadSpeakerSetup(templateInfo->path, LoadSpeakerSetupOption::disallowDiscardingUnsavedChanges);
                break;
            }
            jassert(templateInfo->path.isAChildOf(PROJECT_TEMPLATES_DIR));
            loadProject(templateInfo->path, false);
            break;
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

    if (currentAudioDevice) {
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

        mInfoPanel->setSampleRate(sampleRate);
        mInfoPanel->setBufferSize(bufferSize);
        mInfoPanel->setNumInputs(inputCount);
        mInfoPanel->setNumOutputs(outputCount);

        output_patch_t const maxOutputPatch{ outputCount };

        // reset stereo routing if current output channels don't exist
        if (mData.appData.stereoRouting.left.get() > maxOutputPatch.get()
            || mData.appData.stereoRouting.right.get() > maxOutputPatch.get()) {
            StereoRouting const newStereoRouting;
            setStereoRouting(newStereoRouting);
        }

        mControlPanel->updateMaxOutputPatch(maxOutputPatch, mData.appData.stereoRouting);
    }

    refreshSpeakers();
}

//==============================================================================
juce::PopupMenu MainContentComponent::getMenuForIndex(int /*menuIndex*/, const juce::String & menuName)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::ApplicationCommandManager * commandManager = &mMainWindow.getApplicationCommandManager();

    auto const extractTemplatesToMenu = [&](auto const & templates) {
        juce::PopupMenu menu{};
        for (auto const & setupTemplate : templates) {
            menu.addCommandItem(commandManager, setupTemplate.commandId, setupTemplate.name);
        }
        return menu;
    };

    auto const getProjectTemplatesMenu = [&]() {
        juce::PopupMenu menu{};
        menu.addSubMenu("Dome", extractTemplatesToMenu(PROJECT_TEMPLATES.dome));
        menu.addSubMenu("Cube", extractTemplatesToMenu(PROJECT_TEMPLATES.cube));
        menu.addSubMenu("Hybrid", extractTemplatesToMenu(PROJECT_TEMPLATES.hybrid));
        return menu;
    };

    auto const getSpeakerSetupTemplatesMenu = [&]() {
        juce::PopupMenu menu{};
        menu.addSubMenu("Dome", extractTemplatesToMenu(SPEAKER_SETUP_TEMPLATES.dome));
        menu.addSubMenu("Cube", extractTemplatesToMenu(SPEAKER_SETUP_TEMPLATES.cube));
        return menu;
    };

    juce::PopupMenu menu;

    if (menuName == "File") {
        menu.addCommandItem(commandManager, CommandId::newProjectId);
        menu.addCommandItem(commandManager, CommandId::openProjectId);
        menu.addSubMenu("Project Templates", getProjectTemplatesMenu());
        menu.addCommandItem(commandManager, CommandId::saveProjectId);
        menu.addCommandItem(commandManager, CommandId::saveProjectAsId);
        menu.addSeparator();
        menu.addCommandItem(commandManager, CommandId::openSpeakerSetupId);
        menu.addSubMenu("Speaker Setup Templates", getSpeakerSetupTemplatesMenu());
        menu.addCommandItem(commandManager, CommandId::saveSpeakerSetupId);
        menu.addCommandItem(commandManager, CommandId::saveSpeakerSetupAsId);
        menu.addSeparator();
        menu.addCommandItem(commandManager, CommandId::openSettingsWindowId);
#if !JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem(commandManager, CommandId::quitId);
#endif
    } else if (menuName == "View") {
        menu.addCommandItem(commandManager, CommandId::show2DViewId);
        menu.addCommandItem(commandManager, CommandId::showSpeakerEditId);
        menu.addCommandItem(commandManager, CommandId::showPlayerWindowId);
        menu.addCommandItem(commandManager, CommandId::showOscMonitorId);
        menu.addCommandItem(commandManager, CommandId::showSpeakerViewId);
        menu.addSeparator();
        menu.addCommandItem(commandManager, CommandId::keepSpeakerViewOnTopId);
        menu.addSeparator();
        menu.addCommandItem(commandManager, CommandId::showHallId);
        menu.addCommandItem(commandManager, CommandId::showSourceNumbersId);
        menu.addCommandItem(commandManager, CommandId::showSpeakerNumbersId);
        menu.addCommandItem(commandManager, CommandId::showSpeakersId);
        menu.addCommandItem(commandManager, CommandId::keepSpeakersOriginOrientatedId);
        if (mAudioProcessor->getSpatAlgorithm()->hasTriplets()) {
            menu.addCommandItem(commandManager, CommandId::showTripletsId);
        } else {
            menu.addItem(CommandId::showTripletsId, "Show Speaker Triplets", false, false);
        }
        menu.addCommandItem(commandManager, CommandId::showSourceActivityId);
        menu.addCommandItem(commandManager, CommandId::showSpeakerActivityId);
        menu.addCommandItem(commandManager, CommandId::showSphereId);
        menu.addSeparator();
        menu.addCommandItem(commandManager, CommandId::colorizeInputsId);
        menu.addCommandItem(commandManager, CommandId::resetInputPosId);
        menu.addCommandItem(commandManager, CommandId::resetMeterClipping);
        menu.addCommandItem(commandManager, CommandId::muteAllSpeakers);
    } else if (menuName == "Help") {
        menu.addCommandItem(commandManager, CommandId::aboutId);
        menu.addCommandItem(commandManager, CommandId::openManualENId);
        menu.addCommandItem(commandManager, CommandId::openManualFRId);
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
    auto const savedProject{ ProjectData::fromXml(*savedElement) };
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
    return mData.speakerSetup != *savedSpeakerSetup;
}

//==============================================================================
bool MainContentComponent::exitApp()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    static constexpr auto EXIT_APP = true;
    static constexpr auto DONT_EXIT_APP = false;

    if (!makeSureSpeakerSetupIsSavedToDisk()) {
        return DONT_EXIT_APP;
    }

    if (!makeSureProjectIsSavedToDisk()) {
        return DONT_EXIT_APP;
    }

    return EXIT_APP;
}

//==============================================================================
void MainContentComponent::updatePeaks()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto & viewPortData{ mSpeakerViewComponent->getData() };
    auto * flatViewViewportDataQueues{ mFlatViewWindow ? &mFlatViewWindow->getSourceDataQueues() : nullptr };

    auto & audioData{ mAudioProcessor->getAudioData() };
    auto *& sourcePeaksTicket{ mData.mostRecentSourcePeaks };
    audioData.sourcePeaksUpdater.getMostRecent(sourcePeaksTicket);
    if (!sourcePeaksTicket) {
        return;
    }
    auto const & sourcePeaks{ sourcePeaksTicket->get() };
    for (auto const sourceData : mData.project.sources) {
        auto const & peak{ sourcePeaks[sourceData.key] };
        auto const dbPeak{ dbfs_t::fromGain(peak) };
        mSourceSliceComponents[sourceData.key].setLevel(dbPeak);

        if (!sourceData.value->position) {
            continue;
        }

        auto const data{ sourceData.value->toViewportData(
            mData.appData.viewSettings.showSourceActivity ? gainToSourceAlpha(sourceData.key, peak) : 0.8f) };

        // update 3d view
        if (!viewPortData.hotSourcesDataUpdaters.contains(sourceData.key)) {
            viewPortData.hotSourcesDataUpdaters.add(sourceData.key);
        }

        {
            auto & exchanger{ viewPortData.hotSourcesDataUpdaters[sourceData.key] };
            auto * ticket{ exchanger.acquire() };
            ticket->get() = data;
            exchanger.setMostRecent(ticket);
        }

        // update 2d view
        if (flatViewViewportDataQueues) {
            auto & exchanger{ (*flatViewViewportDataQueues)[sourceData.key] };
            auto * ticket{ exchanger.acquire() };
            ticket->get() = data;
            exchanger.setMostRecent(ticket);
        }
    }

    if (mData.appData.stereoMode) {
        auto *& stereoPeaksTicket{ mData.mostRecentStereoPeaks };
        audioData.stereoPeaksUpdater.getMostRecent(stereoPeaksTicket);
        if (stereoPeaksTicket == nullptr) {
            return;
        }

        auto const & stereoPeaks{ stereoPeaksTicket->get() };
        for (size_t i{}; i < 2; ++i) {
            auto const & peak{ stereoPeaks[i] };
            auto const dbPeak{ dbfs_t::fromGain(peak) };
            mStereoSliceComponents[static_cast<int>(i)]->setLevel(dbPeak);
        }
        if (mData.appData.viewSettings.showSpeakerLevels) {
            auto *& speakerPeaksTicket{ mData.mostRecentSpeakerPeaks };
            audioData.speakerPeaksUpdater.getMostRecent(speakerPeaksTicket);
            if (speakerPeaksTicket == nullptr) {
                return;
            }
            auto const & speakerPeaks{ speakerPeaksTicket->get() };
            for (auto const speaker : mData.speakerSetup.speakers) {
                auto const & peak{ speakerPeaks[speaker.key] };
                auto & exchanger{ viewPortData.hotSpeakersAlphaUpdaters[speaker.key] };
                auto * ticket{ exchanger.acquire() };
                ticket->get() = gainToSpeakerAlpha(peak);
                exchanger.setMostRecent(ticket);
            }
        }
        return;
    }

    auto *& speakerPeaksTicket{ mData.mostRecentSpeakerPeaks };
    audioData.speakerPeaksUpdater.getMostRecent(speakerPeaksTicket);
    if (speakerPeaksTicket == nullptr) {
        return;
    }
    auto const & speakerPeaks{ speakerPeaksTicket->get() };
    for (auto const speaker : mData.speakerSetup.speakers) {
        auto const & peak{ speakerPeaks[speaker.key] };
        auto const dbPeak{ dbfs_t::fromGain(peak) };
        mSpeakerSliceComponents[speaker.key].setLevel(dbPeak);

        auto & exchanger{ viewPortData.hotSpeakersAlphaUpdaters[speaker.key] };
        auto * ticket{ exchanger.acquire() };
        ticket->get() = gainToSpeakerAlpha(peak);
        exchanger.setMostRecent(ticket);
    }
}

//==============================================================================
void MainContentComponent::refreshSourceSlices()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    mSourcesInnerLayout->clearSections();
    mSourceSliceComponents.clear();

    auto const isAtLeastOneSourceSolo{ std::any_of(
        mData.project.sources.cbegin(),
        mData.project.sources.cend(),
        [](SourcesData::ConstNode const & node) { return node.value->state == SliceState::solo; }) };

    auto const directOutChoices{ std::make_shared<DirectOutSelectorComponent::Choices>() };

    for (const auto speaker : mData.speakerSetup.speakers) {
        auto & destination{ speaker.value->isDirectOutOnly ? directOutChoices->directOutputPatches
                                                           : directOutChoices->nonDirectOutputPatches };
        destination.add(speaker.key);
    }

    for (auto source : mData.project.sources) {
        auto newSlice{ std::make_unique<SourceSliceComponent>(source.key,
                                                              source.value->directOut,
                                                              mData.project.spatMode,
                                                              source.value->hybridSpatMode,
                                                              source.value->colour,
                                                              directOutChoices,
                                                              *this,
                                                              mLookAndFeel,
                                                              mSmallLookAndFeel) };
        auto const & state{ source.value->state };
        newSlice->setState(state, isAtLeastOneSourceSolo);
        auto & addedSlice{ mSourceSliceComponents.add(source.key, std::move(newSlice)) };
        mSourcesInnerLayout->addSection(&addedSlice).withChildMinSize();
    }

    mSourcesInnerLayout->resized();
}

//==============================================================================
void MainContentComponent::refreshSpeakerSlices()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    mSpeakersLayout->clearSections();
    mSpeakerSliceComponents.clear();
    mStereoSliceComponents.clearQuick(true);

    auto const isAtLeastOneSpeakerSolo{ std::any_of(
        mData.speakerSetup.speakers.cbegin(),
        mData.speakerSetup.speakers.cend(),
        [](SpeakersData::ConstNode const & node) { return node.value->state == SliceState::solo; }) };

    if (mData.appData.stereoMode) {
        auto const slicesState{ mData.speakerSetup.generalMute ? SliceState::muted : SliceState::normal };
        auto newLeftSlice{ std::make_unique<StereoSliceComponent>("L", mLookAndFeel, mSmallLookAndFeel) };
        auto newRightSlice{ std::make_unique<StereoSliceComponent>("R", mLookAndFeel, mSmallLookAndFeel) };
        newLeftSlice->setState(slicesState, false);
        newRightSlice->setState(slicesState, false);
        mSpeakersLayout->addSection(mStereoSliceComponents.add(std::move(newLeftSlice))).withChildMinSize();
        mSpeakersLayout->addSection(mStereoSliceComponents.add(std::move(newRightSlice))).withChildMinSize();
    } else {
        for (auto const outputPatch : mData.speakerSetup.ordering) {
            auto newSlice{
                std::make_unique<SpeakerSliceComponent>(outputPatch, *this, mLookAndFeel, mSmallLookAndFeel)
            };
            auto const & state{ mData.speakerSetup.speakers[outputPatch].state };
            newSlice->setState(state, isAtLeastOneSpeakerSolo);
            auto & addedSlice{ mSpeakerSliceComponents.add(outputPatch, std::move(newSlice)) };
            mSpeakersLayout->addSection(&addedSlice).withChildMinSize();
        }
    }

    mSpeakersLayout->resized();
}

//==============================================================================
void MainContentComponent::updateSourceSpatData(source_index_t const sourceIndex)
{
    jassert(!isProbablyAudioThread());

    juce::ScopedReadLock const lock{ mLock };
    mAudioProcessor->getSpatAlgorithm()->updateSpatData(sourceIndex, mData.project.sources[sourceIndex]);
}

//==============================================================================
void MainContentComponent::setLegacySourcePosition(source_index_t const sourceIndex,
                                                   radians_t const azimuth,
                                                   radians_t const elevation,
                                                   float const length,
                                                   float const newAzimuthSpan,
                                                   float const newZenithSpan)
{
    ASSERT_OSC_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    if (!mData.project.sources.contains(sourceIndex)) {
        jassertfalse;
        return;
    }

    auto const getCorrectedPosition = [&]() -> Position {
        auto const & projectSpatMode{ mData.project.spatMode };
        auto const spatMode{ projectSpatMode == SpatMode::hybrid ? mData.project.sources[sourceIndex].hybridSpatMode
                                                                 : projectSpatMode };

        switch (spatMode) {
        case SpatMode::vbap:
            return Position{ PolarVector{ azimuth, elevation, 1.0f } };

        case SpatMode::mbap:
            return LegacyLbapPosition{ azimuth, elevation, length }.toPosition();
        case SpatMode::hybrid:
        case SpatMode::invalid:
            break;
        }
        jassertfalse;
        return {};
    };

    auto const correctedPosition{ getCorrectedPosition() };
    auto & source{ mData.project.sources[sourceIndex] };

    if (correctedPosition == source.position && newAzimuthSpan == source.azimuthSpan
        && newZenithSpan == source.zenithSpan) {
        return;
    }

    juce::ScopedWriteLock const writeLock{ mLock };

    source.position = correctedPosition;
    source.azimuthSpan = newAzimuthSpan;
    source.zenithSpan = newZenithSpan;

    updateSourceSpatData(sourceIndex);
}

//==============================================================================
void MainContentComponent::setSourcePosition(source_index_t const sourceIndex,
                                             Position position,
                                             float azimuthSpan,
                                             float zenithSpan)
{
    ASSERT_OSC_THREAD;

    juce::ScopedWriteLock const lock{ getLock() };

    if (!mData.project.sources.contains(sourceIndex)) {
        jassertfalse;
        return;
    }

    auto & source{ mData.project.sources[sourceIndex] };

    azimuthSpan = std::clamp(azimuthSpan, 0.0f, 1.0f);
    zenithSpan = std::clamp(zenithSpan, 0.0f, 1.0f);

    auto const & projectSpatMode{ mData.project.spatMode };
    auto const effectiveSpatMode{ projectSpatMode == SpatMode::hybrid ? source.hybridSpatMode : projectSpatMode };
    switch (effectiveSpatMode) {
    case SpatMode::vbap:
        position = position.getPolar().normalized();
        break;
    case SpatMode::mbap:
        position = position.getCartesian().clampedToFarField();
        break;
    case SpatMode::hybrid:
    case SpatMode::invalid:
        jassertfalse;
        break;
    }

    if (position == source.position && azimuthSpan == source.azimuthSpan && zenithSpan == source.zenithSpan) {
        return;
    }

    source.position = position;
    source.azimuthSpan = azimuthSpan;
    source.zenithSpan = zenithSpan;

    updateSourceSpatData(sourceIndex);
}

//==============================================================================
void MainContentComponent::resetSourcePosition(source_index_t const sourceIndex)
{
    juce::ScopedWriteLock const lock{ mLock };

    if (!mData.project.sources.contains(sourceIndex)) {
        jassertfalse;
        return;
    }

    mData.project.sources[sourceIndex].position = tl::nullopt;
    updateSourceSpatData(sourceIndex);
}

//==============================================================================
void MainContentComponent::speakerDirectOutOnlyChanged(output_patch_t const outputPatch, bool const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & speaker{ mData.speakerSetup.speakers[outputPatch] };
    auto & val{ speaker.isDirectOutOnly };
    if (state != val) {
        val = state;

        if (!state && mData.speakerSetup.spatMode != SpatMode::mbap) {
            speaker.position = speaker.position.normalized();
        }

        refreshAudioProcessor();
        refreshViewportConfig();
        refreshSourceSlices();
    }
}

//==============================================================================
void MainContentComponent::speakerOutputPatchChanged(output_patch_t const oldOutputPatch,
                                                     output_patch_t const newOutputPatch)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & speakers{ mData.speakerSetup.speakers };
    speakers.add(newOutputPatch, std::make_unique<SpeakerData>(speakers[oldOutputPatch]));
    speakers.remove(oldOutputPatch);

    auto & order{ mData.speakerSetup.ordering };
    order.set(order.indexOf(oldOutputPatch), newOutputPatch);

    refreshSourceSlices();
    refreshSpeakerSlices();
}

//==============================================================================
void MainContentComponent::setSpeakerGain(output_patch_t const outputPatch, dbfs_t const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.speakerSetup.speakers[outputPatch].gain = gain;

    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::setSpeakerHighPassFreq(output_patch_t const outputPatch, hz_t const freq)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    if (!mData.speakerSetup.speakers.contains(outputPatch)) {
        jassertfalse;
        return;
    }

    auto & speaker{ mData.speakerSetup.speakers[outputPatch] };

    if (freq == hz_t{ 0.0f }) {
        speaker.highpassData.reset();
    } else {
        speaker.highpassData = SpeakerHighpassData{ freq };
    }
    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::setOscPort(int const newOscPort)
{
    juce::ScopedWriteLock const lock{ mLock };
    mData.project.oscPort = newOscPort;
    if (!mOscInput) {
        return;
    }
    mOscInput->closeConnection();
    mOscInput->startConnection(newOscPort);
}

//==============================================================================
void MainContentComponent::setSpeakerSetupDiffusion(float diffusion)
{
    mData.speakerSetup.diffusion = diffusion;
}

//==============================================================================
void MainContentComponent::setPinkNoiseGain(tl::optional<dbfs_t> const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    if (gain == mData.pinkNoiseLevel) {
        return;
    }

    mData.pinkNoiseLevel = gain;
    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::setSourceColor(source_index_t const sourceIndex, juce::Colour const colour)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    if (sourceIndex.get() == mData.project.sources.size() + 1) {
        // Right click on the last source's color selector
        return;
    }

    mData.project.sources[sourceIndex].colour = colour;
    mSourceSliceComponents[sourceIndex].setSourceColour(colour);
}

//==============================================================================
void MainContentComponent::setSourceState(source_index_t const sourceIndex, SliceState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.sources[sourceIndex].state = state;

    refreshAudioProcessor();
    refreshSourceSlices();
}

//==============================================================================
void MainContentComponent::setSelectedSpeakers(juce::Array<output_patch_t> const selection)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    if (mEditSpeakersWindow) {
        output_patch_t const unusedOutputPatchFromSV{ 0 };
        if (selection.contains(unusedOutputPatchFromSV))
            return;
    }

    for (auto const speaker : mData.speakerSetup.speakers) {
        auto const isSelected{ selection.contains(speaker.key) };

        if (speaker.value->isSelected == isSelected) {
            if (speaker.value->isSelected && !mEditSpeakersWindow) {
                speaker.value->isSelected = false;
                mSpeakerSliceComponents[speaker.key].setSelected(false);
            }
            continue;
        }

        speaker.value->isSelected = isSelected;
        if (mSpeakerSliceComponents.contains(speaker.key)) {
            mSpeakerSliceComponents[speaker.key].setSelected(isSelected);
        }

        if (!isSelected || !mEditSpeakersWindow) {
            continue;
        }

        mEditSpeakersWindow->selectSpeaker(speaker.key);
    }

    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::setSpeakerState(output_patch_t const outputPatch, SliceState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.speakerSetup.speakers[outputPatch].state = state;

    refreshAudioProcessor();
    refreshSpeakerSlices();
}

//==============================================================================
void MainContentComponent::setSourceDirectOut(source_index_t const sourceIndex,
                                              tl::optional<output_patch_t> const outputPatch)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.project.sources[sourceIndex].directOut = outputPatch;

    if (mData.appData.stereoMode) {
        refreshSpeakers();
    } else {
        refreshAudioProcessor();
    }

    mSourceSliceComponents[sourceIndex].setDirectOut(outputPatch);
}

//==============================================================================
void MainContentComponent::refreshAudioProcessor() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    if (!mAudioProcessor) {
        return;
    }

    mAudioProcessor->setAudioConfig(mData.toAudioConfig());
}

//==============================================================================
void MainContentComponent::refreshSpatAlgorithm()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    if (!mAudioProcessor) {
        return;
    }

    juce::ScopedLock const audioLock{ mAudioProcessor->getLock() };

    auto & oldSpatAlgorithm{ mAudioProcessor->getSpatAlgorithm() };
    auto newSpatAlgorithm{ AbstractSpatAlgorithm::make(mData.speakerSetup,
                                                       mData.project.spatMode,
                                                       mData.appData.stereoMode,
                                                       mData.project.sources,
                                                       mData.appData.audioSettings.sampleRate,
                                                       mData.appData.audioSettings.bufferSize) };

    if (newSpatAlgorithm->getError()
        && (!oldSpatAlgorithm || oldSpatAlgorithm->getError() != newSpatAlgorithm->getError())) {
        switch (*newSpatAlgorithm->getError()) {
        case AbstractSpatAlgorithm::Error::notEnoughDomeSpeakers:
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::InfoIcon,
                                                   "Disabled spatialization",
                                                   "Domes need at least 3 speakers.\n",
                                                   "Ok",
                                                   this);
            break;
        case AbstractSpatAlgorithm::Error::notEnoughCubeSpeakers:
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::InfoIcon,
                                                   "Disabled spatialization",
                                                   "The Cubes need at least 2 speakers.\n",
                                                   "Ok",
                                                   this);
            break;
        case AbstractSpatAlgorithm::Error::flatDomeSpeakersTooFarApart:
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::InfoIcon,
                                                   "Disabled spatialization",
                                                   "If all speakers are at the same height, Domes require their "
                                                   "speakers not to be more than 170 degrees apart from each others.\n",
                                                   "Ok",
                                                   this);
            break;
        default:
            jassertfalse;
            break;
        }
    }

    oldSpatAlgorithm = std::move(newSpatAlgorithm);

    refreshAudioProcessor();
    reassignSourcesPositions();
}

//==============================================================================
void MainContentComponent::refreshViewportConfig() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    if (!mAudioProcessor) {
        return;
    }

    auto const & spatAlgorithm{ *mAudioProcessor->getSpatAlgorithm() };

    auto const newConfig{ mData.toViewportConfig() };
    if (newConfig.viewSettings.showSpeakerTriplets && spatAlgorithm.hasTriplets()) {
        mSpeakerViewComponent->setTriplets(spatAlgorithm.getTriplets());
    }
    mSpeakerViewComponent->setConfig(newConfig, mData.project.sources);
}

//==============================================================================
void MainContentComponent::setShowTriplets(bool const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    mData.appData.viewSettings.showSpeakerTriplets = state;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::setSourceHybridSpatMode(source_index_t const sourceIndex, SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & sources{ mData.project.sources };

    if (!sources.contains(sourceIndex)) {
        jassertfalse;
        return;
    }

    auto & source{ sources[sourceIndex] };

    source.hybridSpatMode = spatMode;

    // we need to erase the position in the current spat algorithm
    auto const position{ source.position };
    source.position = tl::nullopt;
    updateSourceSpatData(sourceIndex);

    // we now reinstate the position and update again
    source.position = position;
    updateSourceSpatData(sourceIndex);

    refreshViewportConfig();
    mSourceSliceComponents[sourceIndex].setHybridSpatMode(spatMode);
}

//==============================================================================
void MainContentComponent::reorderSpeakers(juce::Array<output_patch_t> newOrder)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & order{ mData.speakerSetup.ordering };
    jassert(newOrder.size() == order.size());
    order = std::move(newOrder);
    refreshSpeakerSlices();
}

//==============================================================================
output_patch_t MainContentComponent::getMaxSpeakerOutputPatch() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto const & patches{ mData.speakerSetup.ordering };
    auto const * maxIterator{ std::max_element(patches.begin(), patches.end()) };
    auto const maxPatch{ maxIterator != mData.speakerSetup.ordering.end() ? *maxIterator : output_patch_t{} };
    return maxPatch;
}

//==============================================================================
tl::optional<SpeakerSetup> MainContentComponent::extractSpeakerSetup(juce::File const & file)
{
    auto const displayError = [&](juce::String const & message) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "Unable to open Speaker Setup.",
                                               message,
                                               "OK",
                                               nullptr,
                                               nullptr);
    };

    if (!file.existsAsFile()) {
        displayError("File \"" + file.getFullPathName() + "\" does not exist.");
        return tl::nullopt;
    }

    juce::XmlDocument xmlDoc{ file };
    auto const mainXmlElem(xmlDoc.getDocumentElement());
    if (!mainXmlElem) {
        displayError("Corrupted file.\n" + xmlDoc.getLastParseError());
        return tl::nullopt;
    }

    if (mainXmlElem->hasTagName("ServerGRIS_Preset") || mainXmlElem->hasTagName(ProjectData::XmlTags::MAIN_TAG)) {
        displayError("This is a project file, not a Speaker Setup !");
        return tl::nullopt;
    }

    auto speakerSetup{ SpeakerSetup::fromXml(*mainXmlElem) };

    if (!speakerSetup) {
        auto const version{ SpatGrisVersion::fromString(
            mainXmlElem->getStringAttribute(SpeakerSetup::XmlTags::VERSION)) };
        if (version.compare(SPAT_GRIS_VERSION) > 0) {
            displayError("This speaker setup was created using a newer version of SpatGRIS that is not compatible with "
                         "this one.\nPlease upgrade to the latest version.");
        } else {
            displayError("File \"" + file.getFullPathName()
                         + "\" is missing one more mandatory parameters.\nYour file might be corrupted.");
        }
    }

    return speakerSetup;
}

//==============================================================================
output_patch_t MainContentComponent::addSpeaker(tl::optional<output_patch_t> const speakerToCopy,
                                                tl::optional<int> const index)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto newSpeaker{ std::make_unique<SpeakerData>() };

    if (speakerToCopy) {
        auto const speakerToCopyExists{ mData.speakerSetup.speakers.contains(*speakerToCopy) };
        jassert(speakerToCopyExists);
        if (speakerToCopyExists) {
            *newSpeaker = mData.speakerSetup.speakers[*speakerToCopy];
        }
    }

    auto const newOutputPatch{ ++getMaxSpeakerOutputPatch() };

    if (index) {
        auto const isValidIndex{ *index >= 0 && *index < mData.speakerSetup.ordering.size() };
        if (isValidIndex) {
            mData.speakerSetup.ordering.insert(*index, newOutputPatch);
        } else {
            static constexpr auto AT_END = -1;
            mData.speakerSetup.ordering.insert(AT_END, newOutputPatch);
        }
    } else {
        mData.speakerSetup.ordering.add(newOutputPatch);
    }

    mData.speakerSetup.speakers.add(newOutputPatch, std::move(newSpeaker));

    return newOutputPatch;
}

//==============================================================================
void MainContentComponent::removeSpeaker(output_patch_t const outputPatch)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const dataLock{ mLock };
    juce::ScopedLock const audioLock{ mAudioProcessor->getLock() };

    mData.speakerSetup.ordering.removeFirstMatchingValue(outputPatch);
    mData.speakerSetup.speakers.remove(outputPatch);

    refreshSpeakers();
}

//==============================================================================
void MainContentComponent::refreshSpeakers()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    if (!mAudioProcessor) {
        return;
    }

    refreshSpatAlgorithm();

    if (!mAudioProcessor->getSpatAlgorithm()->hasTriplets()) {
        mData.appData.viewSettings.showSpeakerTriplets = false;
    }

    refreshSourceSlices();
    refreshSpeakerSlices();
    refreshViewportConfig();
    if (mEditSpeakersWindow != nullptr) {
        mEditSpeakersWindow->updateWinContent();
    }
}

//==============================================================================
bool MainContentComponent::loadSpeakerSetup(juce::File const & file, LoadSpeakerSetupOption const option)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto speakerSetup{ extractSpeakerSetup(file) };

    if (!speakerSetup) {
        return false;
    }

    if (option == LoadSpeakerSetupOption::disallowDiscardingUnsavedChanges) {
        if (!makeSureSpeakerSetupIsSavedToDisk()) {
            return false;
        }
    }

    juce::ScopedWriteLock const lock{ mLock };

    mIsLoadingSpeakerSetupOrProjectFile = true;

    mData.speakerSetup = std::move(*speakerSetup);
    mData.appData.lastSpeakerSetup = file.getFullPathName();

    auto newSpatMode{ mData.speakerSetup.spatMode };

    if (mData.project.spatMode == SpatMode::hybrid && newSpatMode == SpatMode::vbap) {
        newSpatMode = SpatMode::hybrid;
    }

    if (mData.speakerSetup.generalMute) {
        for (auto& speaker : mData.speakerSetup.speakers) {
            speaker.value->state = SliceState::muted;
        }
    }

    setSpatMode(newSpatMode);
    mIsLoadingSpeakerSetupOrProjectFile = false;

    if (mPlayerWindow != nullptr) {
        mPlayerWindow->reloadPlayer();
    }

    return true;
}

//==============================================================================
void MainContentComponent::setTitles() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto const projectFileName{ juce::File{ mData.appData.lastProject }.getFileNameWithoutExtension() };
    auto const mainWindowTitle{ juce::String{ "SpatGRIS v" }
                                + juce::JUCEApplication::getInstance()->getApplicationVersion() + " - "
                                + projectFileName };
    mMainWindow.DocumentWindow::setName(mainWindowTitle);

    if (mEditSpeakersWindow != nullptr) {
        auto const speakerSetupFileName{ juce::File{ mData.appData.lastSpeakerSetup }.getFileNameWithoutExtension() };
        auto const windowName{ juce::String{ "Speaker Setup Edition - " }
                               + spatModeToString(mData.speakerSetup.spatMode) + juce::String(" - ")
                               + speakerSetupFileName };
        mEditSpeakersWindow->setName(windowName);
    }

    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::reassignSourcesPositions()
{
    for (auto const & source : mData.project.sources) {
        if (!source.value->position) {
            continue;
        }
        updateSourceSpatData(source.key);
    }
}

//==============================================================================
void MainContentComponent::startOsc()
{
    mOscInput.reset(new OscInput(*this, mLogBuffer));
    mOscInput->startConnection(mData.project.oscPort);
}

//==============================================================================
void MainContentComponent::stopOsc()
{
    mOscInput->closeConnection();
    mOscInput.reset();
}

//==============================================================================
void MainContentComponent::handlePlayerPlayStop()
{
    if (mPlayerWindow != nullptr) {
        const juce::KeyPress spaceKey(juce::KeyPress::spaceKey);
        mPlayerWindow->keyPressed(spaceKey);
    }
}

//==============================================================================
int MainContentComponent::getSpeakerViewPIDOnMacOS() const
{
    juce::ChildProcess checkPID;
    juce::StringArray launchCommand;

    launchCommand.add("pgrep");
    launchCommand.add("SpeakerView");

    [[maybe_unused]] auto res{ checkPID.start(launchCommand) };
    auto pid{ checkPID.readAllProcessOutput().getIntValue() };

    return pid ? pid : -1;
}

//==============================================================================
bool MainContentComponent::isSpeakerViewProcessRunning() const
{
    if (juce::SystemStats::getOperatingSystemName().contains("Mac")) {
        auto const speakerViewPID{ getSpeakerViewPIDOnMacOS() };
        if (speakerViewPID > 0) {
            return true;
        }
    }

    return mSpeakerViewProcess.isRunning();
}

//==============================================================================
void MainContentComponent::buttonPressed([[maybe_unused]] SpatButton * button)
{
    jassert(button == &mAddRemoveSourcesButton);
    mAddRemoveSourcesWindow
        = std::make_unique<AddRemoveSourcesWindow>(mData.project.sources.size(), *this, mLookAndFeel);
}

//==============================================================================
void MainContentComponent::handleSaveSpeakerSetup()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    saveSpeakerSetup(mData.appData.lastSpeakerSetup);
}

//==============================================================================
bool MainContentComponent::saveProject(tl::optional<juce::File> maybeFile)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    static auto const IS_SAVABLE = [](juce::File const & file) { return !file.isAChildOf(CURRENT_WORKING_DIR); };

    if (!maybeFile || !IS_SAVABLE(*maybeFile)) {
        juce::File const lastProjectFile{ mData.appData.lastProject };
        auto const initialFile{ lastProjectFile.isAChildOf(CURRENT_WORKING_DIR)
                                    ? juce::File::getSpecialLocation(
                                          juce::File::SpecialLocationType::userDocumentsDirectory)
                                          .getChildFile(lastProjectFile.getFileName())
                                    : lastProjectFile };
        juce::FileChooser fc{ "Choose file to save to...", initialFile, "*.xml", true, false, this };
        if (!fc.browseForFileToSave(true)) {
            return false;
        }

        maybeFile = fc.getResult();
    }

    auto const file{ *maybeFile };
    auto const content{ mData.project.toXml() };
    auto const success{ content->writeTo(file) };
    jassert(success);

    if (success) {
        mData.appData.lastProject = maybeFile->getFullPathName();
    }

    setTitles();

    return success;
}

//==============================================================================
bool MainContentComponent::saveSpeakerSetup(tl::optional<juce::File> maybeFile)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    static auto const IS_SAVABLE = [](juce::File const & file) { return !file.isAChildOf(CURRENT_WORKING_DIR); };

    if (!maybeFile || !IS_SAVABLE(*maybeFile)) {
        juce::File const & lastSpeakerSetup{ mData.appData.lastSpeakerSetup };
        auto const initialFile{ lastSpeakerSetup.isAChildOf(CURRENT_WORKING_DIR)
                                    ? juce::File::getSpecialLocation(
                                          juce::File::SpecialLocationType::userDocumentsDirectory)
                                          .getChildFile(juce::File{ mData.appData.lastSpeakerSetup }.getFileName())
                                    : lastSpeakerSetup };
        juce::FileChooser fc{ "Choose file to save to...", initialFile, "*.xml", true, false, this };
        if (!fc.browseForFileToSave(true)) {
            return false;
        }

        maybeFile = fc.getResult();
    }

    auto const & file{ *maybeFile };
    auto const content{ mData.speakerSetup.toXml() };
    auto const success{ content->writeTo(file) };

    jassert(success);
    if (success) {
        mData.appData.lastSpeakerSetup = maybeFile->getFullPathName();
    }

    setTitles();

    return success;
}

//==============================================================================
bool MainContentComponent::makeSureProjectIsSavedToDisk() noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (!isProjectModified()) {
        return true;
    }

    juce::AlertWindow alertWindow{ "Unsaved project",
                                   "Your current project has unsaved changes. Do you wish to save it to disk?",
                                   juce::AlertWindow::AlertIconType::InfoIcon,
                                   this };
    alertWindow.addButton("Save", BUTTON_OK, juce::KeyPress{ juce::KeyPress::returnKey });
    alertWindow.addButton("Discard", BUTTON_DISCARD);
    alertWindow.addButton("Cancel", BUTTON_CANCEL, juce::KeyPress{ juce::KeyPress::escapeKey });

    auto const pressedButton{ alertWindow.runModalLoop() };

    if (pressedButton == BUTTON_CANCEL) {
        return false;
    }

    if (pressedButton == BUTTON_DISCARD) {
        return true;
    }

    jassert(pressedButton == BUTTON_OK);

    return saveProject(tl::nullopt);
}

//==============================================================================
bool MainContentComponent::makeSureSpeakerSetupIsSavedToDisk() noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (!isSpeakerSetupModified()) {
        return true;
    }

    juce::AlertWindow alertWindow{ "Unsaved speaker setup",
                                   "Your current speaker setup has unsaved changes. Do you wish to save it to disk?",
                                   juce::AlertWindow::AlertIconType::InfoIcon,
                                   this };
    alertWindow.addButton("Save", BUTTON_OK, juce::KeyPress{ juce::KeyPress::returnKey });
    alertWindow.addButton("Discard", BUTTON_DISCARD);
    alertWindow.addButton("Cancel", BUTTON_CANCEL, juce::KeyPress{ juce::KeyPress::escapeKey });

    auto const pressedButton{ alertWindow.runModalLoop() };

    if (pressedButton == BUTTON_CANCEL) {
        return false;
    }

    if (pressedButton == BUTTON_DISCARD) {
        return true;
    }

    jassert(pressedButton == BUTTON_OK);

    return saveSpeakerSetup(tl::nullopt);
}

//==============================================================================
void MainContentComponent::timerCallback()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    // Update levels
    if (!mIsLoadingSpeakerSetupOrProjectFile) {
        updatePeaks();
    }

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

    mInfoPanel->setCpuLoad(cpuRunningAverage);

    if (mIsProcessForeground != juce::Process::isForegroundProcess()) {
        mIsProcessForeground = juce::Process::isForegroundProcess();
        if (mEditSpeakersWindow != nullptr && mIsProcessForeground) {
            mEditSpeakersWindow->setAlwaysOnTop(true);
        } else if (mEditSpeakersWindow != nullptr && !mIsProcessForeground) {
            mEditSpeakersWindow->setAlwaysOnTop(false);
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
juce::StringArray MainContentComponent::getMenuBarNames()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    char const * names[] = { "File", "View", "Help", nullptr };
    return juce::StringArray{ names };
}

//==============================================================================
void MainContentComponent::prepareAndStartRecording(juce::File const & fileOrDirectory,
                                                    RecordingOptions const & recordingOptions)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    {
        juce::ScopedWriteLock const lock{ mLock };

        mData.appData.lastRecordingDirectory = fileOrDirectory.getParentDirectory().getFullPathName();
        mData.appData.recordingOptions = recordingOptions;
    }

    juce::ScopedReadLock const lock{ mLock };
    auto const getSpeakersToRecord = [&]() -> juce::Array<output_patch_t> {
        juce::Array<output_patch_t> result{};

        if (mData.appData.stereoMode) {
            // I'm not even sure this is useful
            auto const & routing{ mData.appData.stereoRouting };
            result.add(routing.left);
            result.add(routing.right);
            return result;
        }

        result = mData.speakerSetup.ordering;
        result.sort();

        return result;
    };

    auto speakersToRecord = getSpeakersToRecord();

    AudioManager::RecordingParameters const recordingParams{ fileOrDirectory.getFullPathName(),
                                                             mData.appData.recordingOptions,
                                                             mData.appData.audioSettings.sampleRate,
                                                             std::move(speakersToRecord) };
    if (AudioManager::getInstance().prepareToRecord(recordingParams)) {
        AudioManager::getInstance().startRecording();

        // export speaker setup
        if (recordingOptions.shouldSaveSpeakerSetup) {
            juce::File const & lastSpeakerSetup{ mData.appData.lastSpeakerSetup };
            auto const file{ fileOrDirectory.getParentDirectory().getFullPathName() + juce::File::getSeparatorString()
                             + lastSpeakerSetup.getFileName() };
            auto const content{ mData.speakerSetup.toXml() };
            [[maybe_unused]] auto const success{ content->writeTo(file) };
            jassert(success);
        }

        mControlPanel->setRecordButtonState(RecordButton::State::recording);
        mPrepareToRecordWindow.reset();
    }
}

//==============================================================================
tl::optional<SpeakerSetup> MainContentComponent::playerExtractSpeakerSetup(juce::File const & file)
{
    juce::XmlDocument xmlDoc{ file };
    auto const mainXmlElem{ xmlDoc.getDocumentElement() };

    if (mainXmlElem->hasTagName("SpeakerSetup") || mainXmlElem->hasTagName(SpeakerSetup::XmlTags::MAIN_TAG)) {
        return extractSpeakerSetup(file);
    }

    return tl::nullopt;
}

//==============================================================================
void MainContentComponent::handlePlayerSourcesPositions(tl::optional<SpeakerSetup> & playerSpeakerSetup,
                                                        juce::File & playerFilesFolder)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (!playerSpeakerSetup) {
        return;
    }

    auto const playerProjectFile{ playerFilesFolder.getFullPathName() + juce::File::getSeparatorString()
                                  + "player_project.xml" };
    const juce::File projectFile{ playerProjectFile };
    if (projectFile.existsAsFile()) {
        loadProject(projectFile, true);
    }

    int numberOfSources{};
    for (int i{}; i < playerSpeakerSetup->speakers.size(); ++i) {
        numberOfSources = juce::jmax(playerSpeakerSetup->ordering[i].get(), numberOfSources);
    }

    numSourcesChanged(numberOfSources);
    handleResetSourcesPositions();

    std::vector<bool> usedSources(numberOfSources);
    std::fill(usedSources.begin(), usedSources.end(), false);
    std::vector<bool> subSources(numberOfSources);
    std::fill(subSources.begin(), subSources.end(), false);

    for (auto & speaker : playerSpeakerSetup->speakers) {
        // update sources data from speakers position of speaker setup
        source_index_t const sourceIndex{ speaker.key.get() };
        auto source = mData.project.sources.getNode(sourceIndex);
        source.value->position = speaker.value->position;
        source.value->azimuthSpan = 0.0f;
        source.value->zenithSpan = 0.0f;
        usedSources[sourceIndex.get() - source_index_t::OFFSET] = true;

        if (!projectFile.existsAsFile()) {
            // manage source color
            if (speaker.value->isDirectOutOnly) {
                subSources[speaker.key.get() - source_index_t::OFFSET] = true;
            }
            source.value->colour = mLookAndFeel.getSourceColor();
            mSourceSliceComponents[source.key].setSourceColour(mLookAndFeel.getSourceColor());

            // reset source output
            setSourceDirectOut(sourceIndex, tl::nullopt);
            setSourceState(sourceIndex, SliceState::normal);
        }

        auto & spatAlgorithm{ *mAudioProcessor->getSpatAlgorithm() };
        spatAlgorithm.updateSpatData(source.key, *source.value);
    }

    if (!projectFile.existsAsFile()) {
        // set color of unused sources, mute them and remove direct out
        int i{};
        for (auto & source : mData.project.sources) {
            if (!usedSources[i]) {
                source.value->colour = mLookAndFeel.getInactiveColor();
                mSourceSliceComponents[source.key].setSourceColour(mLookAndFeel.getInactiveColor());

                source_index_t const sourceIndex{ i + source_index_t::OFFSET };
                setSourceState(sourceIndex, SliceState::muted);
                setSourceDirectOut(sourceIndex, tl::nullopt);
            }
            ++i;
        }

        // set color of subs
        i = 0;
        for (auto & source : mData.project.sources) {
            if (subSources[i]) {
                source.value->colour = mLookAndFeel.getSubColor();
                mSourceSliceComponents[source.key].setSourceColour(mLookAndFeel.getSubColor());
            }
            ++i;
        }

        // manage direct outputs speakers
        juce::Array<source_index_t> playerDirectOutSpeakers{};
        juce::Array<output_patch_t> speakerSetupDirectOutSpeakers{};
        for (auto & playerSpeaker : playerSpeakerSetup->speakers) {
            if (playerSpeaker.value->isDirectOutOnly) {
                source_index_t const sourceIndex{ playerSpeaker.key.get() };
                playerDirectOutSpeakers.add(sourceIndex);
            }
        }
        for (auto & speaker : mData.speakerSetup.speakers) {
            if (speaker.value->isDirectOutOnly) {
                speakerSetupDirectOutSpeakers.add(speaker.key);
            }
        }

        if (playerDirectOutSpeakers.size() > 0 && speakerSetupDirectOutSpeakers.size() > 0) {
            int setupIndex{};
            int playerIndex{};
            while (playerIndex < playerDirectOutSpeakers.size()) {
                setSourceDirectOut(playerDirectOutSpeakers[playerIndex], speakerSetupDirectOutSpeakers[setupIndex]);
                setSourceState(playerDirectOutSpeakers[playerIndex], SliceState::normal);
                ++setupIndex;
                ++playerIndex;
                if (setupIndex == speakerSetupDirectOutSpeakers.size()
                    && playerDirectOutSpeakers.size() > speakerSetupDirectOutSpeakers.size()) {
                    setupIndex %= speakerSetupDirectOutSpeakers.size();
                }
            }
        }
    }

    refreshAudioProcessor();
}

//==============================================================================
void MainContentComponent::handleNewProjectForPlayer()
{
    [[maybe_unused]] auto const success{ loadProject(DEFAULT_PROJECT_FILE, true) };
}

//==============================================================================
void MainContentComponent::handleResetSourcesPositionsFromSpeakerView()
{
    handleResetSourcesPositions();
}

//==============================================================================
void MainContentComponent::handleKeepSVOnTopFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.keepSpeakerViewWindowOnTop };
    var = value;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowHallFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showHall };
    var = value;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSourceNumbersFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSourceNumbers };

    if (var != value) {
        var = value;
        refreshViewportConfig();
    }
}

//==============================================================================
void MainContentComponent::handleShowSpeakerNumbersFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    {
        juce::ScopedWriteLock const lock{ mLock };

        auto & var{ mData.appData.viewSettings.showSpeakerNumbers };
        var = value;
    }
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSpeakersFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    {
        juce::ScopedWriteLock const lock{ mLock };

        auto & var{ mData.appData.viewSettings.showSpeakers };
        var = value;
    }
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSpeakerTripletsFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const readLock{ mLock };

    if (!mAudioProcessor->getSpatAlgorithm()->hasTriplets()) {
        return;
    }

    juce::ScopedWriteLock const writeLock{ mLock };
    mData.appData.viewSettings.showSpeakerTriplets = value;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSourceActivityFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSourceActivity };
    var = value;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSpeakerLevelFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSpeakerLevels };
    var = value;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleShowSphereOrCubeFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.appData.viewSettings.showSphereOrCube };
    var = value;
    refreshViewportConfig();
}

//==============================================================================
void MainContentComponent::handleGeneralMuteFromSpeakerView(bool value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto & var{ mData.speakerSetup.generalMute };

    if (var != value) {
        generalMuteButtonPressed();
    }
}

//==============================================================================
void MainContentComponent::handleWindowPositionFromSpeakerView(juce::String value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    value = value.removeCharacters("( )");
    auto posX = value.upToFirstOccurrenceOf(",", false, true).getIntValue();
    auto posY = value.fromFirstOccurrenceOf(",", false, true).getIntValue();

    auto & var{ mData.appData.speakerViewWindowPosition };

    var = juce::Point<int>(posX, posY);
}

//==============================================================================
void MainContentComponent::handleWindowSizeFromSpeakerView(juce::String value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    value = value.removeCharacters("( )");
    auto sizeX = value.upToFirstOccurrenceOf(",", false, true).getIntValue();
    auto sizeY = value.fromFirstOccurrenceOf(",", false, true).getIntValue();

    auto & var{ mData.appData.speakerViewWindowSize };

    var = juce::Point<int>(sizeX, sizeY);
}

//==============================================================================
void MainContentComponent::handleCameraPositionFromSpeakerView(juce::String value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    value = value.removeCharacters("( )");
    auto lengthStr = value.fromLastOccurrenceOf(",", false, true);
    value = value.dropLastCharacters(lengthStr.length() + 1);
    auto elevationStr = value.fromLastOccurrenceOf(",", false, true);
    value = value.dropLastCharacters(elevationStr.length() + 1);
    auto azimuthStr = value.fromLastOccurrenceOf(",", false, true);

    auto azimuth = static_cast<radians_t>(juce::degreesToRadians(azimuthStr.getFloatValue()));
    auto elevation = static_cast<radians_t>(juce::degreesToRadians(elevationStr.getFloatValue()));
    auto length = lengthStr.getFloatValue() / 10.0f;

    auto camPolarVec = PolarVector(azimuth, elevation, length);
    auto camPos = Position(camPolarVec);

    mSpeakerViewComponent->setCameraPosition(camPos.getCartesian());
}

//==============================================================================
bool MainContentComponent::speakerViewShouldGrabFocus()
{
    return mSpeakerViewShouldGrabFocus;
}

//==============================================================================
void MainContentComponent::resetSpeakerViewShouldGrabFocus()
{
    mSpeakerViewShouldGrabFocus = false;
}

//==============================================================================
bool MainContentComponent::savePlayerProject(juce::File & playerFilesFolder)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedWriteLock const lock{ mLock };

    auto const playerProjectFile{ playerFilesFolder.getFullPathName() + juce::File::getSeparatorString()
                                  + "player_project.xml" };
    const juce::File file{ playerProjectFile };
    bool shouldOverwriteProjectFile{ true };

    if (file.exists()) {
        auto const result{ juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon,
            "File exists",
            "The Player Project file already exists. Would you like to overwrite it ?",
            "Overwrite",
            "Cancel",
            nullptr,
            nullptr) };

        // 0 = Cancel, 1 = Overwrite
        if (result == 0) {
            shouldOverwriteProjectFile = false;
        }
    }

    if (shouldOverwriteProjectFile) {
        auto const content{ mData.project.toXml() };
        auto const success{ content->writeTo(playerProjectFile) };
        jassert(success);

        if (success) {
            mData.appData.lastProject = playerProjectFile;
        }

        setTitles();

        return true;
    }
    return false;
}

//==============================================================================
void MainContentComponent::resized()
{
    static constexpr auto MENU_BAR_HEIGHT = 20;

    JUCE_ASSERT_MESSAGE_THREAD;

    mMenuBar->setBounds(0, 0, getWidth(), MENU_BAR_HEIGHT);

    auto const availableBounds{ getLocalBounds().reduced(2).withTrimmedTop(MENU_BAR_HEIGHT) };

    // Lay out the speaker view and the vertical divider. (No divider since SpeakerView)
    Component * vComps[] = { mMainLayout.get(), nullptr };

    // Lay out side-by-side and resize the components' heights as well as widths.
    mVerticalLayout.layOutComponents(vComps,
                                     1,
                                     availableBounds.getX(),
                                     availableBounds.getY(),
                                     availableBounds.getWidth(),
                                     availableBounds.getHeight(),
                                     false,
                                     true);
}

} // namespace gris
