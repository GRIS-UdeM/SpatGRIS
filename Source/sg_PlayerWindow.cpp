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

#include "sg_PlayerWindow.hpp"
#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"

namespace gris
{
//==============================================================================
ThumbnailComp::ThumbnailComp(PlayerComponent & playerComponent,
                             GrisLookAndFeel & glaf,
                             juce::OwnedArray<juce::AudioTransportSource> & transportSources,
                             juce::AudioFormatManager & manager)
    : mPlayerComponent(playerComponent)
    , mLookAndFeel(glaf)
    , mTransportSources(transportSources)
    , mManager(manager)
{
    currentPositionMarker.setFill(mLookAndFeel.getLightColour());
    addAndMakeVisible(currentPositionMarker);
}

//==============================================================================
ThumbnailComp::~ThumbnailComp()
{
    for (auto thumbnail : mThumbnails) {
        thumbnail->removeChangeListener(this);
    }
    mThumbnails.clear();
}

//==============================================================================
void ThumbnailComp::setSources()
{
    if (mTransportSources.size() > 0) {
        auto const playerMode{ mPlayerComponent.getPlayerMode() };
        const auto & audioFiles = playerMode == PlayerMode::audio ? AudioManager::getInstance().getAudioFiles()
                                                                  : AudioManager::getInstance().getAmbiAudioFiles();
        for (int i{}; i < mThumbnails.size(); ++i) {
            mThumbnails[i]->setSource(new juce::FileInputSource(audioFiles[i]));
        }
        startTimerHz(40);
    }
}

//==============================================================================
void ThumbnailComp::paint(juce::Graphics & g)
{
    JUCE_ASSERT_MESSAGE_THREAD

    g.fillAll(mLookAndFeel.getBackgroundColour());
    g.setColour(mLookAndFeel.getLightColour());

    if (mTransportSources.size() > 0) {
        updateCursorPosition();

        if (mThumbnails[0]->getTotalLength() > 0.0) {
            auto thumbArea = getLocalBounds();

            int i{ mThumbnails.size() };
            for (auto thumbnail : mThumbnails) {
                thumbnail->drawChannels(g,
                                        thumbArea.removeFromTop(thumbArea.getHeight() / i--),
                                        0.0,
                                        thumbnail->getTotalLength(),
                                        1.0f);
            }
        }
    } else {
        g.setFont(mLookAndFeel.getFont());

        auto localBounds{ getLocalBounds().reduced(10) };
        auto textRect{ localBounds.removeFromTop(140) };

        auto sourcesRect{ localBounds.removeFromTop(20) };
        if (mPlayerComponent.getPlayerMode() == PlayerMode::audio) {
            juce::Rectangle<int> colorSourcesRect;
            colorSourcesRect.setBounds(sourcesRect.getTopLeft().getX(), sourcesRect.getTopLeft().getY(), 10, 10);
            g.setColour(mLookAndFeel.getSourceColor());
            g.fillRect(colorSourcesRect);
        }

        auto subsRect{ localBounds.removeFromTop(20) };
        juce::Rectangle<int> colorSubsRect;
        colorSubsRect.setBounds(subsRect.getTopLeft().getX(), subsRect.getTopLeft().getY(), 10, 10);
        g.setColour(mLookAndFeel.getSubColor());
        g.fillRect(colorSubsRect);

        auto unusedRect{ localBounds.removeFromTop(20) };
        if (mPlayerComponent.getPlayerMode() == PlayerMode::audio) {
            juce::Rectangle<int> colorUnusedRect;
            colorUnusedRect.setBounds(unusedRect.getTopLeft().getX(), unusedRect.getTopLeft().getY(), 10, 10);
            g.setColour(mLookAndFeel.getInactiveColor());
            g.fillRect(colorUnusedRect);
        }

        g.setColour(mLookAndFeel.getLightColour());

        juce::String text;
        juce::String sources;
        juce::String subs;
        juce::String unused;

        if (mPlayerComponent.getPlayerMode() == PlayerMode::audio) {
            text << "No audio file loaded.\n";
            text << "Open the folder containing the audio files and the speaker setup used to record them.\n";
            text << "Click in the audio thumbnails to position the playhead.\n\n";
            text << "NOTES :\n";
            text << "If a project file for the player has already been generated (with the save player project "
                    "button), it "
                    "will be loaded automatically. "
                    "Otherwise the player will try to assign direct outputs to the currently loaded speaker setup "
                    "direct outputs.\n"
                    "Please adjust this configuration to fit your needs.\n"
                 << "Source numbers will correspond to audio file numbers.\n";
        } else {
            text << "No ambisonic file loaded.\n";
            text << "Open the folder containing the \"ambisonic_files\" folder.\n";
            text << "Click in the audio thumbnails to position the playhead.\n\n";
            text << "NOTES :\n";
            text << "The player will add a source for every direct out audio file of the \"ambisonic_files\" folder. "
                    "It will try to assign them to the currently loaded speaker setup direct outputs.\n"
                    "Please adjust this configuration to fit your needs.\n";
        }
        const auto font = g.getCurrentFont();
        const int baselineY = textRect.getY() + (int)font.getAscent();
        g.drawMultiLineText(text, textRect.getX(), baselineY, textRect.getWidth(), juce::Justification::topLeft, 3.0f);
        if (mPlayerComponent.getPlayerMode() == PlayerMode::audio) {
            sources << "    Color of spatialized sources\n";
            g.drawFittedText(sources, sourcesRect, juce::Justification::topLeft, 2);
        }

        subs << "    Color of direct outputs\n";
        g.drawFittedText(subs, subsRect, juce::Justification::topLeft, 2);

        if (mPlayerComponent.getPlayerMode() == PlayerMode::audio) {
            unused << "    Color of unused sources\n";
            g.drawFittedText(unused, unusedRect, juce::Justification::topLeft, 2);
        }
    }
}

//==============================================================================
void ThumbnailComp::changeListenerCallback(juce::ChangeBroadcaster *)
{
    // repaint thumbnail when it has changed
    repaint();
}

//==============================================================================
void ThumbnailComp::updateCursorPosition()
{
    currentPositionMarker.setVisible(mTransportSources.size() > 0);

    auto r = getLocalBounds();
    auto cursorHeight = getLocalBounds().getHeight();

    currentPositionMarker.setRectangle(
        juce::Rectangle<float>((float)((mTransportSources[0]->getCurrentPosition() * r.getWidth())
                                       / mTransportSources[0]->getLengthInSeconds()),
                               0,
                               1.5f,
                               (float)cursorHeight));
}

//==============================================================================
void ThumbnailComp::setThumbnails(int numThumbnails)
{
    mThumbnails.clear();

    for (int i{}; i < numThumbnails; ++i) {
        auto thumbnail = std::make_unique<juce::AudioThumbnail>(512, mManager, mThumbnailCache);
        thumbnail->addChangeListener(this);
        mThumbnails.add(thumbnail.release());
    }

    if (mTransportSources.size() > 0) {
        setSources();
    }
}

//==============================================================================
void ThumbnailComp::timerCallback()
{
    if (mTransportSources[0]->isPlaying()) {
        updateCursorPosition();
        repaint();
        mPlayerComponent.setTimeCode(mTransportSources[0]->getCurrentPosition());
    }
    if (mTransportSources[0]->hasStreamFinished()) {
        mPlayerComponent.setTimeCode(mTransportSources[0]->getLengthInSeconds());
    }
}

//==============================================================================
void ThumbnailComp::mouseDown(const juce::MouseEvent & e)
{
    mouseDrag(e);
}

//==============================================================================
void ThumbnailComp::mouseDrag(const juce::MouseEvent & e)
{
    if (mTransportSources.size() > 0) {
        auto position
            = juce::jmax(0.0,
                         juce::jmin(e.x * mTransportSources[0]->getLengthInSeconds() / getLocalBounds().getWidth(),
                                    mTransportSources[0]->getLengthInSeconds()));
        AudioManager::getInstance().setPosition(position);

        updateCursorPosition();
        repaint();
        mPlayerComponent.setTimeCode(mTransportSources[0]->getCurrentPosition());
    }
}

//==============================================================================
int ThumbnailComp::getNumSources() const
{
    return mTransportSources.size();
}

//==============================================================================
PlayerComponent::PlayerComponent(MainContentComponent & mainContentComponent, GrisLookAndFeel & glaf)
    : mMainContentComponent(mainContentComponent)
    , mLookAndFeel(glaf)
//, mCrossOverSlider(glaf, 500.0f, 2000.0f, 1.0f, 0, 500.0f, "Hz", "Set cross-over frequency")
{
    mLoadFilesButton.setButtonText("Load audio files and Speaker setup folder");
    mSavePlayerProjectButton.setButtonText("Save Player Project");
    mPlayButton.setButtonText("Play");
    mStopButton.setButtonText("Stop");
    mLoadFilesButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSavePlayerProjectButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mPlayButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mStopButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSavePlayerProjectButton.setEnabled(false);
    mPlayButton.setEnabled(false);
    mStopButton.setEnabled(false);
    mLoadFilesButton.addListener(this);
    mSavePlayerProjectButton.addListener(this);
    mPlayButton.addListener(this);
    mStopButton.addListener(this);
    addAndMakeVisible(mLoadFilesButton);
    addAndMakeVisible(mSavePlayerProjectButton);
    addAndMakeVisible(mPlayButton);
    addAndMakeVisible(mStopButton);

    // mode
    mPlayerModeLabel.setText("Mode", juce::dontSendNotification);
    addAndMakeVisible(mPlayerModeLabel);
    mPlayerModeComboBox.setLookAndFeel(&mLookAndFeel);
    mPlayerModeComboBox.addItem("Multichannel", static_cast<int>(PlayerMode::audio));
    mPlayerModeComboBox.addItem("Ambisonic B-Format", static_cast<int>(PlayerMode::ambisonic));
    mPlayerModeComboBox.setSelectedId(static_cast<int>(PlayerMode::audio));
    mPlayerModeComboBox.onChange = [this] {
        mPlayerMode = mPlayerModeComboBox.getSelectedId() == static_cast<int>(PlayerMode::audio)
                          ? PlayerMode::audio
                          : PlayerMode::ambisonic;
        stopAudio();
        if (mPlayerMode == PlayerMode::audio) {
            mMainContentComponent.setPlayerMode(PlayerMode::audio);
            mLoadFilesButton.setButtonText("Load audio files and Speaker setup folder");
            mSavePlayerProjectButton.setButtonText("Save Player Project");
            // mSavePlayerProjectButton.setVisible(true);
            mThumbnails->setVisible(true);
            mAmbisonicThumbnails->setVisible(false);
            if (mCanReloadPlayer) {
                AudioManager::getInstance().preparePlayer(mPlayerFilesFolder, mPlayerMode);
                loadPlayer();
            }
            // mShowAmbisonicControlsButton.setVisible(false);
            mAmbiOrderLabel.setVisible(false);
            mAmbiOrderComboBox.setVisible(false);
            showSpinningWheel(false);
        } else {
            mMainContentComponent.setPlayerMode(PlayerMode::ambisonic);
            mLoadFilesButton.setButtonText("Load ambisonic files folder");
            mSavePlayerProjectButton.setButtonText("Save Ambisonic Player Project");
            // mSavePlayerProjectButton.setVisible(false);
            mThumbnails->setVisible(false);
            mAmbisonicThumbnails->setVisible(true);
            if (mCanReloadAmbisonicPlayer) {
                AudioManager::getInstance().preparePlayer(mPlayerAmbisonicFilesFolder, mPlayerMode);
                loadPlayer();
            }
            // mShowAmbisonicControlsButton.setVisible(true);
            mAmbiOrderLabel.setVisible(true);
            mAmbiOrderComboBox.setVisible(true);
        }
        resized();
        repaint();
    };
    addAndMakeVisible(mPlayerModeComboBox);

    mAmbiOrderLabel.setText("Order", juce::dontSendNotification);
    addAndMakeVisible(mAmbiOrderLabel);
    mAmbiOrderLabel.setVisible(false);
    mAmbiOrderComboBox.setLookAndFeel(&mLookAndFeel);
    for (int i{ 1 }; i <= 10; ++i) {
        mAmbiOrderComboBox.addItem(juce::String(i), i);
    }
    mAmbiOrderComboBox.setSelectedId(1, juce::dontSendNotification);
    mAmbiOrderComboBox.onChange = [this] {
        const auto order{ mAmbiOrderComboBox.getSelectedId() };
        auto & ambiDecSpatAlgorithm{ mMainContentComponent.getAudioProcessor().getAmbiDecSpatAlgorithm() };
        if (ambiDecSpatAlgorithm != nullptr) {
            static_cast<AmbiDecSpatAlgorithm *>(ambiDecSpatAlgorithm.get())->setOrder(order);
            mIsLoadingAmbisonicOrder = true;
            showSpinningWheel(true);
        }
    };
    addAndMakeVisible(mAmbiOrderComboBox);
    mAmbiOrderComboBox.setVisible(false);

    addAndMakeVisible(mSpinningWheel);
    mSpinningWheel.setVisible(false);
    mSpinningWheel.setColour(juce::ProgressBar::ColourIds::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mSpinningWheel.setColour(juce::ProgressBar::ColourIds::foregroundColourId, mLookAndFeel.getOnColour());

    //// Ambi Controls
    // mShowAmbisonicControlsButton.setButtonText("Show Controls");
    // mShowAmbisonicControlsButton.onClick = [this] { resized(); };
    // addAndMakeVisible(mShowAmbisonicControlsButton);
    // mShowAmbisonicControlsButton.setVisible(false);

    // mLowFreqDecoderLabel.setText("Low Frequency Decoder", juce::dontSendNotification);
    // mHighFreqDecoderLabel.setText("High Frequency Decoder", juce::dontSendNotification);
    // mCrossoverLabel.setText("Cross-over Frequency", juce::dontSendNotification);
    // mLowFreqMethodLabel.setText("Method", juce::dontSendNotification);
    // mHighFreqMethodLabel.setText("Method", juce::dontSendNotification);
    // mLowFreqNormFactorLabel.setText("Normalisation Factor", juce::dontSendNotification);
    // mHighFreqNormFactorLabel.setText("Normalisation Factor", juce::dontSendNotification);

    // mLowFreqMethodComboBox.setLookAndFeel(&mLookAndFeel);
    ////mLowFreqMethodComboBox.addItem("SAD", 1);
    ////mLowFreqMethodComboBox.addItem("MMD", 2);
    ////mLowFreqMethodComboBox.addItem("EPAD", 3);
    ////mLowFreqMethodComboBox.addItem("AllRAD", 4);
    // mLowFreqMethodComboBox.addItemList(mAmbiDecMethods, 1);
    // mLowFreqMethodComboBox.setSelectedId(4);

    // mHighFreqMethodComboBox;
    // mLowFreqNormFactorComboBox;
    // mHighFreqNormFactorComboBox;

    // addAndMakeVisible(mCrossOverSlider);

    // timecode
    addAndMakeVisible(mTimeCodeLabel);
    mTimeCodeLabel.setName("TimeCode_label");
    mTimeCodeLabel.setText("00 : 00 . 000", juce::NotificationType::dontSendNotification);
    mTimeCodeLabel.setColour(juce::Label::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mTimeCodeLabel.setColour(juce::Label::textColourId, mLookAndFeel.getLightColour());
    mTimeCodeLabel.setJustificationType(juce::Justification::centred);

    // thumbnails
    mThumbnails.reset(new ThumbnailComp(*this,
                                        mLookAndFeel,
                                        AudioManager::getInstance().getTransportSources(),
                                        AudioManager::getInstance().getAudioFormatManager()));
    addAndMakeVisible(mThumbnails.get());
    mAmbisonicThumbnails.reset(new ThumbnailComp(*this,
                                                 mLookAndFeel,
                                                 AudioManager::getInstance().getAmbiTransportSources(),
                                                 AudioManager::getInstance().getAmbiAudioFormatManager()));
    addAndMakeVisible(mAmbisonicThumbnails.get());
}

//==============================================================================
PlayerComponent::~PlayerComponent()
{
    if (mThumbnails->getNumSources() > 0 || mAmbisonicThumbnails->getNumSources() > 0) {
        stopAudio();
    }
    mLoadFilesButton.removeListener(this);
    mSavePlayerProjectButton.removeListener(this);
    mPlayButton.removeListener(this);
    mStopButton.removeListener(this);

    auto & audioManager = AudioManager::getInstance();
    auto & transportSources{ audioManager.getTransportSources() };
    auto & ambiTransportSources{ audioManager.getAmbiTransportSources() };
    if (transportSources.size() > 0) {
        transportSources[0]->removeChangeListener(this);
    }
    if (ambiTransportSources.size() > 0) {
        ambiTransportSources[0]->removeChangeListener(this);
    }

    audioManager.unloadPlayer();
    mMainContentComponent.setPlayerMode(PlayerMode::none);
}

//==============================================================================
void PlayerComponent::handleOpenFiles()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto const displayError = [&](juce::String const & message) {
        auto title = mPlayerMode == PlayerMode::audio
                         ? juce::String("Unable to open Speaker Setup and Audio Files folder")
                         : juce::String("Unable to open Ambisonic Files folder");
        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, title, message);
    };

    juce::File const filesFolder;
    juce::FileChooser fc{ "Choose the ambisonic_files folder...", filesFolder, {}, true };

    if (!fc.browseForDirectory()) {
        return;
    }
    auto const chosen{ fc.getResult() };

    if (mPlayerMode == PlayerMode::audio) {
        if (validateAudioFilesAndSpeakerSetup(chosen)) {
            if (AudioManager::getInstance().preparePlayer(chosen, mPlayerMode)) {
                mPlayerFilesFolder = chosen;
                loadPlayer();
            } else {
                displayError("Audio files do not have the same length.");
            }
        }
    } else {
        if (validateAmbisonicFiles(chosen)) {
            if (AudioManager::getInstance().preparePlayer(chosen, mPlayerMode)) {
                mPlayerAmbisonicFilesFolder = chosen;
                loadPlayer();
            } else {
                displayError("Ambisonic files do not have the same length.");
            }
        }
    }
}

//==============================================================================
bool PlayerComponent::validateAudioFilesAndSpeakerSetup(juce::File const & folder)
{
    juce::StringArray audioFileList;
    juce::StringArray speakerList;
    juce::StringArray audioFileExtensions{ ".wav", ".aif", ".aiff" };
    juce::XmlElement xml("tmp");

    auto const displayError = [&](juce::String const & message) {
        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                    "Unable to open Speaker Setup and Audio Files folder",
                                                    message);
    };

