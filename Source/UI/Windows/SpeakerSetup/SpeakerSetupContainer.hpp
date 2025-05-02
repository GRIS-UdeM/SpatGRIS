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
inline juce::Colour getUIColourIfAvailable (juce::LookAndFeel_V4::ColourScheme::UIColour uiColour,
                                            juce::Colour fallback = juce::Colour (0xff4d4d4d)) noexcept
{
    if (auto* v4 = dynamic_cast<juce::LookAndFeel_V4*> (&juce::LookAndFeel::getDefaultLookAndFeel ()))
        return v4->getCurrentColourScheme ().getUIColour (uiColour);

    return fallback;
}

//===============================

class SpeakerSetupContainer final : public juce::Component,
    public juce::DragAndDropContainer,
    private juce::Timer
{
public:
    SpeakerSetupContainer (const SpeakerSetup& setup, const juce::File& speakerSetupXmlFile);

    ~SpeakerSetupContainer () override
    {
        speakerSetupTreeView.setRootItem (nullptr);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getUIColourIfAvailable (juce::LookAndFeel_V4::ColourScheme::UIColour::windowBackground));
    }

    void resized () override;

    void deleteSelectedItems ();

    bool keyPressed (const juce::KeyPress& key) override;

    void saveSpeakerSetup();

    juce::ValueTree getSelectedItem();

private:
    //TODO VB: this can't be const lol
    const SpeakerSetup& speakerSetup;
    juce::File vtFile;
    juce::ValueTree vt;

    juce::TreeView speakerSetupTreeView;
    juce::TextButton undoButton { "Undo" }, redoButton { "Redo" }, sortButton { "Sort" }, saveButton { "Save" };

    std::unique_ptr<SpeakerSetupLine> mainSpeakerGroupLine;
    juce::UndoManager undoManager;

    void timerCallback () override
    {
        undoManager.beginNewTransaction ();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerSetupContainer)
};

}