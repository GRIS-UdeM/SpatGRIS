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
                             GrisLookAndFeel & lookAndFeel,
                             juce::OwnedArray<juce::AudioTransportSource> & transportSources,
                             juce::AudioFormatManager & manager)
    : mPlayerComponent(playerComponent)
    , mLookAndFeel(lookAndFeel)
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
        for (int i{}; i < mThumbnails.size(); ++i) {
            mThumbnails[i]->setSource(new juce::FileInputSource(AudioManager::getInstance().getAudioFiles()[i]));
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
        auto textRect{ localBounds.removeFromTop(160) };

        auto sourcesRect{ localBounds.removeFromTop(20) };
        juce::Rectangle<int> colorSourcesRect;
        colorSourcesRect.setBounds(sourcesRect.getTopLeft().getX(), sourcesRect.getTopLeft().getY(), 10, 10);
        g.setColour(mLookAndFeel.getSourceColor());
        g.fillRect(colorSourcesRect);

        auto subsRect{ localBounds.removeFromTop(20) };
        juce::Rectangle<int> colorSubsRect;
        colorSubsRect.setBounds(subsRect.getTopLeft().getX(), subsRect.getTopLeft().getY(), 10, 10);
        g.setColour(mLookAndFeel.getSubColor());
        g.fillRect(colorSubsRect);

        auto unusedRect{ localBounds.removeFromTop(20) };
        juce::Rectangle<int> colorUnusedRect;
        colorUnusedRect.setBounds(unusedRect.getTopLeft().getX(), unusedRect.getTopLeft().getY(), 10, 10);
        g.setColour(mLookAndFeel.getInactiveColor());
        g.fillRect(colorUnusedRect);

        g.setColour(mLookAndFeel.getLightColour());

        juce::String text;
        juce::String sources;
        juce::String subs;
        juce::String unused;
        text << "No audio file loaded.\n";
        text << "Open the folder containing the audio files and the speaker setup used to record them.\n";
        text << "Click in the audio thumbnails to position the playhead.\n\n";
        text << "NOTES :\n";
        text << "If a project file for the player has already been generated (with the save player project button), it "
                "will be loaded automatically. "
                "Otherwise the player will try to assign direct outputs to the currently loaded speaker setup direct "
                "outputs. "
                "Please adjust this configuration to fit your needs.\n"
             << "Source numbers will correspond to audio file numbers. If there is a gap in numbering, unused sources "
                "will be muted.\n";
        g.drawFittedText(text, textRect, juce::Justification::topLeft, 2);

        sources << "    Color of spatialized sources\n";
        g.drawFittedText(sources, sourcesRect, juce::Justification::topLeft, 2);

        subs << "    Color of direct outputs\n";
        g.drawFittedText(subs, subsRect, juce::Justification::topLeft, 2);

        unused << "    Color of unused sources\n";
        g.drawFittedText(unused, unusedRect, juce::Justification::topLeft, 2);
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
void ThumbnailComp::addThumbnails(int numThumbnails)
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
PlayerComponent::PlayerComponent(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel)
    : mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
{
    mLoadWavFilesAndSpeakerSetupButton.setButtonText("Load audio files and Speaker setup folder");
    mSavePlayerProjectButton.setButtonText("Save Player Project");
    mPlayButton.setButtonText("Play");
    mStopButton.setButtonText("Stop");
    mLoadWavFilesAndSpeakerSetupButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSavePlayerProjectButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mPlayButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mStopButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSavePlayerProjectButton.setEnabled(false);
    mPlayButton.setEnabled(false);
    mStopButton.setEnabled(false);
    mLoadWavFilesAndSpeakerSetupButton.addListener(this);
    mSavePlayerProjectButton.addListener(this);
    mPlayButton.addListener(this);
    mStopButton.addListener(this);
    addAndMakeVisible(mLoadWavFilesAndSpeakerSetupButton);
    addAndMakeVisible(mSavePlayerProjectButton);
    addAndMakeVisible(mPlayButton);
    addAndMakeVisible(mStopButton);

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
}

//==============================================================================
PlayerComponent::~PlayerComponent()
{
    if (mThumbnails->getNumSources() > 0) {
        stopAudio();
    }
    mLoadWavFilesAndSpeakerSetupButton.removeListener(this);
    mSavePlayerProjectButton.removeListener(this);
    mPlayButton.removeListener(this);
    mStopButton.removeListener(this);

    auto & audioManager = AudioManager::getInstance();
    auto & transportSources{ audioManager.getTransportSources() };
    if (transportSources.size() > 0) {
        transportSources[0]->removeChangeListener(this);
    }

    audioManager.unloadPlayer();
}

//==============================================================================
void PlayerComponent::handleOpenWavFilesAndSpeakerSetup()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    auto const displayError = [&](juce::String const & message) {
        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                    "Unable to open Speaker Setup and Audio Files folder",
                                                    message);
    };

    juce::File const wavFilesAndSSFolder;
    juce::FileChooser fc{ "Choose a folder to open...", wavFilesAndSSFolder, {}, true };

    if (!fc.browseForDirectory()) {
        return;
    }
    auto const chosen{ fc.getResult() };

    if (validateWavFilesAndSpeakerSetup(chosen)) {
        if (AudioManager::getInstance().prepareAudioPlayer(chosen)) {
            mPlayerFilesFolder = chosen;
            loadPlayer();
        } else {
            displayError("Audio files do not have the same length.");
        }
    }
}