    bool foundSpeakerSetup{};
    for (const auto & filenameThatWasFound :
         folder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*")) {
        jassert(filenameThatWasFound.existsAsFile());

        if (audioFileExtensions.contains(filenameThatWasFound.getFileExtension())) {
            audioFileList.add(filenameThatWasFound.getFileNameWithoutExtension().substring(
                filenameThatWasFound.getFileNameWithoutExtension().lastIndexOfChar('-') + 1));

        } else if (filenameThatWasFound.getFileExtension() == ".xml" && !foundSpeakerSetup) {
            mPlayerSpeakerSetup
                = mMainContentComponent.playerExtractSpeakerSetup(juce::File(filenameThatWasFound.getFullPathName()));

            if (!mPlayerSpeakerSetup) {
                continue;
            }

            juce::XmlDocument xmlDoc(juce::File(filenameThatWasFound.getFullPathName()));
            xml = *xmlDoc.getDocumentElement();

            foundSpeakerSetup = true;
        }
    }

    if (!foundSpeakerSetup) {
        displayError("No Speaker Setup file found.");
        return false;
    }

    if (audioFileList.isEmpty()) {
        displayError("No audio file found.");
        return false;
    }

    for (auto const * speaker : xml.getChildIterator()) {
        auto const tagName{ speaker->getTagName() };
        if (tagName.startsWith(SpeakerData::XmlTags::MAIN_TAG_PREFIX)) {
            speakerList.add(tagName.substring(juce::String(SpeakerData::XmlTags::MAIN_TAG_PREFIX).length()));
        }
    }

