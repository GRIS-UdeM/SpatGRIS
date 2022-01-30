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

#include "sg_MainWindow.hpp"

namespace gris
{
//==============================================================================
MainWindow::MainWindow(juce::String const & name,
                       GrisLookAndFeel & newLookAndFeel,
                       SmallGrisLookAndFeel & smallGrisLookAndFeel)
    : DocumentWindow(name, juce::Colours::lightgrey, DocumentWindow::allButtons)
    , mMainContentComponent(*this, newLookAndFeel, smallGrisLookAndFeel)
    , mTooltipWindow(&mMainContentComponent)
{
    setUsingNativeTitleBar(true);
    setContentNonOwned(&mMainContentComponent, true);
    setResizable(true, true);

    // this lets the command manager use keypresses that arrive in our window.
    DocumentWindow::addKeyListener(getApplicationCommandManager().getKeyMappings());

    // These offset values compensate for the title bar size.
#ifdef __linux__
    static constexpr auto X_OFFSET = 3;
    static constexpr auto Y_OFFSET = 29;
#else
    static constexpr auto X_OFFSET = 0;
    static constexpr auto Y_OFFSET = 0;
#endif

    auto const screenBounds = juce::Desktop::getInstance().getDisplays().getTotalBounds(true);

    auto const & appData{ mMainContentComponent.getData().appData };
    auto const windowX{ appData.windowX };
    if (windowX != -1) {
        auto const windowWidth{ appData.windowWidth };
        auto const fitInside{ windowX + windowWidth <= screenBounds.getWidth() };
        if (fitInside) {
            auto const windowY{ appData.windowY };
            auto const windowHeight{ appData.windowHeight };
            DocumentWindow::setBounds(windowX - X_OFFSET, windowY - Y_OFFSET, windowWidth, windowHeight);
        } else {
            DocumentWindow::centreWithSize(DocumentWindow::getWidth(), DocumentWindow::getHeight());
        }
    } else {
        DocumentWindow::centreWithSize(DocumentWindow::getWidth(), DocumentWindow::getHeight());
    }

    DocumentWindow::setVisible(true);
}

//==============================================================================
bool MainWindow::exitWinApp()
{
    return mMainContentComponent.exitApp();
}

//==============================================================================
void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

//==============================================================================
MainWindow * MainWindow::getMainAppWindow()
{
    for (int i = TopLevelWindow::getNumTopLevelWindows(); --i >= 0;) {
        if (auto * maw = dynamic_cast<MainWindow *>(TopLevelWindow::getTopLevelWindow(i)))
            return maw;
    }
    return nullptr;
}

//==============================================================================
juce::ApplicationCommandManager & MainWindow::getApplicationCommandManager()
{
    return mApplicationCommandManager;
}

} // namespace gris