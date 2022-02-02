/*
  ==============================================================================

    sg_PlayerWindow.cpp
    Created: 2 Feb 2022 2:57:35pm
    Author:  glanelepine
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
PlayerWindow::PlayerWindow(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel)
    : juce::DocumentWindow("Player", lookAndFeel.getBackgroundColour(), juce::DocumentWindow::allButtons, true)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    setResizable(true, true);
    setUsingNativeTitleBar(true);
    centreWithSize(800, 400);
    Component::setVisible(true);
}

void PlayerWindow::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;
}

void PlayerWindow::closeButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mMainContentComponent.closePlayerWindow();
}

} // namespace gris