    if (speakerList.size() != audioFileList.size()) {
        displayError("Audio file list does not match Speaker Setup data.\n" + juce::String(audioFileList.size())
                     + " audio files.\n" + juce::String(speakerList.size()) + " speakers.");
        return false;
    }

    // for (auto const elem : audioFileList) {
    //    if (!speakerList.contains(elem)) {
    //        displayError("Audio file list does not match Speaker Setup data.\nAudio file #" + elem
    //                     + " not found in Speaker Setup file.");
    //        return false;
    //    }
    //}

    for (auto const & elem : speakerList) {
        if (!audioFileList.contains(elem)) {
            displayError("Audio file list does not match Speaker Setup data.\nMissing audio file #" + elem + ".");
            return false;
        }
    }

    return true;
}

//==============================================================================
bool PlayerComponent::validateAmbisonicFiles(juce::File const & folder)
{
    juce::StringArray ambisonicFileList;
    juce::StringArray ambisonicDirectsFileList;
    juce::StringArray ambisonicFileExtensions{ ".wav", ".aif", ".aiff" };

    auto const displayError = [&](juce::String const & message) {
        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                    "Unable to open Ambisonic Files folder",
                                                    message);
    };

    if (folder.getFileName().compare("ambisonic_files") != 0) {
        auto const msg{ juce::String("Ambisonic files folder must be named \"ambisonic_files\".") };
        displayError(msg);
        return false;
    }

    for (const auto & filenameThatWasFound :
         folder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*")) {
        jassert(filenameThatWasFound.existsAsFile());

        if (ambisonicFileExtensions.contains(filenameThatWasFound.getFileExtension())) {
            if (filenameThatWasFound.getFileNameWithoutExtension().contains("-direct-")) {
                ambisonicDirectsFileList.add(filenameThatWasFound.getFileNameWithoutExtension().substring(
                    filenameThatWasFound.getFileNameWithoutExtension().lastIndexOfChar('-') + 1));
            } else {
                ambisonicFileList.add(filenameThatWasFound.getFileNameWithoutExtension().substring(
                    filenameThatWasFound.getFileNameWithoutExtension().lastIndexOfChar('-') + 1));
            }
        }
    }

    if (ambisonicFileList.isEmpty()) {
        displayError("No ambisonic file found.");
        return false;
    }
    mNumAmbiFiles = ambisonicFileList.size();
    mNumAmbiDirectFiles = ambisonicDirectsFileList.size();

    return true;
}

