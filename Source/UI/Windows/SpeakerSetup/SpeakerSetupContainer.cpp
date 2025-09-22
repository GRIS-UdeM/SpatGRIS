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

#include <functional>
#include "SpeakerSetupContainer.hpp"
#include "SpeakerTreeComponent.hpp"

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
    , containerHeader{grisLookAndFeel}
{
    speakerSetupTreeView.setRootItemVisible(false);
    addAndMakeVisible (speakerSetupTreeView);

    speakerSetupTreeView.setTitle (speakerSetupFileName);
    speakerSetupTreeView.setDefaultOpenness (true);
    speakerSetupTreeView.setMultiSelectEnabled (true);

    mainSpeakerGroupLine = std::make_unique<SpeakerSetupLine> (speakerSetupVt.getChild(0), undoManager, onSelectionChanged);
    speakerSetupTreeView.setRootItem (mainSpeakerGroupLine.get ());

    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);

    addAndMakeVisible(containerHeader);
    containerHeader.setSortFunc([this](SpeakerColumnHeader::ColumnID sortID, int sortDirection) { mainSpeakerGroupLine->sort ({}, sortID, sortDirection); });

    undoButton.onClick = [this] { undoManager.undo (); };
    redoButton.onClick = [this] { undoManager.redo (); };

    startTimer (500);

    setSize (800, 800);
}

void SpeakerSetupContainer::reload(juce::ValueTree theSpeakerSetupVt)
{
    // Note: we also have logic in SpeakerSetupLine::refreshSubItems() to restore the openness of our tree.
    // It would seem like only one of those would be enough but currently both are required to restore the state properly

    // cache the current node openness and scroll position
    const auto cachedOpenness = speakerSetupTreeView.getOpennessState(true);

    // rebuild everything
    speakerSetupVt = theSpeakerSetupVt;
    speakerSetupTreeView.setRootItem (nullptr);
    mainSpeakerGroupLine.reset(new SpeakerSetupLine (speakerSetupVt.getChild(0), undoManager, onSelectionChanged));
    speakerSetupTreeView.setRootItem (mainSpeakerGroupLine.get ());
    speakerSetupTreeView.restoreOpennessState(*cachedOpenness, true);

    // try to match and restore the previous scroll position
    auto& viewport = speakerSetupTreeView.getViewport()->getVerticalScrollBar();
    const auto maxRangeLimit = viewport.getMaximumRangeLimit();
    const auto currentRangeSize = viewport.getCurrentRangeSize();
    const auto maxScrollValue = maxRangeLimit - currentRangeSize;

    auto cachedPosition = cachedOpenness->getDoubleAttribute("scrollPos");
    if (maxScrollValue < cachedPosition)
        cachedPosition = maxScrollValue;

    viewport.setCurrentRangeStart(cachedPosition);
}

void SpeakerSetupContainer::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    auto header = bounds.removeFromTop(30);
    header.removeFromLeft(20);

    containerHeader.setBounds(header);

    auto buttons = bounds.removeFromBottom(22);
    undoButton.setBounds(buttons.removeFromLeft(100));
    buttons.removeFromLeft(6);
    redoButton.setBounds(buttons.removeFromLeft(100));
    buttons.removeFromLeft(6);

    bounds.removeFromBottom(4);
    speakerSetupTreeView.setBounds(bounds);
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

const juce::ValueTree& SpeakerSetupContainer::getSpeakerSetupVt() {
    return speakerSetupVt;
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

int SpeakerSetupContainer::getNextOrderingIndex()
{
    auto targetRow = getSelectedItem();

    int index {0};
    bool found = false;
    std::function<void(const juce::ValueTree&, bool)> recurUntilTargetFound;
    recurUntilTargetFound = [this, &targetRow, &recurUntilTargetFound, &index, &found](const juce::ValueTree& valueTree, bool countChildren) {
        if (found) {
            return;
        }
        // if we get to the right tree, set found to true and increment the index one last time. All other calls will immediately return.
        if (targetRow == valueTree) {
            found = true;
        }
        if ((valueTree.getType() == SPEAKER_GROUP && valueTree[SPEAKER_GROUP_NAME] == MAIN_SPEAKER_GROUP_NAME) || valueTree.getType() == SPEAKER_SETUP) {
            for (auto child: valueTree) {
                recurUntilTargetFound(child, true);
            }
        } else if (valueTree.getType() == SPEAKER_GROUP && valueTree[SPEAKER_GROUP_NAME] != MAIN_SPEAKER_GROUP_NAME) {
            // since we can only have speakers in this level of grouping, add the indexes all at once but still recure over everything in case the selected speaker is inside.
            index += valueTree.getNumChildren();
            for (auto child: valueTree) {
                // recurse over all children but don't count them.
                recurUntilTargetFound(child, false);
            }
        } else {
            // if we are not in a subgroup, increment the ordering index.
            if (countChildren) {
                index += 1;
            }
        }
    };
    recurUntilTargetFound(getMainSpeakerGroup(), true);

    return index;
}

int SpeakerSetupContainer::getMainGroupIndexFromOrderingIndex(const int targetOrderingIndex)
{
    auto vtRow = getMainSpeakerGroup();
    int mainGroupIndex{0};
    int currentOrderingIndex{0};
    std::function<void(const juce::ValueTree&, bool)> getMainGroupIndex;
    getMainGroupIndex = [&getMainGroupIndex, &currentOrderingIndex, &mainGroupIndex, &targetOrderingIndex](const juce::ValueTree& valueTree, bool isInSubGroup) {
        // When our currentOrderingIndex is targetOrderingIndex, we can stop everything.
        // we can just return since nothing in this function increments anything after a
        // recursive call (please keep it that way).
        if (currentOrderingIndex == targetOrderingIndex) {
            return;
        }
        if (valueTree.getType() == SPEAKER_GROUP && valueTree[SPEAKER_GROUP_NAME] != MAIN_SPEAKER_GROUP_NAME) {
            // groups take one space in the main group.
            mainGroupIndex +=1;
            for (auto child: valueTree) {
                // when we are in a speaker group, indicate to the recursive function that we are in a subgroup so we don't increment the mainGroupIndex
                // but do increment the currentOrderingIndex
                getMainGroupIndex(child, true);
            }
        } else if (valueTree.getType() == SPEAKER_SETUP || (valueTree.getType() == SPEAKER_GROUP && valueTree[SPEAKER_GROUP_NAME] == MAIN_SPEAKER_GROUP_NAME)) {
            for (auto child: valueTree)
                // if we are the root speaker setup group or the main speaker group, just process all child.
                getMainGroupIndex(child, false);
        } else {
            if (!isInSubGroup) {
                // Increment the ordering index when its a speaker and its not in a subgroup.
                mainGroupIndex +=1;
            }
            // Only increment the ordering index when its a speaker
            currentOrderingIndex += 1;
        }
    };
    getMainGroupIndex(vtRow, false);
    return mainGroupIndex;
}

juce::ValueTree SpeakerSetupContainer::getMainSpeakerGroup()
{
    return speakerSetupVt.getChild(0);
}

void SpeakerSetupContainer::timerCallback ()
{
    undoManager.beginNewTransaction ();

    undoButton.setEnabled (undoManager.canUndo ());
    redoButton.setEnabled (undoManager.canRedo ());
}

} // namespace gris
