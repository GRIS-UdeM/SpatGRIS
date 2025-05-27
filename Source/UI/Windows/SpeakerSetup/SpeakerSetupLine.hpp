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
    SpeakerSetupLine(const juce::ValueTree & v, juce::UndoManager & um, std::function<void()> selectionChanged);

    juce::String getUniqueName() const override { return lineValueTree.getType().toString(); }

    bool isSpeakerGroup() const { return lineValueTree.getType () == SPEAKER_GROUP; }

    bool mightContainSubItems() override { return isSpeakerGroup(); }

    juce::String getSpeakerIdOrGroupName() const
    {
        return isSpeakerGroup() ? lineValueTree[SPEAKER_GROUP_NAME] : lineValueTree[SPEAKER_PATCH_ID];
    }

    std::unique_ptr<juce::Component> createItemComponent () override;

    void itemOpennessChanged (bool isNowOpen) override;

    juce::var getDragSourceDescription () override { return lineValueTree.getType ().toString(); }

    bool isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails& /*dragSourceDetails*/) override
    {
        return mightContainSubItems ();
    }

    void itemDropped (const juce::DragAndDropTarget::SourceDetails&, int insertIndex) override;

    static void moveItems (juce::TreeView& treeView, const juce::OwnedArray<juce::ValueTree>& items,
                           juce::ValueTree newParent, int insertIndex, juce::UndoManager& undoManager);

    static void getSelectedTreeViewItems (juce::TreeView& treeView, juce::OwnedArray<juce::ValueTree>& items);

    void selectChildSpeaker (tl::optional<output_patch_t> const outputPatch);

    void sort (juce::ValueTree vt = {});

    juce::ValueTree getValueTree() { return lineValueTree; }

    tl::optional<output_patch_t> getOutputPatch();

    static bool isDeletingGroup;
private:
    juce::ValueTree lineValueTree;
    juce::UndoManager& undoManager;
    std::function<void ()> onSelectionChanged;

    void refreshSubItems ();

    void itemSelectionChanged(bool isNowSelected) override;

    void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree&) override { treeChildrenChanged (parentTree); }
    void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree&, int) override { treeChildrenChanged (parentTree); }
    void valueTreeChildOrderChanged (juce::ValueTree& parentTree, int, int) override { treeChildrenChanged (parentTree); }

    void treeChildrenChanged (const juce::ValueTree& parentTree);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerSetupLine)
};

}