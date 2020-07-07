/*
 This file is part of SpatGRIS2.

 Developers: Olivier Belanger, Nicolas Masson

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

#include "../JuceLibraryCode/JuceHeader.h"

#include "GrisLookAndFeel.h"
#include "MainWindow.h"

class SpatGRIS2Application : public JUCEApplication
{
public:
    //==============================================================================
    SpatGRIS2Application() {}

    const String getApplicationName() override { return ProjectInfo::projectName; }
    const String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool         moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const String & commandLine) override
    {
        LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);
        // This method is where you should put your application's initialisation code..
        mainWindow.reset(new MainWindow(getApplicationName()));
    }
    //==============================================================================
    void shutdown() override
    {
        // Add your application's shutdown code here..
        mainWindow = nullptr;
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        if (mainWindow->exitWinApp()) {
            quit();
        }
    }
    //==============================================================================
    void anotherInstanceStarted(const String & commandLine) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

private:
    //==============================================================================
    GrisLookAndFeel             mGrisFeel;
    std::unique_ptr<MainWindow> mainWindow;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatGRIS2Application);
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(SpatGRIS2Application)