//==============================================================================
bool PlayerComponent::validateWavFilesAndSpeakerSetup(juce::File const & folder)
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
void PlayerComponent::playAudio()
{
    // juce::ScopedReadLock const lock{ mLock };
    auto & audioManager{ AudioManager::getInstance() };
    audioManager.startPlaying();
}

//==============================================================================
void PlayerComponent::stopAudio()
{
    // juce::ScopedReadLock const lock{ mLock };
    auto & audioManager{ AudioManager::getInstance() };
    audioManager.stopPlaying();
}

//==============================================================================
void PlayerComponent::loadPlayer()
{
    mMainContentComponent.handleNewProjectForPlayer();
    mMainContentComponent.handlePlayerSourcesPositions(mPlayerSpeakerSetup, mPlayerFilesFolder);
    mThumbnails->addThumbnails(mPlayerSpeakerSetup->speakers.size());

    mSavePlayerProjectButton.setEnabled(true);
    mPlayButton.setEnabled(true);
    mStopButton.setEnabled(true);

    // register listener (only the first AudioTransportSource)
    auto & transportSources{ AudioManager::getInstance().getTransportSources() };
    transportSources[0]->addChangeListener(this);

    mThumbnails->updateCursorPosition();
    setTimeCode(0.0);
}

//==============================================================================
void PlayerComponent::paint(juce::Graphics & g)
{
    g.fillAll(mLookAndFeel.getWinBackgroundColour());
}

//==============================================================================
void PlayerComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    // Stop audio when file has ended.
    auto & transportSources{ AudioManager::getInstance().getTransportSources() };
    if (source == transportSources.getUnchecked(0)) {
        if (transportSources.getUnchecked(0)->hasStreamFinished()) {
            stopAudio();
        }
    }
}

//==============================================================================
void PlayerComponent::timerCallback()
{
    if (mBlinkNTimes > 0) {
        if (mBlinkNTimes-- % 2 == 0) {
            mSavePlayerProjectButton.setButtonText("");
        } else {
            mSavePlayerProjectButton.setButtonText("File saved !");
        }
    } else {
        stopTimer();
        mBlinkNTimes = 5;
        mSavePlayerProjectButton.setButtonText("Save Player Project");
    }
}

//==============================================================================
void PlayerComponent::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (button == &mLoadWavFilesAndSpeakerSetupButton) {
        handleOpenWavFilesAndSpeakerSetup();
        mThumbnails->setSources();
    } else if (button == &mSavePlayerProjectButton) {
        auto playerProjectSaved{ mMainContentComponent.savePlayerProject(mPlayerFilesFolder) };
        if (playerProjectSaved) {
            startTimer(400);
        }
    } else if (button == &mPlayButton) {
        playAudio();
    } else if (button == &mStopButton) {
        stopAudio();
        mThumbnails->updateCursorPosition();
        setTimeCode(AudioManager::getInstance().getTransportSources().getFirst()->getCurrentPosition());
    }
}

//==============================================================================
void PlayerComponent::resized()
{
    auto r = getLocalBounds().reduced(4);
    auto controls = r.removeFromBottom(40);
    r.removeFromBottom(4);

    mThumbnails->setBounds(r);
    mLoadWavFilesAndSpeakerSetupButton.setBounds(controls.removeFromLeft(controls.getWidth() / 3));
    mSavePlayerProjectButton.setBounds(controls.removeFromLeft(controls.getWidth() / 4));
    mTimeCodeLabel.setBounds(controls.removeFromLeft(controls.getWidth() / 2).reduced(1));
    mStopButton.setBounds(controls.removeFromRight(controls.getWidth() / 2));
    mPlayButton.setBounds(controls);
}

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
PlayerWindow::PlayerWindow(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel)
    : DocumentWindow("SpatGRIS Player", lookAndFeel.getBackgroundColour(), allButtons)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
    , mPlayerComponent(mainContentComponent, lookAndFeel)
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

    mMainContentComponent.closePlayerWindow();
}

//==============================================================================
bool PlayerWindow::keyPressed(const juce::KeyPress & k)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (k == juce::KeyPress::spaceKey) {
        auto & audioManager{ AudioManager::getInstance() };
        if (audioManager.getTransportSources().size() > 0) {
            audioManager.isPlaying() ? mPlayerComponent.stopAudio() : mPlayerComponent.playAudio();
            return true;
        }
    }
    return false;
}

//==============================================================================
void PlayerWindow::reloadPlayer()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mPlayerComponent.loadPlayer();
    auto * currentAudioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
    AudioManager::getInstance().reloadPlayerAudioFiles(currentAudioDevice->getCurrentBufferSizeSamples(),
                                                       currentAudioDevice->getCurrentSampleRate());
}

} // namespace gris
