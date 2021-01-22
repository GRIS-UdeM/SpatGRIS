/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MainWindow.h"

#include "AudioManager.h"
#include "constants.hpp"

//==============================================================================
MainWindow::MainWindow(juce::String const & name,
                       GrisLookAndFeel & newLookAndFeel,
                       juce::String const & inputDevice,
                       juce::String const & outputDevice,
                       std::optional<juce::String> const & deviceType)
    : DocumentWindow(name, juce::Colours::lightgrey, DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    mMainContentComponent.reset(new MainContentComponent(*this, newLookAndFeel, inputDevice, outputDevice, deviceType));
    setContentOwned(mMainContentComponent.get(), true);
    setResizable(true, true);

    // this lets the command manager use keypresses that arrive in our window.
    addKeyListener(getApplicationCommandManager().getKeyMappings());

    juce::PropertiesFile * props = mMainContentComponent->getApplicationProperties().getUserSettings();

    // These offset values compensate for the title bar size.
    // TODO: it works on linux, need to be tested on MacOS.
#ifdef __linux__
    static constexpr auto X_OFFSET = 3;
    static constexpr auto Y_OFFSET = 29;
#else
    static constexpr auto X_OFFSET = 0;
    static constexpr auto Y_OFFSET = 0;
#endif

    auto const screenBounds = juce::Desktop::getInstance().getDisplays().getTotalBounds(true);

    if (props->containsKey("xPosition")) {
        auto const fitInside{ props->getIntValue("xPosition") + props->getIntValue("winWidth")
                              <= screenBounds.getWidth() };
        if (fitInside) {
            setBounds(props->getIntValue("xPosition") - X_OFFSET,
                      props->getIntValue("yPosition") - Y_OFFSET,
                      props->getIntValue("winWidth"),
                      props->getIntValue("winHeight"));
        } else {
            centreWithSize(getWidth(), getHeight());
        }
    } else {
        centreWithSize(getWidth(), getHeight());
    }

    setUsingNativeTitleBar(USE_OS_NATIVE_DIALOG_BOX);

    MainWindow::setVisible(true);
}

//==============================================================================
bool MainWindow::exitWinApp() const
{
    auto * props{ mMainContentComponent->getApplicationProperties().getUserSettings() };
    props->setValue("xPosition", getScreenX());
    props->setValue("yPosition", getScreenY());
    props->setValue("winWidth", getWidth());
    props->setValue("winHeight", getHeight());
    return mMainContentComponent->exitApp();
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
