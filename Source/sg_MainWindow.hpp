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

#include "sg_MainComponent.hpp"

namespace gris
{
//==============================================================================
/* This class implements the desktop window that contains an instance of
   our MainContentComponent class.
*/
class MainWindow final : public juce::DocumentWindow
{
    juce::ApplicationCommandManager mApplicationCommandManager{};
    MainContentComponent mMainContentComponent;
    juce::TooltipWindow mTooltipWindow;

public:
    //==============================================================================
    MainWindow() = delete;
    MainWindow(juce::String const & name,
               GrisLookAndFeel & newLookAndFeel,
               SmallGrisLookAndFeel & smallGrisLookAndFeel);
    ~MainWindow() override = default;
    SG_DELETE_COPY_AND_MOVE(MainWindow)
    //==============================================================================
    [[nodiscard]] bool exitWinApp();
    [[nodiscard]] juce::ApplicationCommandManager & getApplicationCommandManager();
    //==============================================================================
    void closeButtonPressed() override;
    //==============================================================================
    [[nodiscard]] static MainWindow * getMainAppWindow();

private:
    //=============================================================
    JUCE_LEAK_DETECTOR(MainWindow)
};

} // namespace gris