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

#ifndef PROPERTIESWINDOW_H
#define PROPERTIESWINDOW_H

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class PropertiesComponent : public juce::Component,
                            public TextButton::Listener
{
public:
    PropertiesComponent( MainContentComponent& parent,
                         GrisLookAndFeel& lookAndFeel,
                         Array<String> const& devices,
                         String const& currentDevice,
                         int indR,
                         int indB,
                         int indFF,
                         int indFC,
                         int indAttDB,
                         int indAttHz,
                         int oscPort );
    ~PropertiesComponent() = default;
    //==============================================================================
    Label * createPropLabel(juce::String const& lab, Justification::Flags just, int ypos, int width = 100);
    TextEditor * createPropIntTextEditor(juce::String const& tooltip, int ypos, int init);
    ComboBox * createPropComboBox(juce::StringArray const& choices, int selected, int ypos);

    void buttonClicked(Button *button);
    void closeButtonPressed();
private:
    //==============================================================================
    MainContentComponent& mainContentComponent;
    GrisLookAndFeel& grisFeel;

    std::unique_ptr<Label> generalLabel;
    std::unique_ptr<Label> jackSettingsLabel;
    std::unique_ptr<Label> recordingLabel;
    std::unique_ptr<Label> cubeDistanceLabel;

    std::unique_ptr<Label> labOSCInPort;
    std::unique_ptr<TextEditor> tedOSCInPort;

    std::unique_ptr<Label> labDevice;
    std::unique_ptr<ComboBox> cobDevice;

    std::unique_ptr<Label> labRate;
    std::unique_ptr<ComboBox> cobRate;

    std::unique_ptr<Label> labBuff;
    std::unique_ptr<ComboBox> cobBuffer;

    std::unique_ptr<Label> labRecFormat;
    std::unique_ptr<ComboBox> recordFormat;

    std::unique_ptr<Label> labRecFileConfig;
    std::unique_ptr<ComboBox> recordFileConfig;

    std::unique_ptr<Label> labDistanceDB;
    std::unique_ptr<ComboBox> cobDistanceDB;

    std::unique_ptr<Label> labDistanceCutoff;
    std::unique_ptr<ComboBox> cobDistanceCutoff;

    std::unique_ptr<TextButton> butValidSettings;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropertiesComponent);
};

//==============================================================================
class PropertiesWindow : public juce::DocumentWindow
{
    MainContentComponent& mainContentComponent;
    PropertiesComponent propertiesComponent;
public:
    //==============================================================================
    PropertiesWindow( MainContentComponent& parent,
                      GrisLookAndFeel& feel, Array<String> const & devices,
                      String const & currentDevice,
                      int indR     = 0,
                      int indB     = 0,
                      int indFF    = 0,
                      int indFC    = 0,
                      int indAttDB = 2,
                      int indAttHz = 3,
                      int oscPort  = 18032 );
    ~PropertiesWindow() = default;
    //==============================================================================
    void closeButtonPressed() override;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropertiesWindow);
};

#endif // PROPERTIESWINDOW_H
