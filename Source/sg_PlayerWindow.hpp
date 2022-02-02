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

#include <JuceHeader.h>

namespace gris
{
class MainContentComponent;
class GrisLookAndFeel;

class PlayerWindow final : public juce::DocumentWindow
{
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;

public:
    //==============================================================================
    PlayerWindow(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel);
    PlayerWindow() = delete;
    ~PlayerWindow() override = default;
    //==============================================================================
    PlayerWindow(PlayerWindow const &) = delete;
    PlayerWindow(PlayerWindow &&) = delete;
    PlayerWindow & operator=(PlayerWindow const &) = delete;
    PlayerWindow & operator=(PlayerWindow &&) = delete;
    //==============================================================================
    void resized() override;
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(PlayerWindow)
};

} // namespace gris