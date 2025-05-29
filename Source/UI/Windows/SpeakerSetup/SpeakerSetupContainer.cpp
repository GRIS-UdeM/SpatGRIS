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
SpeakerSetupContainer::SpeakerSetupContainer(const juce::File & speakerSetupXmlFile,
                                             juce::ValueTree theSpeakerSetupVt,
                                             juce::UndoManager & undoMan,
                                             std::function<void()> selectionChanged)
    : speakerSetupFileName { speakerSetupXmlFile.getFileName()}
    , speakerSetupVt (theSpeakerSetupVt)
    , undoManager (undoMan)
    , onSelectionChanged (selectionChanged)
{
    speakerSetupTreeView.setRootItemVisible(false);
    addAndMakeVisible (speakerSetupTreeView);

    speakerSetupTreeView.setTitle (speakerSetupFileName);
    speakerSetupTreeView.setDefaultOpenness (true);
    speakerSetupTreeView.setMultiSelectEnabled (true);

    mainSpeakerGroupLine.reset (new SpeakerSetupLine (speakerSetupVt.getChild(0), undoManager, onSelectionChanged));
    speakerSetupTreeView.setRootItem (mainSpeakerGroupLine.get ());

    auto setLabelText = [this](juce::Label& label, const juce::String & text) {
        label.setColour (juce::Label::ColourIds::outlineColourId, grisLookAndFeel.mLightColour.withAlpha (.25f));
        label.setText(text, juce::dontSendNotification);
        addAndMakeVisible(label);
    };

    setLabelText (id, "ID");
    setLabelText (x, "X");
    setLabelText (y, "Y");
    setLabelText (z, "Z");
    setLabelText (azim, "Azimuth");
    setLabelText (elev, "Elevation");
    setLabelText (radius, "Radius");
    setLabelText (gain, "Gain");
    setLabelText (highpass, "Highpass");
    setLabelText (direct, "Direct");
    setLabelText (del, "Delete");
    setLabelText (drag, "Drag");

    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);
    addAndMakeVisible (sortButton);

    undoButton.onClick = [this] { undoManager.undo (); };
    redoButton.onClick = [this] { undoManager.redo (); };
    sortButton.onClick = [this] { mainSpeakerGroupLine->sort (); };

    startTimer (500);

    setSize (500, 500);
}

void SpeakerSetupContainer::reload(juce::ValueTree theSpeakerSetupVt)
{
    speakerSetupVt = theSpeakerSetupVt;
    speakerSetupTreeView.setRootItem (nullptr);
    mainSpeakerGroupLine.reset (new SpeakerSetupLine (speakerSetupVt.getChild(0), undoManager, onSelectionChanged));
    speakerSetupTreeView.setRootItem (mainSpeakerGroupLine.get ());
}

void SpeakerSetupContainer::resized ()
{
    auto bounds = getLocalBounds ().reduced (8);
    auto header = bounds.removeFromTop (30);
    header.removeFromLeft (20);
    // this is taken from SpeakerTreeComponent::resized () and should be DRYed
    {
        constexpr auto fixedLeftColWidth{ 172 };
        constexpr auto otherColWidth{ 60 };
        constexpr auto dragColWidth { 40 };

        id.setBounds(header.removeFromLeft(fixedLeftColWidth));

        // then position the other components with a fixed width of otherColWidth
        for (auto label : { &x, &y, &z, &azim, &elev, &radius, &gain, &highpass, &direct, &del })
            label->setBounds(header.removeFromLeft(otherColWidth));

        drag.setBounds (header.removeFromLeft (dragColWidth));
    }

    auto buttons = bounds.removeFromBottom (22);
    undoButton.setBounds (buttons.removeFromLeft (100));
    buttons.removeFromLeft (6);
    redoButton.setBounds (buttons.removeFromLeft (100));
    buttons.removeFromLeft (6);
    sortButton.setBounds (buttons.removeFromLeft (100));

    bounds.removeFromBottom (4);
    speakerSetupTreeView.setBounds (bounds);
}

void SpeakerSetupContainer::deleteSelectedItems ()
{
    juce::OwnedArray<juce::ValueTree> selectedItems;
    SpeakerSetupLine::getSelectedTreeViewItems (speakerSetupTreeView, selectedItems);

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

juce::ValueTree SpeakerSetupContainer::getSelectedItem()
{
    //if we have a selection, return the last selected item. Otherwise return the last overall item
    if (auto const numSelected{ speakerSetupTreeView.getNumSelectedItems() }) {
        if (auto const selected = dynamic_cast<SpeakerSetupLine *>(speakerSetupTreeView.getSelectedItem(0)))
            return selected->getValueTree();
    } else {
        auto const numLines = speakerSetupTreeView.getNumRowsInTree();
        if (auto const last = dynamic_cast<SpeakerSetupLine *>(speakerSetupTreeView.getItemOnRow(numLines-1)))
            return last->getValueTree();
    }

    jassertfalse;
    return juce::ValueTree{};
}

juce::Array<output_patch_t> SpeakerSetupContainer::getSelectedSpeakers()
{
    juce::Array<output_patch_t> selectedOutputPatches{};

    for (int i = 0; i < speakerSetupTreeView.getNumSelectedItems(); ++i)
        if (auto * speakerLine = dynamic_cast<SpeakerSetupLine *>(speakerSetupTreeView.getSelectedItem(i)))
            if (auto outputPatch = speakerLine->getOutputPatch())
                selectedOutputPatches.add(*outputPatch);

    return selectedOutputPatches;
}

void SpeakerSetupContainer::selectSpeaker (tl::optional<output_patch_t> const outputPatch)
{
    mainSpeakerGroupLine->selectChildSpeaker(outputPatch);
}

std::pair<juce::ValueTree, int> SpeakerSetupContainer::getParentAndIndexOfSelectedItem ()
{
    auto const vtRow = getSelectedItem ();
    auto parent = vtRow.getParent ();
    return { parent, parent.indexOf (vtRow) };
}

std::pair<juce::ValueTree, int> SpeakerSetupContainer::getMainSpeakerGroupAndIndex ()
{
    auto vtRow = getSelectedItem ();

    juce::ValueTree parent;
    int index {0};
    do
    {
        parent = vtRow.getParent ();
        index += parent.indexOf (vtRow);
        vtRow = parent;
    } while (vtRow[SPEAKER_GROUP_NAME] != MAIN_SPEAKER_GROUP_NAME);


    return { parent, index };
}

std::pair<juce::ValueTree, int> SpeakerSetupContainer::getCurSpeakerGroupAndIndex()
{
    auto vtRow = getSelectedItem();

    auto const parent{ vtRow.getParent() };
    auto const index{ parent.indexOf(vtRow) };

    return { parent, index };
}

void SpeakerSetupContainer::timerCallback ()
{
    undoManager.beginNewTransaction ();

    undoButton.setEnabled (undoManager.canUndo ());
    redoButton.setEnabled (undoManager.canRedo ());
}

} // namespace gris