//==============================================================================
void PlayerComponent::setAmbisonicDecoderReadyToProcess(int state)
{
    switch (state) {
    case 0:
        // done loading ambisonic order
        mIsAmbisonicDecoderReadyToProcess = true;
        mIsLoadingAmbisonicOrder = false;
        showSpinningWheel(false);
        showCheckSign();
        break;
    case 1:
        // still loading ambisonic order
        break;
    default:
        return;
    }
}

//==============================================================================
void PlayerComponent::playAudio()
{
    // juce::ScopedReadLock const lock{ mLock };
    if ((mPlayerMode == PlayerMode::ambisonic && mIsAmbisonicDecoderReadyToProcess)
        || mPlayerMode == PlayerMode::audio) {
        auto & audioManager{ AudioManager::getInstance() };
        audioManager.startPlaying();
    }
}

//==============================================================================
void PlayerComponent::stopAudio()
{
    // juce::ScopedReadLock const lock{ mLock };
    auto & audioManager{ AudioManager::getInstance() };
    audioManager.stopPlaying();
}

//==============================================================================
void PlayerComponent::loadPlayer(bool handleNewProject)
{
    AudioManager::getInstance().setPlayerLoading(true);

    if (mPlayerMode == PlayerMode::audio) {
        if (handleNewProject) {
            mMainContentComponent.handleNewProjectForPlayer();
        }

        mMainContentComponent.handlePlayerSourcesPositions(mPlayerSpeakerSetup, mPlayerFilesFolder);
        mThumbnails->setThumbnails(mPlayerSpeakerSetup->speakers.size());

        mSavePlayerProjectButton.setEnabled(true);
        mPlayButton.setEnabled(true);
        mStopButton.setEnabled(true);

        // register listener (only the first AudioTransportSource)
        auto & transportSources{ AudioManager::getInstance().getTransportSources() };
        transportSources[0]->addChangeListener(this);

        mThumbnails->updateCursorPosition();
        setTimeCode(0.0);
        mCanReloadPlayer = true;
    } else {
        mIsAmbisonicDecoderReadyToProcess = false;

        if (handleNewProject) {
            mMainContentComponent.handleNewProjectForPlayer();
        }

        mMainContentComponent.handleAmbisonicPlayerSourcesPositions(mPlayerAmbisonicFilesFolder);
        mAmbisonicThumbnails->setThumbnails(mNumAmbiFiles + mNumAmbiDirectFiles);

        mSavePlayerProjectButton.setEnabled(true);
        mPlayButton.setEnabled(true);
        mStopButton.setEnabled(true);

        // register listener (only the first AudioTransportSource)
        auto & ambiTransportSources{ AudioManager::getInstance().getAmbiTransportSources() };
        ambiTransportSources[0]->addChangeListener(this);

        mAmbisonicThumbnails->updateCursorPosition();
        setTimeCode(0.0);
        mCanReloadAmbisonicPlayer = true;

        auto ambisonicOrder{ static_cast<int>(std::sqrt(mNumAmbiFiles)) - 1 };
        jassert((ambisonicOrder + 1) * (ambisonicOrder + 1) == mNumAmbiFiles);
        mAmbiOrderComboBox.setSelectedId(ambisonicOrder, juce::sendNotification);
    }

    AudioManager::getInstance().setPlayerLoading(false);
}

