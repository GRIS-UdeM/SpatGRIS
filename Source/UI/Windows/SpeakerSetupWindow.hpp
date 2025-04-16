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

class ValueTreeItem final : public TreeViewItem,
    private ValueTree::Listener
{
public:
    ValueTreeItem (const ValueTree& v, UndoManager& um)
        : tree (v), undoManager (um)
    {
        tree.addListener (this);
    }

    String getUniqueName () const override
    {
        return tree["name"].toString ();
    }

    bool mightContainSubItems () override
    {
        return tree.getNumChildren () > 0;
    }

    void paintItem (Graphics& g, int width, int height) override
    {
        if (isSelected ())
            g.fillAll (getUIColourIfAvailable (LookAndFeel_V4::ColourScheme::UIColour::highlightedFill,
                                               Colours::teal));

        g.setColour (getUIColourIfAvailable (LookAndFeel_V4::ColourScheme::UIColour::defaultText,
                                             Colours::black));
        g.setFont (15.0f);

        g.drawText (tree["name"].toString (),
                    4, 0, width - 4, height,
                    Justification::centredLeft, true);
    }

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

        moveItems (*getOwnerView (), selectedTrees, tree, insertIndex, undoManager);
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
            if (auto* vti = dynamic_cast<ValueTreeItem*> (treeView.getSelectedItem (i)))
                items.add (new ValueTree (vti->tree));
    }

private:
    ValueTree tree;
    UndoManager& undoManager;

    void refreshSubItems ()
    {
        clearSubItems ();

        for (int i = 0; i < tree.getNumChildren (); ++i)
            addSubItem (new ValueTreeItem (tree.getChild (i), undoManager));
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
        if (parentTree == tree)
        {
            refreshSubItems ();
            treeHasChanged ();
            setOpen (true);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueTreeItem)
};

//==============================================================================
class ValueTreesDemo final : public Component,
    public DragAndDropContainer,
    private Timer
{
public:
    ValueTreesDemo ()
    {
        addAndMakeVisible (tree);

        tree.setTitle ("ValueTree");
        tree.setDefaultOpenness (true);
        tree.setMultiSelectEnabled (true);
        rootItem.reset (new ValueTreeItem (createRootValueTree (), undoManager));
        tree.setRootItem (rootItem.get ());

        addAndMakeVisible (undoButton);
        addAndMakeVisible (redoButton);
        undoButton.onClick = [this] { undoManager.undo (); };
        redoButton.onClick = [this] { undoManager.redo (); };

        startTimer (500);

        setSize (500, 500);
    }

    ~ValueTreesDemo () override
    {
        tree.setRootItem (nullptr);
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
        tree.setBounds (r);
    }

    static ValueTree createTree (const String& desc)
    {
        ValueTree t ("Item");
        t.setProperty ("name", desc, nullptr);
        return t;
    }

    static ValueTree createRootValueTree ()
    {
        auto vt = createTree ("This demo displays a ValueTree as a treeview.");
        vt.appendChild (createTree ("You can drag around the nodes to rearrange them"), nullptr);
        vt.appendChild (createTree ("..and press 'delete' or 'backspace' to delete them"), nullptr);
        vt.appendChild (createTree ("Then, you can use the undo/redo buttons to undo these changes"), nullptr);

        int n = 1;
        vt.appendChild (createRandomTree (n, 0), nullptr);

        return vt;
    }

    static ValueTree createRandomTree (int& counter, int depth)
    {
        auto t = createTree ("Item " + String (counter++));

        if (depth < 3)
            for (int i = 1 + Random::getSystemRandom ().nextInt (7); --i >= 0;)
                t.appendChild (createRandomTree (counter, depth + 1), nullptr);

        return t;
    }

    void deleteSelectedItems ()
    {
        OwnedArray<ValueTree> selectedItems;
        ValueTreeItem::getSelectedTreeViewItems (tree, selectedItems);

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
    TreeView tree;
    TextButton undoButton { "Undo" },
        redoButton { "Redo" };

    std::unique_ptr<ValueTreeItem> rootItem;
    UndoManager undoManager;

    void timerCallback () override
    {
        undoManager.beginNewTransaction ();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueTreesDemo)
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

    //void resized() override
    //{
    //    auto r = getLocalBounds();
    //    r.removeFromTop(getTitleBarHeight());
    //    r.removeFromBottom(10/*getBottomBorderSize()*/);
    //    mValueTreesDemo.setBounds(r);
    //}

private:
    MainContentComponent& mMainContentComponent;
    GrisLookAndFeel& mLookAndFeel;
    ValueTreesDemo mValueTreesDemo;
};
}
