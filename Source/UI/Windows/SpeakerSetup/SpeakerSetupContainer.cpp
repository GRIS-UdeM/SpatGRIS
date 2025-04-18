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

#include "SpeakerSetupContainer.hpp"
namespace gris
{
SpeakerSetupContainer::SpeakerSetupContainer ()
{
#if JUCE_LINUX
    const auto vtFile = juce::File ("/home/vberthiaume/Documents/git/sat/GRIS/SpatGRIS/Resources/templates/Speaker setups/DOME/Dome124(64-20-20-20)Subs2.xml");
#else
    const auto vtFile = juce::File ("C:/Users/barth/Documents/git/sat/GRIS/SpatGRIS/Resources/templates/Speaker setups/DOME/Dome124(64-20-20-20)Subs2.xml");
#endif
    const auto vt { convertSpeakerSetup (juce::ValueTree::fromXml (vtFile.loadFileAsString ())) };

    addAndMakeVisible (treeView);

    treeView.setTitle (vtFile.getFileName ());
    treeView.setDefaultOpenness (true);
    treeView.setMultiSelectEnabled (true);

    rootItem.reset (new SpeakerSetupLine (vt, undoManager));
    treeView.setRootItem (rootItem.get ());

    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);
    addAndMakeVisible (sortButton);
    undoButton.onClick = [this] { undoManager.undo (); };
    redoButton.onClick = [this] { undoManager.redo (); };
    sortButton.onClick = [this] { rootItem->sort (); };

    startTimer (500);

    setSize (500, 500);
}
}
