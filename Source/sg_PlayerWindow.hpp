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

//#include "sg_LayoutComponent.hpp"
//#include "sg_Player.hpp"
//#include "sg_SpatButton.hpp"
//#include "sg_TitledComponent.hpp"
#include "sg_Macros.hpp"

#include <JuceHeader.h>

namespace gris
{
class MainContentComponent;
class GrisLookAndFeel;
class Player;

class PlayerComponent final
    : public juce::Component
    , private juce::TextButton::Listener
{
    Player & mPlayer;

    juce::ReadWriteLock mLock{};

    juce::TextButton mLoadWavFilesAndSpeakerSetupButton{};
    juce::TextButton mPlayButton{};

public:
    //==============================================================================
    PlayerComponent() = delete;
    explicit PlayerComponent(Player & player);
    ~PlayerComponent() override;
    SG_DELETE_COPY_AND_MOVE(PlayerComponent)
    //==============================================================================
    void handleOpenWavFilesAndSpeakerSetup();
    void buttonClicked(juce::Button * button) override;
    void resized() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(PlayerComponent)
};

class PlayerWindow final : public juce::DocumentWindow
{
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;
    PlayerComponent mPlayerComponent;

public:
    //==============================================================================
    PlayerWindow(Player & player, MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel);
    PlayerWindow() = delete;
    ~PlayerWindow() override = default;
    SG_DELETE_COPY_AND_MOVE(PlayerWindow)
    //==============================================================================
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(PlayerWindow)
};

} // namespace gris