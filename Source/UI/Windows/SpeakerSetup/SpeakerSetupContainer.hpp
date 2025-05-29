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

#include "SpeakerSetupLine.hpp"
#include <Data/sg_LogicStrucs.hpp>
#include <StructGRIS/ValueTreeUtilities.hpp>

namespace gris
{
/**
 * @class gris::SpeakerSetupContainer
 * @brief A UI component for managing and displaying speaker setups in a tree view.
 *
 * This class provides a graphical interface for editing, selecting, and organizing speaker setups.
 * It supports drag-and-drop, undo/redo, sorting, and selection callbacks. The container interacts
 * with a JUCE ValueTree for data management and provides utility functions for manipulating speaker
 * groups and individual speakers.
 *
 * @see SpeakerSetupLine
 */
class SpeakerSetupContainer final
    : public juce::Component
    , public juce::DragAndDropContainer
    , private juce::Timer
{
public:
    SpeakerSetupContainer(const juce::File & speakerSetupXmlFile,
                          juce::ValueTree theSpeakerSetupVt,
                          juce::UndoManager & undoMan,
                          std::function<void()> selectionChanged);

    ~SpeakerSetupContainer () override
    {
        speakerSetupTreeView.setRootItem (nullptr);
    }

    void reload(juce::ValueTree theSpeakerSetupVt);

    void resized () override;

    void deleteSelectedItems ();

    bool keyPressed (const juce::KeyPress& key) override;

    /** returns either the selected item or the last item*/
    juce::ValueTree getSelectedItem();

    void getSelectedTreeViewItems(juce::OwnedArray<juce::ValueTree> & items)
    {
        SpeakerSetupLine::getSelectedTreeViewItems(speakerSetupTreeView, items);
    }

    juce::Array<output_patch_t> getSelectedSpeakers();

    void selectSpeaker (tl::optional<output_patch_t> const outputPatch);

    std::pair<juce::ValueTree, int> getParentAndIndexOfSelectedItem();

    std::pair<juce::ValueTree, int> getMainSpeakerGroupAndIndex ();
    std::pair<juce::ValueTree, int> getCurSpeakerGroupAndIndex ();

    void addValueTreeListener(juce::ValueTree::Listener * listener) { speakerSetupVt.addListener(listener); }
    void setSpatMode(SpatMode spatMode) { speakerSetupVt.setProperty(SPAT_MODE, spatModeToString(spatMode), &undoManager); }

    bool isDeletingGroup() { return SpeakerSetupLine::isDeletingGroup; }

private:
    GrisLookAndFeel grisLookAndFeel;
    juce::String speakerSetupFileName;
    juce::ValueTree speakerSetupVt;

    //header
    juce::Label id, x, y, z, azim, elev, radius, gain, highpass, direct, del, drag;

    juce::TreeView speakerSetupTreeView;
    juce::TextButton undoButton{ "Undo" }, redoButton{ "Redo" }, sortButton{ "Sort by ID" };

    std::unique_ptr<SpeakerSetupLine> mainSpeakerGroupLine;

    juce::UndoManager& undoManager;
    std::function<void ()> onSelectionChanged;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerSetupContainer)
};

}
