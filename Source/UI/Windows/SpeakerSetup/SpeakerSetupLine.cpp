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
SpeakerSetupLine::SpeakerSetupLine (const juce::ValueTree& v, juce::UndoManager& um)
    : valueTree (v), undoManager (um)
{
    valueTree.addListener (this);
}

std::unique_ptr<juce::Component> SpeakerSetupLine::createItemComponent ()
{
    //TODO VB: either I need to keep a reference to this pointer or I need to pass a ref to this object into this constructor
    if (mightContainSubItems ())
        return std::make_unique<SpeakerGroupComponent> (this, valueTree, undoManager);
    else
        return std::make_unique<SpeakerComponent> (this, valueTree, undoManager);
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

    moveItems (*getOwnerView (), selectedTrees, valueTree, insertIndex, undoManager);
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
            items.add (new juce::ValueTree (vti->valueTree));
}

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
    if (! vt.isValid ())
        vt = valueTree;

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
}

void SpeakerSetupLine::refreshSubItems ()
{
    clearSubItems ();

    for (int i = 0; i < valueTree.getNumChildren (); ++i)
        addSubItem (new SpeakerSetupLine (valueTree.getChild (i), undoManager));
}

void SpeakerSetupLine::treeChildrenChanged (const juce::ValueTree& parentTree)
{
    if (parentTree == valueTree)
    {
        refreshSubItems ();
        treeHasChanged ();
        setOpen (true);
    }
}
}