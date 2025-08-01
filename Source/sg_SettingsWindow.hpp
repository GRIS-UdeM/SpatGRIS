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
#include "sg_SpeakerViewComponent.hpp"

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
    static inline const juce::String localhost = "127.0.0.1";
    static constexpr int maxUDPPort = 65535;
    static inline const juce::String maxUDPPortString = "65535";
    static constexpr int minUDPPort = 1024;
    static inline const juce::String minUDPPortString = "1024";

    juce::StringArray mInputDevices{};
    juce::StringArray mOutputDevices{};

    MainContentComponent & mMainContentComponent;
    SpeakerViewComponent & mSVComponent;
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
    juce::Label mSpatNetworkSettings{ "", "Spatialisation Data Network Settings" };

    juce::Label mOscInputPortLabel{ "", "OSC Input Port :" };
    juce::TextEditor mOscInputPortTextEditor{};


    juce::Label mSpeakerViewNetworkSettings{ "", "Standalone SpeakerView Network Settings :" };

    juce::Label mSpeakerViewInputPortLabel{ "", "UDP Input Port :" };
    juce::TextEditor mSpeakerViewInputPortTextEditor{};

    juce::Label mSpeakerViewOutputAddressLabel{ "", "UDP Output Address :" };
    juce::TextEditor mSpeakerViewOutputAddressTextEditor{};

    juce::Label mSpeakerViewOutputPortLabel{ "", "UDP Output Port :" };
    juce::TextEditor mSpeakerViewOutputPortTextEditor{};

    juce::TextButton mSaveSettingsButton;

public:
    int mInitialOSCPort;
    /**
     * UDP input port for an extra networked SpeakerView
     */
    tl::optional<int> mInitialExtraUDPInputPort;
    /**
     * UDP output port for an extra networked SpeakerView
     */
    tl::optional<int> mInitialExtraUDPOutputPort;
    /**
     * IP address for an extra networked SpeakerView
     */
    tl::optional<juce::String> mInitialExtraUDPOutputAddress;

    //==============================================================================
    SettingsComponent(MainContentComponent & parent, SpeakerViewComponent & sVComponent, GrisLookAndFeel & lookAndFeel);
    //==============================================================================
    SettingsComponent() = delete;
    ~SettingsComponent() override;
    SG_DELETE_COPY_AND_MOVE(SettingsComponent)
    //==============================================================================

    void buttonClicked(juce::Button * button) override;

    void textEditorFocusLost(juce::TextEditor & text_editor) override;

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
    SettingsWindow(MainContentComponent & parent, SpeakerViewComponent & sVComponent, GrisLookAndFeel & grisLookAndFeel);
    //==============================================================================
    SettingsWindow() = delete;
    ~SettingsWindow() override = default;
    SG_DELETE_COPY_AND_MOVE(SettingsWindow)
    //==============================================================================
    void closeButtonPressed() override;
    bool keyPressed(const juce::KeyPress & key) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SettingsWindow)
}; // class PropertiesWindow

} // namespace gris