//==============================================================================
void PlayerComponent::showSpinningWheel(bool showSpinningWheel)
{
    mSpinningWheel.setVisible(showSpinningWheel);
}

//==============================================================================
void PlayerComponent::showCheckSign()
{
    mShowSucceedCheck = true;
    startTimer(2000);
    callAfterDelay(2000, [this]() {
        stopTimer();
        mShowSucceedCheck = false;
        repaint();
    });
}

//==============================================================================
void PlayerComponent::paint(juce::Graphics & g)
{
    g.fillAll(mLookAndFeel.getWinBackgroundColour());

    if (mShowSucceedCheck) {
        g.setColour(mLookAndFeel.getOnColour());
        g.drawText(juce::String::charToString(0x2713),
                   mSpinningWheel.getBounds().getTopLeft().getX() + 3,
                   mSpinningWheel.getBounds().getTopLeft().getY(),
                   14,
                   14,
                   juce::Justification::left);
    }
}

//==============================================================================
void PlayerComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    // Stop audio when file has ended.
    auto & transportSources{ AudioManager::getInstance().getTransportSources() };
    auto & ambiTransportSources{ AudioManager::getInstance().getAmbiTransportSources() };

    if (mPlayerMode == PlayerMode::audio) {
        if (transportSources.size() != 0) {
            if (source == transportSources.getUnchecked(0)) {
                if (transportSources.getUnchecked(0)->hasStreamFinished()) {
                    stopAudio();
                }
            }
        }
    } else {
        if (ambiTransportSources.size() != 0) {
            if (source == ambiTransportSources.getUnchecked(0)) {
                if (ambiTransportSources.getUnchecked(0)->hasStreamFinished()) {
                    stopAudio();
                }
            }
        }
    }
}

