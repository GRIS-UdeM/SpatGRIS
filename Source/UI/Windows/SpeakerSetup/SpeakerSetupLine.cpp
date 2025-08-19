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
#include "SpeakerTreeComponent.hpp"
#include "SpeakerGroupSettingsWindow.hpp"

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

void SpeakerSetupLine::moveItems(juce::TreeView & treeView,
                                 const juce::OwnedArray<juce::ValueTree> & items,
                                 juce::ValueTree newParent,
                                 int insertIndex,
                                 juce::UndoManager & undoManager)
{
    if (items.size() > 0) {
        for (auto * v : items) {
            auto curParent{ v->getParent() };
            if (curParent.isValid() && newParent != *v && !newParent.isAChildOf(*v)) {
                if (curParent == newParent && newParent.indexOf(*v) < insertIndex)
                    --insertIndex;

                curParent.removeChild(*v, &undoManager);

                auto const childPosition
                    = juce::VariantConverter<Position>::fromVar(v->getProperty(CARTESIAN_POSITION));
                auto const prevParentPosition
                    = juce::VariantConverter<Position>::fromVar(curParent[CARTESIAN_POSITION]);
                auto const newParentPosition = juce::VariantConverter<Position>::fromVar(newParent[CARTESIAN_POSITION]);
                auto const summedPosition = childPosition.getCartesian() + prevParentPosition.getCartesian()
                                            - newParentPosition.getCartesian();
                v->setProperty(CARTESIAN_POSITION,
                               juce::VariantConverter<Position>::toVar(Position{ summedPosition }),
                               &undoManager);

                newParent.addChild(*v, insertIndex, &undoManager);
            }
        }
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

struct ValueTreeComparator {
    juce::String getSortName(const juce::ValueTree & valueTree)
    {
        juce::String str;
        if (valueTree.hasProperty(SPEAKER_PATCH_ID))
            str = valueTree[SPEAKER_PATCH_ID].toString();
        else if (valueTree.hasProperty(SPEAKER_GROUP_NAME))
            str = valueTree[SPEAKER_GROUP_NAME].toString();
        else
            jassertfalse;
        return str;
    }

    int compareElements(const juce::ValueTree & first, const juce::ValueTree & second)
    {
        juce::String firstStr = getSortName(first);
        juce::String secondStr = getSortName(second);
        // Compare as strings
        return firstStr.compareNatural(secondStr);
    }
};

void SpeakerSetupLine::sort(juce::ValueTree vt)
{
    if (!vt.isValid())
        vt = lineValueTree;

    juce::Array<juce::ValueTree> speakerGroups;
    juce::Array<juce::ValueTree> allChildren;

    for (auto child : vt) {
        if (child.getType() == SPEAKER_GROUP)
            speakerGroups.add(child);

        allChildren.add(child);
    }

    // first recurse into speaker groups to sort them
    for (auto speakerGroup : speakerGroups)
        sort(speakerGroup);

    // then actually sort all children
    ValueTreeComparator comparison;
    allChildren.sort(comparison);

    // and rebuild the tree
    vt.removeAllChildren(&undoManager);
    for (const auto & speaker : allChildren)
        vt.appendChild(speaker, &undoManager);
}

tl::optional<output_patch_t> SpeakerSetupLine::getOutputPatch ()
{
    if (lineValueTree.getType () == SPEAKER && lineValueTree.hasProperty(SPEAKER_PATCH_ID))
        return output_patch_t{ lineValueTree[SPEAKER_PATCH_ID] };
    else
        return tl::nullopt;
}

void SpeakerSetupLine::refreshSubItems()
{
    // Note: we also have logic in SpeakerSetupContainer::reload() to restore the openness of our tree.
    // It would seem like only one of those would be enough but currently both are required to restore the state properly

    // by default the addSubItem() call below will automatically open the sub-item, so if they are
    // currently closed we cache their UUID here and restore the closedness after the addSubItem() call
    std::unordered_set<juce::String> closedSubItems;
    for (int i = 0; i < getNumSubItems(); ++i)
        if (auto* item = dynamic_cast<SpeakerSetupLine*>(getSubItem(i)))
            if (! item->isOpen())
                closedSubItems.insert(item->lineValueTree[UUID]);

    // clear everything
    clearSubItems();

    // re-add the sub-items
    for (int i = 0; i < lineValueTree.getNumChildren(); ++i)
    {
        auto childTree = lineValueTree.getChild(i);
        auto* childItem = new SpeakerSetupLine(childTree, undoManager, onSelectionChanged);

        addSubItem(childItem);

        // making sure to close them if they were closed before
        if (closedSubItems.contains(childTree[UUID]))
            childItem->setOpen(false);
    }
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
    }
}

juce::String SpeakerSetupLine::getUniqueName() const
{
    jassert(lineValueTree.hasProperty(UUID));
    return lineValueTree[UUID];
}

} // namespace gris
