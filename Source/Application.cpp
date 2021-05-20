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

#include "Application.hpp"
#include "AudioManager.hpp"

//==============================================================================
void SpatGrisApplication::initialise(juce::String const & /*commandLine*/)
{
    mMainWindow = std::make_unique<MainWindow>(getApplicationName(), mGrisFeel, mSmallLookAndFeel);
}

//==============================================================================
void SpatGrisApplication::shutdown()
{
    mMainWindow.reset();
    AudioManager::free();
}

//==============================================================================
void SpatGrisApplication::systemRequestedQuit()
{
    if (mMainWindow->exitWinApp()) {
        quit();
    }
}
