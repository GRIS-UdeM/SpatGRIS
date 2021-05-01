/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "Configuration.h"

class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class SettingsComponent final
    : public juce::Component
    , public juce::TextButton::Listener
    , public juce::ComboBox::Listener
    , public juce::TextEditor::Listener
{
    static constexpr auto PADDING = 20;
    static constexpr auto LEFT_COL_WIDTH = 150;
    static constexpr auto RIGHT_COL_WIDTH = 150;

    static constexpr auto LEFT_COL_START = PADDING;
    static constexpr auto RIGHT_COL_START = LEFT_COL_START + LEFT_COL_WIDTH + PADDING;

    static constexpr auto COMPONENT_HEIGHT = 22;

    static constexpr auto LINE_SKIP = 30;
    static constexpr auto SECTION_SKIP = 50;

    juce::StringArray mInputDevices{};
    juce::StringArray mOutputDevices{};

    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;

    //==============================================================================
    juce::Label mAudioSectionLabel{ "", "Audio Settings" };

    juce::Label mDeviceTypeLabel{ "", "Audio device type :" };
    juce::ComboBox mDeviceTypeCombo{};

    juce::Label mInputDeviceLabel{ "", "Audio input device :" };
    juce::ComboBox mInputDeviceCombo{};

    juce::Label mOutputDeviceLabel{ "", "Audio output device :" };
    juce::ComboBox mOutputDeviceCombo{};

    juce::Label mSampleRateLabel{ "", "Sampling Rate (hz) :" };
    juce::ComboBox mSampleRateCombo;

    juce::Label mBufferSize{ "", "Buffer Size (spls) :" };
    juce::ComboBox mBufferSizeCombo;

    //==============================================================================
    juce::Label mGeneralSectionLabel{ "", "General Settings" };

    juce::Label mOscInputPortLabel{ "", "OSC Input Port :" };
    juce::TextEditor mOscInputPortTextEditor{};

    juce::Label mRecFormatLabel{ "", "File Format :" };
    juce::ComboBox mRecFormatCombo;

    juce::Label mRecFileConfigLabel{ "", "Output Format :" };
    juce::ComboBox mRecFileTypeCombo;

    //==============================================================================
    juce::Label mCubeSectionLabel{ "", "CUBE Distance Settings" };

    juce::Label mDistanceDbLabel{ "", "Attenuation (dB) :" };
    juce::ComboBox mDistanceDbCombo;

    juce::Label mDistanceCutoffLabel{ "", "Attenuation (Hz) :" };
    juce::ComboBox mDistanceCutoffCombo;

    juce::TextButton mSaveSettingsButton;

public:
    //==============================================================================
    SettingsComponent(MainContentComponent & parent,
                      RecordingOptions const & recordingOptions,
                      LbapDistanceAttenuationData const & lbapData,
                      int oscPort,
                      GrisLookAndFeel & lookAndFeel);
    //==============================================================================
    SettingsComponent() = delete;
    ~SettingsComponent() override = default;

    SettingsComponent(SettingsComponent const &) = delete;
    SettingsComponent(SettingsComponent &&) = delete;

    SettingsComponent & operator=(SettingsComponent const &) = delete;
    SettingsComponent & operator=(SettingsComponent &&) = delete;
    //==============================================================================

    void buttonClicked(juce::Button * button) override;

    void placeComponents();

private:
    //==============================================================================
    void fillComboBoxes();
    //==============================================================================
    JUCE_LEAK_DETECTOR(SettingsComponent)
public:
    void comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged) override;
}; // class PropertiesComponent

//==============================================================================
class SettingsWindow final : public juce::DocumentWindow
{
    MainContentComponent & mMainContentComponent;
    SettingsComponent mPropertiesComponent;

public:
    //==============================================================================
    SettingsWindow(MainContentComponent & parent,
                   RecordingOptions const & recordingOptions,
                   LbapDistanceAttenuationData const & lbapData,
                   int oscPort,
                   GrisLookAndFeel & grisLookAndFeel);
    //==============================================================================
    SettingsWindow() = delete;
    ~SettingsWindow() override = default;

    SettingsWindow(SettingsWindow const &) = delete;
    SettingsWindow(SettingsWindow &&) = delete;

    SettingsWindow & operator=(SettingsWindow const &) = delete;
    SettingsWindow & operator=(SettingsWindow &&) = delete;
    //==============================================================================
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SettingsWindow)
}; // class PropertiesWindow
