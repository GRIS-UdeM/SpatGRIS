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

#include "SpeakerSetupLine.hpp"
namespace gris
{

bool SpeakerSetupLine::isDeletingGroup = false;

SpeakerSetupLine::SpeakerSetupLine(const juce::ValueTree & v,
                                   juce::UndoManager & um,
                                   std::function<void()> selectionChanged)
    : lineValueTree(v)
    , undoManager(um)
    , onSelectionChanged(selectionChanged)
{
    lineValueTree.addListener(this);
}

std::unique_ptr<juce::Component> SpeakerSetupLine::createItemComponent ()
{
    if (mightContainSubItems ())
        return std::make_unique<SpeakerGroupComponent> (this, lineValueTree, undoManager);
    else
        return std::make_unique<SpeakerComponent> (this, lineValueTree, undoManager);
}

void SpeakerSetupLine::itemOpennessChanged (bool isNowOpen)
{
    if (isNowOpen && getNumSubItems () == 0)
        refreshSubItems ();
    else
        clearSubItems ();
}

void SpeakerSetupLine::itemDropped (const juce::DragAndDropTarget::SourceDetails&, int insertIndex)
{
    juce::OwnedArray<juce::ValueTree> selectedTrees;
    getSelectedTreeViewItems (*getOwnerView (), selectedTrees);

    moveItems (*getOwnerView (), selectedTrees, lineValueTree, insertIndex, undoManager);
}

void SpeakerSetupLine::moveItems (juce::TreeView& treeView, const juce::OwnedArray<juce::ValueTree>& items, juce::ValueTree newParent, int insertIndex, juce::UndoManager& undoManager)
{
    if (items.size () > 0)
    {
        std::unique_ptr<juce::XmlElement> oldOpenness (treeView.getOpennessState (false));

        for (auto* v : items)
        {
            if (v->getParent ().isValid () && newParent != *v && ! newParent.isAChildOf (*v))
            {
                if (v->getParent () == newParent && newParent.indexOf (*v) < insertIndex)
                    --insertIndex;

                v->getParent ().removeChild (*v, &undoManager);
                newParent.addChild (*v, insertIndex, &undoManager);
            }
        }

        if (oldOpenness != nullptr)
            treeView.restoreOpennessState (*oldOpenness, false);
    }
}

void SpeakerSetupLine::getSelectedTreeViewItems (juce::TreeView& treeView, juce::OwnedArray<juce::ValueTree>& items)
{
    auto numSelected = treeView.getNumSelectedItems ();

    for (int i = 0; i < numSelected; ++i)
        if (auto* vti = dynamic_cast<SpeakerSetupLine*> (treeView.getSelectedItem (i)))
            items.add (new juce::ValueTree (vti->lineValueTree));
}

void SpeakerSetupLine::selectChildSpeaker(tl::optional<output_patch_t> const outputPatch)
{
    auto treeView = getOwnerView();
    for (int i = 0; i < treeView->getNumRowsInTree(); ++i) {
        auto speakerSetupLine = dynamic_cast<SpeakerSetupLine *>(treeView->getItemOnRow(i));
        if (!speakerSetupLine || speakerSetupLine->mightContainSubItems())
            continue;

        if (speakerSetupLine->getOutputPatch() == outputPatch)
            speakerSetupLine->setSelected(true, juce::dontSendNotification);
        else
            speakerSetupLine->setSelected (false, juce::dontSendNotification);
    }
}

struct Comparator {
    int compareElements(const juce::ValueTree & first, const juce::ValueTree & second)
    {
        // Try to get SPEAKER_PATCH_ID or SPEAKER_GROUP_NAME from each ValueTree
        juce::String firstStr, secondStr;

        if (first.hasProperty(SPEAKER_PATCH_ID))
            firstStr = first[SPEAKER_PATCH_ID].toString();
        else if (first.hasProperty(SPEAKER_GROUP_NAME))
            firstStr = first[SPEAKER_GROUP_NAME].toString();
        else
            jassertfalse;

        if (second.hasProperty(SPEAKER_PATCH_ID))
            secondStr = second[SPEAKER_PATCH_ID].toString();
        else if (second.hasProperty(SPEAKER_GROUP_NAME))
            secondStr = second[SPEAKER_GROUP_NAME].toString();
        else
            jassertfalse;

        // Compare as strings
        return firstStr.compareNatural(secondStr);
    }
};

#define USE_TREE_VIEW_COMPARATOR 0
#define USE_VALUE_TREE_VIEW_COMPARATOR 1

#if USE_TREE_VIEW_COMPARATOR
struct TreeViewItemComparator {
    int compareElements (juce::TreeViewItem* first, juce::TreeViewItem* second)
    {
        auto const * firstLine = dynamic_cast<SpeakerSetupLine *>(first);
        auto const * secondLine = dynamic_cast<SpeakerSetupLine *>(second);
        if (!firstLine || !secondLine)
        {
            jassertfalse; // This should not happen
            return 0;
        }

        auto const firstString { firstLine->getSpeakerIdOrGroupName () };
        auto const secondString { secondLine->getSpeakerIdOrGroupName () };

        return firstString.compareNatural (secondString);
    }
};
#endif

// TODO: look into whether this should use juce::TreeViewItem::sortSubItems() instead of sorting the value tree
void SpeakerSetupLine::sort (juce::ValueTree vt /*= {valueTree}}*/)
{
    if (! vt.isValid ())
        vt = lineValueTree;

#if USE_TREE_VIEW_COMPARATOR
    TreeViewItemComparator comparison;
    sortSubItems (comparison);
    //treeChildrenChanged (vt);
    refreshSubItems();
#elif USE_VALUE_TREE_VIEW_COMPARATOR
    Comparator comparison;
    vt.sort (comparison, &undoManager, false);
    refreshSubItems ();
#else

    if (! vt.isValid ())
        vt = lineValueTree;

    juce::Array<juce::ValueTree> speakerGroups;
    juce::Array<juce::ValueTree> allChildren;

    for (auto child : vt) {
        if (child.getType () == SPEAKER_GROUP)
            speakerGroups.add (child);

        allChildren.add (child);
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
#endif
}

tl::optional<output_patch_t> SpeakerSetupLine::getOutputPatch ()
{
    if (lineValueTree.getType () == SPEAKER && lineValueTree.hasProperty(SPEAKER_PATCH_ID))
        return output_patch_t{ lineValueTree[SPEAKER_PATCH_ID] };
    else
        return tl::nullopt;
}

void SpeakerSetupLine::refreshSubItems ()
{
    clearSubItems ();

    for (int i = 0; i < lineValueTree.getNumChildren (); ++i)
        addSubItem (new SpeakerSetupLine (lineValueTree.getChild (i), undoManager, onSelectionChanged));
}

void SpeakerSetupLine::itemSelectionChanged(bool /*isNowSelected*/)
{
    onSelectionChanged();
}

void SpeakerSetupLine::treeChildrenChanged (const juce::ValueTree& parentTree)
{
    if (parentTree == lineValueTree)
    {
        refreshSubItems ();
        treeHasChanged ();
        setOpen (true);
    }
}
}