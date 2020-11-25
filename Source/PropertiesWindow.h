/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class PropertiesComponent final
    : public juce::Component
    , public juce::TextButton::Listener
{
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;

    juce::Label mGeneralLabel{ "", "General Settings" };
    juce::Label mJackSettingsLabel{ "", "Jack Settings" };
    juce::Label mRecordingLabel{ "", "Recording Settings" };
    juce::Label mCubeDistanceLabel{ "", "CUBE Distance Settings" };

    juce::Label mOscInputPortLabel{ "", "OSC Input Port :" };
    std::unique_ptr<juce::TextEditor> mOscInputPortTextEditor{};

    juce::Label mDeviceLabel{ "", "Output Device :" };
    std::unique_ptr<juce::ComboBox> mDeviceCombo;

    juce::Label mRateLabel{ "", "Sampling Rate (hz) :" };
    std::unique_ptr<juce::ComboBox> mRateCombo;

    juce::Label mBufferLabel{ "", "Buffer Size (spls) :" };
    std::unique_ptr<juce::ComboBox> mBufferCombo;

    juce::Label mRecFormatLabel{ "", "File Format :" };
    std::unique_ptr<juce::ComboBox> mRecFormatCombo;

    juce::Label mRecFileConfigLabel{ "", "Output Format :" };
    std::unique_ptr<juce::ComboBox> mRecFileConfigCombo;

    juce::Label mDistanceDbLabel{ "", "Attenuation (dB) :" };
    std::unique_ptr<juce::ComboBox> mDistanceDbCombo;

    juce::Label mDistanceCutoffLabel{ "", "Attenuation (Hz) :" };
    std::unique_ptr<juce::ComboBox> mDistanceCutoffCombo;

    std::unique_ptr<juce::TextButton> mValidSettingsButton;

public:
    //==============================================================================
    PropertiesComponent(MainContentComponent & parent,
                        GrisLookAndFeel & lookAndFeel,
                        juce::Array<juce::String> const & devices,
                        juce::String const & currentDevice,
                        int indR,
                        int indB,
                        int indFF,
                        int indFC,
                        int indAttDB,
                        int indAttHz,
                        int oscPort);
    //==============================================================================
    PropertiesComponent() = delete;
    ~PropertiesComponent() override = default;

    PropertiesComponent(PropertiesComponent const &) = delete;
    PropertiesComponent(PropertiesComponent &&) = delete;

    PropertiesComponent & operator=(PropertiesComponent const &) = delete;
    PropertiesComponent & operator=(PropertiesComponent &&) = delete;
    //==============================================================================
    juce::TextEditor * createPropIntTextEditor(juce::String const & tooltip, int ypos, int init);
    juce::ComboBox * createPropComboBox(juce::StringArray const & choices, int selected, int ypos);

    void buttonClicked(juce::Button * button) override;
    void closeButtonPressed();

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(PropertiesComponent)
}; // class PropertiesComponent

//==============================================================================
class PropertiesWindow final : public juce::DocumentWindow
{
    MainContentComponent & mainContentComponent;
    PropertiesComponent propertiesComponent;

public:
    //==============================================================================
    PropertiesWindow(MainContentComponent & parent,
                     GrisLookAndFeel & feel,
                     juce::Array<juce::String> const & devices,
                     juce::String const & currentDevice,
                     int indR = 0,
                     int indB = 0,
                     int indFF = 0,
                     int indFC = 0,
                     int indAttDB = 2,
                     int indAttHz = 3,
                     int oscPort = 18032);
    //==============================================================================
    PropertiesWindow() = delete;
    ~PropertiesWindow() override = default;

    PropertiesWindow(PropertiesWindow const &) = delete;
    PropertiesWindow(PropertiesWindow &&) = delete;

    PropertiesWindow & operator=(PropertiesWindow const &) = delete;
    PropertiesWindow & operator=(PropertiesWindow &&) = delete;
    //==============================================================================
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(PropertiesWindow)
}; // class PropertiesWindow
