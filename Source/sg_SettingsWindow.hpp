/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_Configuration.hpp"

namespace gris
{
class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class SettingsComponent final
    : public juce::Component
    , public juce::TextButton::Listener
    , public juce::ComboBox::Listener
    , public juce::TextEditor::Listener
{
    juce::StringArray mInputDevices{};
    juce::StringArray mOutputDevices{};

    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;

    int mOscPortWhenLoaded;

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

    juce::TextButton mSaveSettingsButton;

public:
    //==============================================================================
    SettingsComponent(MainContentComponent & parent, int oscPort, GrisLookAndFeel & lookAndFeel);
    //==============================================================================
    SettingsComponent() = delete;
    ~SettingsComponent() override;
    SG_DELETE_COPY_AND_MOVE(SettingsComponent)
    //==============================================================================

    void buttonClicked(juce::Button * button) override;

    void placeComponents();

private:
    //==============================================================================
    void fillComboBoxes();
    bool isSelectedAudioDeviceActive();
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
    SettingsWindow(MainContentComponent & parent, int oscPort, GrisLookAndFeel & grisLookAndFeel);
    //==============================================================================
    SettingsWindow() = delete;
    ~SettingsWindow() override = default;
    SG_DELETE_COPY_AND_MOVE(SettingsWindow)
    //==============================================================================
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SettingsWindow)
}; // class PropertiesWindow

} // namespace gris