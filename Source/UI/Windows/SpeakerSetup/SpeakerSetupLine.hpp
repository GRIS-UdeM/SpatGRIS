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

#pragma once

#include "SpeakerTreeComponent.hpp"

namespace gris
{
class SpeakerSetupLine final
    : public juce::TreeViewItem
    , private juce::ValueTree::Listener
{
public:
    SpeakerSetupLine (const juce::ValueTree& v, juce::UndoManager& um)
        : valueTree (v), undoManager (um)
    {
        valueTree.addListener (this);
    }

    juce::String getUniqueName () const override
    {
        return valueTree.getType ().toString ();
    }

    bool mightContainSubItems () override
    {
        return valueTree.getNumChildren () > 0;
    }

    void sort ();

#if 1
    std::unique_ptr<juce::Component> createItemComponent () override
    {
        if (mightContainSubItems ())
            return std::make_unique<SpeakerGroupComponent> (valueTree);
        else
            return std::make_unique<SpeakerComponent> (valueTree);
    }
#else
    void paintItem (Graphics& g, int width, int height) override
    {
        if (isSelected ())
            g.fillAll (getUIColourIfAvailable (LookAndFeel_V4::ColourScheme::UIColour::highlightedFill, Colours::teal));

        g.setColour (getUIColourIfAvailable (LookAndFeel_V4::ColourScheme::UIColour::defaultText, Colours::black));
        g.setFont (15.0f);

        g.drawText (treeView.getType ().toString (), 4, 0, width - 4, height, Justification::centredLeft, true);
    }
#endif

    void itemOpennessChanged (bool isNowOpen) override
    {
        if (isNowOpen && getNumSubItems () == 0)
            refreshSubItems ();
        else
            clearSubItems ();
    }

    juce::var getDragSourceDescription () override
    {
        return "Drag Demo";
    }

    bool isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails& /*dragSourceDetails*/) override
    {
        //TODO VB: using this as a shortcut to identify groups, we need another way
        return mightContainSubItems ();
    }

    void itemDropped (const juce::DragAndDropTarget::SourceDetails&, int insertIndex) override
    {
        juce::OwnedArray<juce::ValueTree> selectedTrees;
        getSelectedTreeViewItems (*getOwnerView (), selectedTrees);

        moveItems (*getOwnerView (), selectedTrees, valueTree, insertIndex, undoManager);
    }

    static void moveItems (juce::TreeView& treeView, const juce::OwnedArray<juce::ValueTree>& items,
                           juce::ValueTree newParent, int insertIndex, juce::UndoManager& undoManager)
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

    static void getSelectedTreeViewItems (juce::TreeView& treeView, juce::OwnedArray<juce::ValueTree>& items)
    {
        auto numSelected = treeView.getNumSelectedItems ();

        for (int i = 0; i < numSelected; ++i)
            if (auto* vti = dynamic_cast<SpeakerSetupLine*> (treeView.getSelectedItem (i)))
                items.add (new juce::ValueTree (vti->valueTree));
    }

private:
    juce::ValueTree valueTree;
    juce::UndoManager& undoManager;

    void refreshSubItems ()
    {
        clearSubItems ();

        for (int i = 0; i < valueTree.getNumChildren (); ++i)
            addSubItem (new SpeakerSetupLine (valueTree.getChild (i), undoManager));
    }

    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override
    {
        repaintItem ();
    }

    void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree&) override { treeChildrenChanged (parentTree); }
    void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree&, int) override { treeChildrenChanged (parentTree); }
    void valueTreeChildOrderChanged (juce::ValueTree& parentTree, int, int) override { treeChildrenChanged (parentTree); }
    void valueTreeParentChanged (juce::ValueTree&) override {}

    void treeChildrenChanged (const juce::ValueTree& parentTree)
    {
        if (parentTree == valueTree)
        {
            refreshSubItems ();
            treeHasChanged ();
            setOpen (true);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerSetupLine)
};

}