//==============================================================================
void PlayerComponent::timerCallback()
{
    if (mMakeButtonBlink) {
        if (mBlinkNTimes > 0) {
            if (mBlinkNTimes-- % 2 == 0) {
                mSavePlayerProjectButton.setButtonText("");
            } else {
                mSavePlayerProjectButton.setButtonText("File saved !");
            }
        } else {
            stopTimer();
            mBlinkNTimes = 5;
            mMakeButtonBlink = false;
            if (mPlayerMode == PlayerMode::audio) {
                mSavePlayerProjectButton.setButtonText("Save Player Project");
            } else {
                mSavePlayerProjectButton.setButtonText("Save Ambisonic Player Project");
            }
        }
    }
}

//==============================================================================
void PlayerComponent::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (button == &mLoadFilesButton) {
        handleOpenFiles();
        if (mPlayerMode == PlayerMode::audio) {
            mThumbnails->setSources();
        } else {
            mIsAmbisonicDecoderReadyToProcess = false;
            mAmbisonicThumbnails->setSources();
        }
    } else if (button == &mSavePlayerProjectButton) {
        auto playerProjectSaved{ mPlayerMode == PlayerMode::audio
                                     ? mMainContentComponent.savePlayerProject(mPlayerFilesFolder)
                                     : mMainContentComponent.savePlayerProject(mPlayerAmbisonicFilesFolder) };
        if (playerProjectSaved) {
            mMakeButtonBlink = true;
            startTimer(400);
        }
    } else if (button == &mPlayButton) {
        playAudio();
    } else if (button == &mStopButton) {
        stopAudio();
        if (mPlayerMode == PlayerMode::audio) {
            mThumbnails->updateCursorPosition();
            setTimeCode(AudioManager::getInstance().getTransportSources().getFirst()->getCurrentPosition());
        } else {
            mAmbisonicThumbnails->updateCursorPosition();
            setTimeCode(AudioManager::getInstance().getAmbiTransportSources().getFirst()->getCurrentPosition());
        }
    }
}

