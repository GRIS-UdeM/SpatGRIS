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

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "GrisLookAndFeel.h"
#include "MainWindow.h"

class SpatGris2Application final : public juce::JUCEApplication
{
    GrisLookAndFeel mGrisFeel{};
    std::unique_ptr<MainWindow> mMainWindow{};

public:
    //==============================================================================
    SpatGris2Application() = default;
    ~SpatGris2Application() override = default;

    SpatGris2Application(SpatGris2Application const &) = delete;
    SpatGris2Application(SpatGris2Application &&) = delete;

    SpatGris2Application & operator=(SpatGris2Application const &) = delete;
    SpatGris2Application & operator=(SpatGris2Application &&) = delete;
    //==============================================================================
    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const juce::String & /*commandLine*/) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);

        mMainWindow.reset(new MainWindow(getApplicationName()));
    }
    //==============================================================================
    void shutdown() override { mMainWindow.reset(); }

    //==============================================================================
    void systemRequestedQuit() override
    {
        if (mMainWindow->exitWinApp()) {
            quit();
        }
    }
    //==============================================================================
    void anotherInstanceStarted(const juce::String & /*commandLine*/) override {}

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatGris2Application)
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(SpatGris2Application)
