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

//TODO VB: this will need unit tests -- and can this be constexpr
juce::ValueTree convertSpeakerSetup (const juce::ValueTree& oldSpeakerSetup)
{
    //<SPEAKER_SETUP VERSION = "3.1.14" SPAT_MODE = "Dome" DIFFUSION = "0.0" GENERAL_MUTE = "0">
    // 
    //  <SPEAKER_1 STATE = "normal" GAIN = "0.0" DIRECT_OUT_ONLY = "0">
    //      <POSITION X = "-4.371138828673793e-8" Y = "1.0" Z = "-4.371138828673793e-8" / >
    //  </SPEAKER_1>
    //  <SPEAKER_2 STATE = "normal" GAIN = "0.0" DIRECT_OUT_ONLY = "0">
    //      <POSITION X = "0.0980171337723732" Y = "0.9951847195625305" Z = "-4.371138828673793e-8" / >

    if (oldSpeakerSetup.getType ().toString () != "SPEAKER_SETUP")
    {
        jassertfalse;
        return {};
    }

    const auto version = "4.0.0";

    auto newVt = juce::ValueTree("SPEAKER_SETUP");

    // Copy all properties from oldSpeakerSetup to newVt
    for (int i = 0; i < oldSpeakerSetup.getNumProperties(); ++i) {
        const auto propertyName = oldSpeakerSetup.getPropertyName(i);
        const auto propertyValue = oldSpeakerSetup.getProperty(propertyName);
        newVt.setProperty(propertyName, propertyValue, nullptr);
    }

    // Update the version property
    newVt.setProperty("VERSION", version, nullptr);


    //then deal with the children
    for (const auto& speaker : oldSpeakerSetup)
    {
        newVt.appendChild (juce::ValueTree {"SPEAKER"}, nullptr);
        const auto id = speaker.getType().toString().removeCharacters ("SPEAKER");
    }
    

    



    return newVt;
}

}
