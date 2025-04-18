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
    SpeakerSetupContainer ();

    ~SpeakerSetupContainer () override
    {
        treeView.setRootItem (nullptr);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getUIColourIfAvailable (juce::LookAndFeel_V4::ColourScheme::UIColour::windowBackground));
    }

    void resized () override
    {
        auto r = getLocalBounds ().reduced (8);

        auto buttons = r.removeFromBottom (22);
        undoButton.setBounds (buttons.removeFromLeft (100));
        buttons.removeFromLeft (6);
        redoButton.setBounds (buttons.removeFromLeft (100));
        buttons.removeFromLeft (6);
        sortButton.setBounds (buttons.removeFromLeft (100));

        r.removeFromBottom (4);
        treeView.setBounds (r);
    }

    void deleteSelectedItems ()
    {
        juce::OwnedArray<juce::ValueTree> selectedItems;
        SpeakerSetupLine::getSelectedTreeViewItems (treeView, selectedItems);

        for (auto* v : selectedItems)
        {
            if (v->getParent ().isValid ())
                v->getParent ().removeChild (*v, &undoManager);
        }
    }

    bool keyPressed (const juce::KeyPress& key) override
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

private:
    juce::TreeView treeView;
    juce::TextButton undoButton { "Undo" }, redoButton { "Redo" }, sortButton { "Sort" };

    std::unique_ptr<SpeakerSetupLine> rootItem;
    juce::UndoManager undoManager;

    void timerCallback () override
    {
        undoManager.beginNewTransaction ();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerSetupContainer)
};

}