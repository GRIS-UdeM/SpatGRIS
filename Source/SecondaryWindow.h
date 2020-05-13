/*
 This file is part of SpatGRIS2.
 
 Developers: Samuel Béland
 
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

#ifndef SECONDARYWINDOW_H
#define SECONDARYWINDOW_H

#include "../JuceLibraryCode/JuceHeader.h"

class SecondaryWindow : public juce::DocumentWindow
{
public:
    using juce::DocumentWindow::DocumentWindow;

    void closeButtonPressed() override { delete this; }
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SecondaryWindow);
};

#endif // SECONDARYWINDOW_H
