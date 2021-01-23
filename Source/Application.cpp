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

#include "Application.h"
#include "AudioManager.h"

//==============================================================================
void SpatGris2Application::initialise(juce::String const & /*commandLine*/)
{
    mMainWindow = std::make_unique<MainWindow>(getApplicationName(), mGrisFeel);
}

//==============================================================================
void SpatGris2Application::shutdown()
{
    mMainWindow.reset();
    AudioManager::free();
}

//==============================================================================
void SpatGris2Application::systemRequestedQuit()
{
    if (mMainWindow->exitWinApp()) {
        quit();
    }
}
