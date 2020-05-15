/*
 This file is part of SpatGRIS2.
 
 Developers: Samuel Béland, Nicolas Masson
 
 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

//==============================================================================

#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent;
class GrisLookAndFeel;

class AboutWindow final 
    : public juce::DocumentWindow
    , public juce::TextButton::Listener
{
public:
    AboutWindow(const String& name, Colour backgroundColour, int buttonsNeeded,
                MainContentComponent *parent, GrisLookAndFeel *feel);
    ~AboutWindow() final;
    //==============================================================================
    void buttonClicked(Button *button) { delete this; }
    void closeButtonPressed() { delete this; }
private:
    //==============================================================================
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    ImageComponent *imageComponent;
    Label *title;
    Label *version;
    Label *label;
    HyperlinkButton *website;
    TextButton *close;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutWindow);
};

#endif // ABOUTWINDOW_H
