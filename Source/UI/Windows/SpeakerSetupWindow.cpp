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
//TODO VB: put somewhere central
void copyValueTreeProperties (const juce::ValueTree& source, juce::ValueTree& dest)
{
    for (int i = 0; i < source.getNumProperties (); ++i) {
        const auto propertyName = source.getPropertyName (i);
        const auto propertyValue = source.getProperty (propertyName);
        dest.setProperty (propertyName, propertyValue, nullptr);
    }
};

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
    auto newVt = juce::ValueTree ("SPEAKER_SETUP");

    //copy and update the root node
    copyValueTreeProperties (oldSpeakerSetup, newVt);
    newVt.setProperty ("VERSION", version, nullptr);

    //then do the same with the children
    for (const auto& speaker : oldSpeakerSetup)
    {
        auto newSpeaker = juce::ValueTree { "SPEAKER" };
        const auto speakerId = speaker.getType ().toString ().removeCharacters ("SPEAKER_");
        newSpeaker.setProperty ("ID", speakerId, nullptr);

        //copy properties for the speaker and its position child into the newSpeaker
        copyValueTreeProperties (speaker, newSpeaker);
        copyValueTreeProperties (speaker.getChild (0), newSpeaker);

        newVt.appendChild (newSpeaker, nullptr);
    }

    return newVt;
}

//==============================================================================

SpeakerTreeItemComponent::SpeakerTreeItemComponent (const ValueTree& v)
    : vt (v)
{
    setInterceptsMouseClicks (false, true);

    setupEditor (x, juce::String::formatted ("%.2f", static_cast<float>(v["X"])));
    setupEditor (y, juce::String::formatted ("%.2f", static_cast<float>(v["Y"])));
    setupEditor (z, juce::String::formatted ("%.2f", static_cast<float>(v["Z"])));
    setupEditor (azim, "need convert");
    setupEditor (elev, "need convert");
    setupEditor (distance, "is this radius?");
    setupEditor (del, "DEL");
}

//==============================================================================

SpeakerGroupComponent::SpeakerGroupComponent(const ValueTree & v) : SpeakerTreeItemComponent(v)
{
    setupEditor (id, v.getType ().toString ());
}

//==============================================================================

SpeakerComponent::SpeakerComponent(const ValueTree & v) : SpeakerTreeItemComponent(v)
{
    setupEditor (id, v["ID"].toString());
    setupEditor (gain, v["GAIN"].toString());
    setupEditor (highpass, "wtf?");
    setupEditor (direct, v["DIRECT_OUT_ONLY"].toString ());
}

//==============================================================================

struct Comparator
{
int compareElements (const juce::ValueTree& first, const juce::ValueTree& second)
{
    if (static_cast<int> (first["ID"]) < static_cast<int> (second["ID"]))
        return -1;
    if (static_cast<int> (first["ID"]) > static_cast<int> (second["ID"]))
        return 1;
    return 0;
}
};

void SpeakerSetupLine::sort()
{
    juce::Array<juce::ValueTree> speakers;
    for (int i = 0; i < valueTree.getNumChildren (); ++i)
        speakers.add (valueTree.getChild (i));

    Comparator comparison;
    speakers.sort (comparison);

    valueTree.removeAllChildren (&undoManager);
    for (const auto& speaker : speakers)
        valueTree.appendChild (speaker, &undoManager);
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

//==============================================================================

SpeakerSetupContainer::SpeakerSetupContainer ()
{
#if JUCE_LINUX
    const auto vtFile = juce::File ("/home/vberthiaume/Documents/git/sat/GRIS/SpatGRIS/Resources/templates/Speaker setups/DOME/Dome124(64-20-20-20)Subs2.xml");
#else
    const auto vtFile = juce::File ("C:/Users/barth/Documents/git/sat/GRIS/SpatGRIS/Resources/templates/Speaker setups/DOME/Dome124(64-20-20-20)Subs2.xml");
#endif
    const auto vt { convertSpeakerSetup (juce::ValueTree::fromXml (vtFile.loadFileAsString ())) };

    addAndMakeVisible (treeView);

    treeView.setTitle (vtFile.getFileName ());
    treeView.setDefaultOpenness (true);
    treeView.setMultiSelectEnabled (true);

    rootItem.reset (new SpeakerSetupLine (vt, undoManager));
    treeView.setRootItem (rootItem.get ());

    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);
    addAndMakeVisible (sortButton);
    undoButton.onClick = [this] { undoManager.undo (); };
    redoButton.onClick = [this] { undoManager.redo (); };
    sortButton.onClick = [this] { rootItem->sort (); };

    startTimer (500);

    setSize (500, 500);
}

} // namespace gris