//==============================================================================
void PlayerComponent::resized()
{
    auto r = getLocalBounds().reduced(4);
    auto playerModeArea = r.removeFromTop(20);
    r.removeFromTop(4);
    // auto ambiControls = mPlayerMode == PlayerMode::ambisonic && mShowAmbisonicControlsButton.getToggleState()
    //                         ? r.removeFromTop(100)
    //                         : juce::Rectangle<int>();
    auto controls = r.removeFromBottom(40);
    r.removeFromBottom(4);

    mPlayerModeLabel.setBounds(playerModeArea.removeFromLeft(45));
    mPlayerModeComboBox.setBounds(playerModeArea.removeFromLeft(150));
    mAmbiOrderLabel.setBounds(playerModeArea.removeFromLeft(45));
    mAmbiOrderComboBox.setBounds(playerModeArea.removeFromLeft(50));
    mSpinningWheel.setBounds(mAmbiOrderComboBox.getBounds().getRight() + 4,
                             mAmbiOrderComboBox.getBounds().getY() + 4,
                             14,
                             14);
    // mShowAmbisonicControlsButton.setBounds(playerModeArea.removeFromLeft(120));
    // if (mPlayerMode == PlayerMode::ambisonic) {
    //     mCrossOverSlider.setBounds(ambiControls.removeFromTop(30));
    // }
    mThumbnails->setBounds(r);
    mAmbisonicThumbnails->setBounds(r);
    if (mPlayerMode == PlayerMode::audio) {
        mLoadFilesButton.setBounds(controls.removeFromLeft(controls.getWidth() / 3));
        mSavePlayerProjectButton.setBounds(controls.removeFromLeft(controls.getWidth() / 4));
        mSavePlayerProjectButton.setVisible(true);
    } else {
        mLoadFilesButton.setBounds(controls.removeFromLeft(controls.getWidth() / 2));
        // mSavePlayerProjectButton.setBounds(controls.removeFromLeft(controls.getWidth() / 4));
        mSavePlayerProjectButton.setVisible(false);
    }
    mTimeCodeLabel.setBounds(controls.removeFromLeft(controls.getWidth() / 2).reduced(1));
    mStopButton.setBounds(controls.removeFromRight(controls.getWidth() / 2));
    mPlayButton.setBounds(controls);
}

