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
#include <JuceHeader.h>

namespace gris
{
using namespace juce;

//==============================================================================

inline Colour getUIColourIfAvailable (LookAndFeel_V4::ColourScheme::UIColour uiColour, Colour fallback = Colour (0xff4d4d4d)) noexcept
{
    if (auto* v4 = dynamic_cast<LookAndFeel_V4*> (&LookAndFeel::getDefaultLookAndFeel ()))
        return v4->getCurrentColourScheme ().getUIColour (uiColour);

    return fallback;
}

juce::ValueTree convertSpeakerSetup (const juce::ValueTree& oldSpeakerSetup);

class SpeakerTreeItemComponent : public juce::Component
{
public:
    SpeakerTreeItemComponent (const ValueTree& v);

    void resized () override
    {
        auto bounds {getLocalBounds()};
        constexpr auto colW {60};

        for (auto* component : { &id, &x, &y, &z, &azim, &elev, &distance, &gain, &highpass, &direct, &del })
            component->setBounds (bounds.removeFromLeft (colW));
    }

protected:
    void setupEditor(juce::Label & editor, juce::StringRef text)
    {
        editor.setText(text, dontSendNotification);
        editor.setEditable(true);
        addAndMakeVisible(editor);
    };

    juce::Label id, x, y, z, azim, elev, distance, gain, highpass, direct, del;

    ValueTree vt;

};

//==============================================================================

class SpeakerGroupComponent : public SpeakerTreeItemComponent
{
public:
    SpeakerGroupComponent (const ValueTree& v);
};

//==============================================================================

class SpeakerComponent : public SpeakerTreeItemComponent
{
public:
    SpeakerComponent (const ValueTree& v);
};

//==============================================================================

class SpeakerSetupLine final
    : public TreeViewItem
    , private ValueTree::Listener
{
public:
    SpeakerSetupLine (const ValueTree& v, UndoManager& um)
        : valueTree (v), undoManager (um)
    {
        valueTree.addListener (this);
    }

    String getUniqueName () const override
    {
        return valueTree.getType().toString ();
    }

    bool mightContainSubItems () override
    {
        return valueTree.getNumChildren () > 0;
    }

#if 1
    std::unique_ptr<Component> createItemComponent () override
    {
        if (mightContainSubItems ())
            return std::make_unique<SpeakerGroupComponent>(valueTree);
        else
            return std::make_unique<SpeakerComponent> (valueTree);
    }
#else
    void paintItem(Graphics & g, int width, int height) override
    {
        if (isSelected())
            g.fillAll(getUIColourIfAvailable(LookAndFeel_V4::ColourScheme::UIColour::highlightedFill, Colours::teal));

        g.setColour(getUIColourIfAvailable(LookAndFeel_V4::ColourScheme::UIColour::defaultText, Colours::black));
        g.setFont(15.0f);

        g.drawText(treeView.getType().toString(), 4, 0, width - 4, height, Justification::centredLeft, true);
    }
#endif

    void itemOpennessChanged (bool isNowOpen) override
    {
        if (isNowOpen && getNumSubItems () == 0)
            refreshSubItems ();
        else
            clearSubItems ();
    }

    var getDragSourceDescription () override
    {
        return "Drag Demo";
    }

    bool isInterestedInDragSource (const DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        return dragSourceDetails.description == "Drag Demo";
    }

    void itemDropped (const DragAndDropTarget::SourceDetails&, int insertIndex) override
    {
        OwnedArray<ValueTree> selectedTrees;
        getSelectedTreeViewItems (*getOwnerView (), selectedTrees);

        moveItems (*getOwnerView (), selectedTrees, valueTree, insertIndex, undoManager);
    }

    static void moveItems (TreeView& treeView, const OwnedArray<ValueTree>& items,
                           ValueTree newParent, int insertIndex, UndoManager& undoManager)
    {
        if (items.size () > 0)
        {
            std::unique_ptr<XmlElement> oldOpenness (treeView.getOpennessState (false));

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

    static void getSelectedTreeViewItems (TreeView& treeView, OwnedArray<ValueTree>& items)
    {
        auto numSelected = treeView.getNumSelectedItems ();

        for (int i = 0; i < numSelected; ++i)
            if (auto* vti = dynamic_cast<SpeakerSetupLine*> (treeView.getSelectedItem (i)))
                items.add (new ValueTree (vti->valueTree));
    }

private:
    ValueTree valueTree;
    UndoManager& undoManager;

    void refreshSubItems ()
    {
        clearSubItems ();

        for (int i = 0; i < valueTree.getNumChildren (); ++i)
            addSubItem (new SpeakerSetupLine (valueTree.getChild (i), undoManager));
    }

    void valueTreePropertyChanged (ValueTree&, const Identifier&) override
    {
        repaintItem ();
    }

    void valueTreeChildAdded (ValueTree& parentTree, ValueTree&) override { treeChildrenChanged (parentTree); }
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree&, int) override { treeChildrenChanged (parentTree); }
    void valueTreeChildOrderChanged (ValueTree& parentTree, int, int) override { treeChildrenChanged (parentTree); }
    void valueTreeParentChanged (ValueTree&) override {}

    void treeChildrenChanged (const ValueTree& parentTree)
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

//==============================================================================
class SpeakerSetupContainer final : public Component,
    public DragAndDropContainer,
    private Timer
{
public:
    SpeakerSetupContainer ();

    ~SpeakerSetupContainer () override
    {
        treeView.setRootItem (nullptr);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getUIColourIfAvailable (LookAndFeel_V4::ColourScheme::UIColour::windowBackground));
    }

    void resized () override
    {
        auto r = getLocalBounds ().reduced (8);

        auto buttons = r.removeFromBottom (22);
        undoButton.setBounds (buttons.removeFromLeft (100));
        buttons.removeFromLeft (6);
        redoButton.setBounds (buttons.removeFromLeft (100));

        r.removeFromBottom (4);
        treeView.setBounds (r);
    }

    void deleteSelectedItems ()
    {
        OwnedArray<ValueTree> selectedItems;
        SpeakerSetupLine::getSelectedTreeViewItems (treeView, selectedItems);

        for (auto* v : selectedItems)
        {
            if (v->getParent ().isValid ())
                v->getParent ().removeChild (*v, &undoManager);
        }
    }

    bool keyPressed (const KeyPress& key) override
    {
        if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey)
        {
            deleteSelectedItems ();
            return true;
        }

        if (key == KeyPress ('z', ModifierKeys::commandModifier, 0))
        {
            undoManager.undo ();
            return true;
        }

        if (key == KeyPress ('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0))
        {
            undoManager.redo ();
            return true;
        }

        return Component::keyPressed (key);
    }

private:
    TreeView treeView;
    TextButton undoButton { "Undo" }, redoButton { "Redo" };

    std::unique_ptr<SpeakerSetupLine> rootItem;
    UndoManager undoManager;

    void timerCallback () override
    {
        undoManager.beginNewTransaction ();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerSetupContainer)
};

//========================================================================

class MainContentComponent;
class GrisLookAndFeel;

class SpeakerSetupWindow final : public juce::DocumentWindow
{
public:
    SpeakerSetupWindow () = delete;
    SpeakerSetupWindow (juce::String const& name,
                         GrisLookAndFeel& lookAndFeel,
                         MainContentComponent& mainContentComponent);

    void closeButtonPressed () override;

private:
    MainContentComponent& mMainContentComponent;
    GrisLookAndFeel& mLookAndFeel;
    SpeakerSetupContainer mValueTreesDemo;
};
}
