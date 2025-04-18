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
#include "../../../sg_GrisLookAndFeel.hpp"
#include "../../../sg_MainComponent.hpp"

namespace gris
{


struct Comparator
{
    int compareElements (const juce::ValueTree& first, const juce::ValueTree& second)
    {
        jassert (first.hasProperty (ID) && second.hasProperty (ID));
        return first[ID].toString ().compareNatural (second[ID].toString ());
    }
};

void SpeakerSetupLine::sort (juce::ValueTree vt /*= {valueTree}}*/)
{
    if (! vt.isValid())
        vt = valueTree;

    juce::Array<juce::ValueTree> speakerGroups;
    juce::Array<juce::ValueTree> allChildren;

    for (auto child : vt) {
        if (child.getType() == SPEAKER_GROUP)
            speakerGroups.add(child);

        allChildren.add(child);
    }

    //first recurse into speaker groups
    for (auto speakerGroup : speakerGroups)
        sort (speakerGroup);

    //then actually sort all children
    Comparator comparison;
    allChildren.sort (comparison);

    vt.removeAllChildren (&undoManager);
    for (const auto& speaker : allChildren)
        vt.appendChild (speaker, &undoManager);
}

//==============================================================================

SpeakerSetupWindow::SpeakerSetupWindow(juce::String const & name,
                                       GrisLookAndFeel & lnf,
                                       MainContentComponent & mainContentComponent)
    : DocumentWindow(name, lnf.getBackgroundColour(), allButtons)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lnf)
{
    setContentNonOwned (&mValueTreesDemo, true);
    setResizable (true, true);
    setUsingNativeTitleBar (true);
    setAlwaysOnTop (true);

    static constexpr auto WIDTH = 850;
    static constexpr auto HEIGHT = 600;
    static constexpr auto TITLE_BAR_HEIGHT = 30;
    const auto& controlsComponent { *mainContentComponent.getControlsComponent () };
    setBounds (controlsComponent.getScreenX (), controlsComponent.getScreenY () + TITLE_BAR_HEIGHT, WIDTH, HEIGHT);

    setVisible (true);
}

void SpeakerSetupWindow::closeButtonPressed ()
{
    mMainContentComponent.setPinkNoiseGain (tl::nullopt);
    mMainContentComponent.closeSpeakersConfigurationWindow ();
}

} // namespace gris
