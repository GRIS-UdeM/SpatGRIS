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

#include "../../ValueTreeUtilities.hpp"
#include "SpeakerSetupLine.hpp"
#include <Data/sg_LogicStrucs.hpp>

namespace gris
{
class SpeakerSetupContainer final : public juce::Component,
    public juce::DragAndDropContainer,
    private juce::Timer
{
public:
    SpeakerSetupContainer(const juce::File & speakerSetupXmlFile,
                          juce::UndoManager & undoMan);

    ~SpeakerSetupContainer () override
    {
        setLookAndFeel (nullptr);
        speakerSetupTreeView.setRootItem (nullptr);

        //TODO VB: delete lnf from all labels. Need a way to iterate through those components
    }

    void resized () override;

    void deleteSelectedItems ();

    bool keyPressed (const juce::KeyPress& key) override;

    void saveSpeakerSetup();

    juce::ValueTree getSelectedItem();

    //juce::ValueTree getSpeakerSetupVt() { return vt; }
    void addValueTreeListener(juce::ValueTree::Listener * listener) { vt.addListener(listener); }
    void setSpatMode(SpatMode spatMode) { vt.setProperty(SPAT_MODE, spatModeToString(spatMode), &undoManager); }

private:
    GrisLookAndFeel lookAndFeel;
    juce::File vtFile;
    juce::ValueTree vt;

    //header
    juce::Label id, x, y, z, azim, elev, radius, gain, highpass, direct, del, drag;

    juce::TreeView speakerSetupTreeView;
    juce::TextButton undoButton { "Undo" }, redoButton { "Redo" }, sortButton { "Sort" }, saveButton { "Save" };

    std::unique_ptr<SpeakerSetupLine> mainSpeakerGroupLine;

    void timerCallback () override
    {
        undoManager.beginNewTransaction ();
    }

    juce::UndoManager& undoManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerSetupContainer)
};

}