//==============================================================================
double PlayerComponent::getTimeCode() const
{
    auto timeCodeStr = mTimeCodeLabel.getText().removeCharacters(" ");
    auto millisecs = timeCodeStr.getLastCharacters(3).getDoubleValue();
    timeCodeStr = timeCodeStr.dropLastCharacters(4);
    auto secs = timeCodeStr.getLastCharacters(2).getDoubleValue();
    timeCodeStr = timeCodeStr.dropLastCharacters(3);
    auto minutes = timeCodeStr.getLastCharacters(2).getDoubleValue();
    timeCodeStr = timeCodeStr.dropLastCharacters(3);
    double hours{};
    if (timeCodeStr.isNotEmpty()) {
        hours = timeCodeStr.getLastCharacters(2).getDoubleValue();
    }

    auto resInSecs = hours * 3600 + minutes * 60 + secs + millisecs * 0.001;

    return resInSecs;
}

//==============================================================================
PlayerMode PlayerComponent::getPlayerMode() const
{
    return mPlayerMode;
}

//==============================================================================
void PlayerComponent::setTimeCode(double const timeInSec)
{
    juce::uint64 totalMillisecs = (juce::uint64)(timeInSec * 1000);
    juce::String hours{ totalMillisecs / 3600000 };
    juce::String minutes{ (totalMillisecs / 60000) % 60 };
    juce::String secs{ (totalMillisecs / 1000) % 60 };
    juce::String formattedMillis{ totalMillisecs % 1000 };

    if (hours == "0") {
        mTimeCodeLabel.setText(minutes.paddedLeft('0', 2) + " : " + secs.paddedLeft('0', 2) + " . "
                                   + formattedMillis.paddedRight('0', 3),
                               juce::NotificationType::dontSendNotification);
    } else {
        mTimeCodeLabel.setText(hours + " : " + minutes.paddedLeft('0', 2) + " : " + secs.paddedLeft('0', 2) + " . "
                                   + formattedMillis.paddedRight('0', 3),
                               juce::NotificationType::dontSendNotification);
    }
}

//==============================================================================
PlayerWindow::PlayerWindow(MainContentComponent & mainContentComponent, GrisLookAndFeel & glaf)
    : DocumentWindow("SpatGRIS Player", glaf.getBackgroundColour(), allButtons)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(glaf)
    , mPlayerComponent(mainContentComponent, glaf)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    setResizable(true, true);
    setResizeLimits(600,
                    330,
                    juce::Desktop::getInstance().getDisplays().getTotalBounds(true).getWidth(),
                    juce::Desktop::getInstance().getDisplays().getTotalBounds(true).getHeight());
    setUsingNativeTitleBar(true);
    // setAlwaysOnTop(true);
    setContentNonOwned(&mPlayerComponent, false);
    centreWithSize(800, 400);
    DocumentWindow::setVisible(true);
}

//==============================================================================
void PlayerWindow::closeButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mPlayerComponent.stopAudio();
    mMainContentComponent.closePlayerWindow();
}

//==============================================================================
bool PlayerWindow::keyPressed(const juce::KeyPress & k)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const key_w{ juce::KeyPress(87) };

    if (k == juce::KeyPress::spaceKey) {
        auto & audioManager{ AudioManager::getInstance() };
        if ((mPlayerComponent.getPlayerMode() == PlayerMode::audio && audioManager.getTransportSources().size() > 0)
            || (mPlayerComponent.getPlayerMode() == PlayerMode::ambisonic
                && audioManager.getAmbiTransportSources().size() > 0)) {
            audioManager.isPlaying() ? mPlayerComponent.stopAudio() : mPlayerComponent.playAudio();
            return true;
        }
    } else if (k.getModifiers().isCommandDown() && k.isKeyCurrentlyDown(key_w.getKeyCode())) {
        mPlayerComponent.stopAudio();
        mMainContentComponent.closePlayerWindow();
        return true;
    }

    return false;
}

//==============================================================================
void PlayerWindow::stopAudio()
{
    mPlayerComponent.stopAudio();
}

//==============================================================================
void PlayerWindow::reloadPlayer()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mPlayerComponent.loadPlayer(false);
    auto * currentAudioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
    AudioManager::getInstance().reloadPlayerAudioFiles(currentAudioDevice->getCurrentBufferSizeSamples(),
                                                       currentAudioDevice->getCurrentSampleRate());
}

//==============================================================================
void PlayerWindow::setAmbisonicDecoderReadyToProcess(int state)
{
    mPlayerComponent.setAmbisonicDecoderReadyToProcess(state);
}

} // namespace gris
