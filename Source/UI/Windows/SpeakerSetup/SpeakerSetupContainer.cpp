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

#include "SpeakerSetupContainer.hpp"
namespace gris
{
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

    // at this point vt is the whole speaker setup, but I think the lines should only care about groups or speakers, so
    // only giving them the main group for now?
    mainSpeakerGroupLine.reset (new SpeakerSetupLine (vt.getChild(0), undoManager));
    treeView.setRootItem (mainSpeakerGroupLine.get ());

    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);
    addAndMakeVisible (sortButton);
    undoButton.onClick = [this] { undoManager.undo (); };
    redoButton.onClick = [this] { undoManager.redo (); };
    sortButton.onClick = [this] { mainSpeakerGroupLine->sort (); };

    startTimer (500);

    setSize (500, 500);
}
void SpeakerSetupContainer::resized ()
{
    auto r = getLocalBounds ().reduced (8);

    auto buttons = r.removeFromBottom (22);
    undoButton.setBounds (buttons.removeFromLeft (100));
    buttons.removeFromLeft (6);
    redoButton.setBounds (buttons.removeFromLeft (100));
    buttons.removeFromLeft (6);
    sortButton.setBounds (buttons.removeFromLeft (100));

    r.removeFromBottom (4);
    treeView.setBounds (r);
}

void SpeakerSetupContainer::deleteSelectedItems ()
{
    juce::OwnedArray<juce::ValueTree> selectedItems;
    SpeakerSetupLine::getSelectedTreeViewItems (treeView, selectedItems);

    for (auto* v : selectedItems)
    {
        if (v->getParent ().isValid ())
            v->getParent ().removeChild (*v, &undoManager);
    }
}

bool SpeakerSetupContainer::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelectedItems ();
        return true;
    }

    if (key == juce::KeyPress ('z', juce::ModifierKeys::commandModifier, 0))
    {
        undoManager.undo ();
        return true;
    }

    if (key == juce::KeyPress ('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        undoManager.redo ();
        return true;
    }

    return Component::keyPressed (key);
}
}
