/*
 This file is part of ServerGris.
 
 Developers: Olivier Belanger, Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MainWindow.h"

static ScopedPointer<ApplicationCommandManager> applicationCommandManager;

MainWindow::MainWindow(String name)  : DocumentWindow (name,Colours::lightgrey,DocumentWindow::allButtons)
{
    setUsingNativeTitleBar (true);
    mcc = new MainContentComponent(this);
    setContentOwned (mcc, true);
    setResizable (true, true);

    // this lets the command manager use keypresses that arrive in our window to send out commands
    addKeyListener (getApplicationCommandManager().getKeyMappings());

    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

bool MainWindow::exitWinApp() {
    return mcc->exitApp();
}

ApplicationCommandManager& MainWindow::getApplicationCommandManager()
{
    if (applicationCommandManager == nullptr)
        applicationCommandManager = new ApplicationCommandManager();

    return *applicationCommandManager;
}
