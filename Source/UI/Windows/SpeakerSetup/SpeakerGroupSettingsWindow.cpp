#include "SpeakerGroupSettingsWindow.hpp"
#include "Data/sg_LogicStrucs.hpp"

namespace gris
{

SpeakerGroupSettingsWindow::SpeakerGroupSettingsWindow(SpeakerGroupComponent& parent)
    : DocumentWindow("Group Rotation Settings", lookAndFeel.getBackgroundColour(), allButtons),
      parent(parent),
      settingsComponent(parent, lookAndFeel)
{
    setAlwaysOnTop(true);
    setContentNonOwned(&settingsComponent, true);
    setResizable(false, false);
    setUsingNativeTitleBar(true);
    centreWithSize(200, 200);
    setVisible(true);
}

void SpeakerGroupSettingsWindow::closeButtonPressed()
{
  parent.closeSettingsWindow();
}

SpeakerGroupSettingsComponent::SpeakerGroupSettingsComponent(SpeakerGroupComponent& sgc, GrisLookAndFeel& glaf)
    : speakerGroupComponent(sgc),
      lookAndFeel(glaf)
{
    auto initLabel = [this](juce::Label & label, juce::Slider & associatedSlider, juce::StringRef text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::Flags::centredRight);
        label.setFont(lookAndFeel.getFont());
        label.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
        label.setLookAndFeel(&lookAndFeel);
        addAndMakeVisible(label);
        label.attachToComponent (&associatedSlider, true);
    };
    auto initSlider = [this](juce::Slider & slider) {
        slider.setRange (0, 360.0);
        slider.setTextValueSuffix("°");
        addAndMakeVisible (slider);
        slider.addListener(this);
    };
    initLabel(yawLabel, yawSlider, "Yaw");
    initLabel(pitchLabel, pitchSlider, "Pitch");
    initLabel(rollLabel, rollSlider, "Roll");
    initSlider(yawSlider);
    initSlider(pitchSlider);
    initSlider(rollSlider);
}

void SpeakerGroupSettingsComponent::sliderValueChanged(juce::Slider* slider) {
    if (slider == &yawSlider) {
        speakerGroupComponent.speakerTreeVt.setProperty(YAW, yawSlider.getValue(), &speakerGroupComponent.undoManager);
    } else if (slider == &pitchSlider) {
        speakerGroupComponent.speakerTreeVt.setProperty(PITCH, pitchSlider.getValue(), &speakerGroupComponent.undoManager);
    } else if (slider == &rollSlider) {
        speakerGroupComponent.speakerTreeVt.setProperty(ROLL, rollSlider.getValue(), &speakerGroupComponent.undoManager);
    }
}

void SpeakerGroupSettingsComponent::resized() {
    auto sliderLeft = 120;
    yawSlider.setBounds (sliderLeft, 20, getWidth() - sliderLeft - 10, 20);
    pitchSlider.setBounds (sliderLeft, 50, getWidth() - sliderLeft - 10, 20);
    rollSlider.setBounds (sliderLeft, 50, getWidth() - sliderLeft - 10, 20);
}
} // namespace gris
