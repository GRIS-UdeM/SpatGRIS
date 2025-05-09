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
SpeakerSetupContainer::SpeakerSetupContainer(const SpeakerSetup & setup, const juce::File & speakerSetupXmlFile, juce::UndoManager& undoMan)
    : speakerSetup{ setup }
    , vtFile{ speakerSetupXmlFile }
    , undoManager (undoMan)
{
    vt = convertSpeakerSetup (juce::ValueTree::fromXml (vtFile.loadFileAsString ()));

    //DBG (vt.toXmlString());

    speakerSetupTreeView.setRootItemVisible(false);
    addAndMakeVisible (speakerSetupTreeView);

    speakerSetupTreeView.setTitle (vtFile.getFileName ());
    speakerSetupTreeView.setDefaultOpenness (true);
    speakerSetupTreeView.setMultiSelectEnabled (true);

    // at this point vt is the whole speaker setup, but I think the lines should only care about groups or speakers, so
    // only giving them the main group for now?
    mainSpeakerGroupLine.reset (new SpeakerSetupLine (vt.getChild(0), undoManager));
    speakerSetupTreeView.setRootItem (mainSpeakerGroupLine.get ());

    auto setLabelText = [this](juce::Label& label, const juce::String & text) {
        label.setColour (juce::Label::ColourIds::outlineColourId, lookAndFeel.mLightColour.withAlpha (.25f));
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
    addAndMakeVisible (saveButton);

    undoButton.onClick = [this] { undoManager.undo (); };
    redoButton.onClick = [this] { undoManager.redo (); };
    sortButton.onClick = [this] { mainSpeakerGroupLine->sort (); };
    saveButton.onClick = [this] { saveSpeakerSetup(); };

    startTimer (500);

    setSize (500, 500);

    setLookAndFeel (&lookAndFeel);
}

void SpeakerSetupContainer::resized ()
{
    auto bounds = getLocalBounds ().reduced (8);
    auto header = bounds.removeFromTop (30);
    header.removeFromLeft (20);
    // this is taken from SpeakerTreeComponent::resized () and should be DRYed
    {
        //TODO VB: for some reason this is slightly less than in SpeakerTreeComponent, probably because of the tree arrow or something
        constexpr auto fixedLeftColWidth{ 172 /*200*/ };
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
    buttons.removeFromLeft (6);
    saveButton.setBounds (buttons.removeFromLeft (100));

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

void SpeakerSetupContainer::saveSpeakerSetup()
{
    const auto saveFile = [valueTree = vt](juce::File file) {
        if (! file.replaceWithText (valueTree.toXmlString())) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Error",
                                                   "Failed to save the file: " + file.getFullPathName());
        } else {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                   "Success",
                                                   "File saved successfully: " + file.getFullPathName());
        }
    };

    const auto saveAs = false;
    if (saveAs)
    {
        juce::FileChooser fileChooser("Save Speaker Setup",
                                      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                                      "*.xml");
        if (fileChooser.browseForFileToSave(true))
            saveFile (fileChooser.getResult());
    }
    else
    {
        saveFile(vtFile);
    }
}

juce::ValueTree SpeakerSetupContainer::getSelectedItem()
{
    //if we have a selection, return the last selected item. Otherwise return the last overall item
    if (auto const numSelected{ speakerSetupTreeView.getNumSelectedItems() }) {
        if (auto const selected = dynamic_cast<SpeakerSetupLine *>(speakerSetupTreeView.getSelectedItem(numSelected)))
            return selected->getValueTree();
    } else {
        auto const numLines = speakerSetupTreeView.getNumRowsInTree();
        if (auto const last = dynamic_cast<SpeakerSetupLine *>(speakerSetupTreeView.getItemOnRow(numLines-1)))
            return last->getValueTree();
    }

    jassertfalse;
    return juce::ValueTree{};
}

} // namespace gris
