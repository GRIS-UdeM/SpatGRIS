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

#pragma once

#include "sg_LogicStrucs.hpp"
#include "sg_Macros.hpp"
#include "sg_constants.hpp"

#include <JuceHeader.h>

namespace gris
{
class MainContentComponent;
class GrisLookAndFeel;
class ThumbnailComp;

//==============================================================================
class PlayerComponent final
    : public juce::Component
    , private juce::TextButton::Listener
    , private juce::ChangeListener
{
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;

    juce::ReadWriteLock mLock{};

    juce::TextButton mLoadWavFilesAndSpeakerSetupButton{};
    juce::TextButton mPlayButton{};
    juce::TextButton mStopButton{};
    juce::Label mTimeCodeLabel{};

    std::unique_ptr<ThumbnailComp> mThumbnails;
    tl::optional<SpeakerSetup> mPlayerSpeakerSetup;

public:
    //==============================================================================
    PlayerComponent() = delete;
    explicit PlayerComponent(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel);
    ~PlayerComponent() override;
    SG_DELETE_COPY_AND_MOVE(PlayerComponent)
    //==============================================================================
    void buttonClicked(juce::Button * button) override;
    void resized() override;
    //==============================================================================
    double getTimeCode() const;
    //==============================================================================
    void setTimeCode(double const timeInSec);
    void playAudio();
    void stopAudio();
    void loadPlayer();

private:
    //==============================================================================
    void handleOpenWavFilesAndSpeakerSetup();
    bool validateWavFilesAndSpeakerSetup(juce::File const & folder);
    //==============================================================================
    void paint(juce::Graphics & g) override;
    void changeListenerCallback(juce::ChangeBroadcaster * source) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(PlayerComponent)
}; // class PlayerComponent

//==============================================================================
class ThumbnailComp
    : public juce::Component
    , public juce::ChangeListener
    , private juce::Timer
{
    juce::CriticalSection mLock{};

    PlayerComponent & mPlayerComponent;
    GrisLookAndFeel & mLookAndFeel;

    juce::OwnedArray<juce::AudioTransportSource> & mTransportSources;
    juce::AudioFormatManager & mManager;
    juce::AudioThumbnailCache mThumbnailCache{ MAX_NUM_SOURCES };
    juce::OwnedArray<juce::AudioThumbnail> mThumbnails;

    juce::DrawableRectangle currentPositionMarker{};

public:
    //==============================================================================
    ThumbnailComp(PlayerComponent & playerComponent,
                  GrisLookAndFeel & lookAndFeel,
                  juce::OwnedArray<juce::AudioTransportSource> & transportSources,
                  juce::AudioFormatManager & manager);
    ThumbnailComp() = delete;
    ~ThumbnailComp() override;
    SG_DELETE_COPY_AND_MOVE(ThumbnailComp)

    //==============================================================================
    void setSources();
    void updateCursorPosition();
    void addThumbnails(int numThumbnails);

    //==============================================================================
    int getNumSources() const;

    //==============================================================================
    void mouseDown(const juce::MouseEvent & e) override;
    void mouseDrag(const juce::MouseEvent & e) override;

private:
    //==============================================================================
    void paint(juce::Graphics & g) override;
    void changeListenerCallback(juce::ChangeBroadcaster * source) override;
    void timerCallback() override;

    JUCE_LEAK_DETECTOR(ThumbnailComp)
}; // class ThumbnailComp

//==============================================================================
class PlayerWindow final : public juce::DocumentWindow
{
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;
    PlayerComponent mPlayerComponent;

public:
    //==============================================================================
    PlayerWindow(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel);
    PlayerWindow() = delete;
    ~PlayerWindow() override = default;
    SG_DELETE_COPY_AND_MOVE(PlayerWindow)
    //==============================================================================
    void closeButtonPressed() override;
    bool keyPressed(const juce::KeyPress & k) override;
    //==============================================================================
    void reloadPlayer();

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(PlayerWindow)
}; // class PlayerWindow
} // namespace gris