/*
 This file is part of ServerGris.
 
 Developers: Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WinControl_h
#define WinControl_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "GrisLookAndFeel.h"

class MainContentComponent;
class Input;

using namespace std;

class WinControl :   public DocumentWindow,
private Timer

{
public:
    WinControl(const String& name, Colour backgroundColour, int buttonsNeeded,MainContentComponent * parent, GrisLookAndFeel * feel);
    ~WinControl();
    
    void setTimerHz(int hz);
    
    void timerCallback() override;
    void paint (Graphics& g) override;
    void resized() override;
    void closeButtonPressed() override;
    
private:
    void drawAzimElevSource(Graphics &g,  Input *  it, const int fieldWH, const int fieldCenter);
    
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WinControl)
};

#endif /* WinControl_h */
