/*
 This file is part of SpatGRIS.

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

#include "SpeakerSetupWindow.hpp"
#include "../../sg_GrisLookAndFeel.hpp"
#include "../../sg_MainComponent.hpp"

namespace gris
{
SpeakerSetupWindow::SpeakerSetupWindow(juce::String const & name,
                                       GrisLookAndFeel & lnf,
                                       MainContentComponent & mainContentComponent)
    : DocumentWindow(name, lnf.getBackgroundColour(), allButtons)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lnf)
{
}
}
