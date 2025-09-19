#include "SpeakerGroupSettingsWindow.hpp"
#include "Data/sg_LogicStrucs.hpp"

namespace gris
{

SpeakerGroupSettingsWindow::SpeakerGroupSettingsWindow(SpeakerGroupComponent & theParent)
    : DocumentWindow(theParent.speakerTreeVt[SPEAKER_GROUP_NAME].toString() + " Rotation Settings", {}, allButtons)
    , parent(theParent)
    , settingsComponent(parent)
{
    setOpaque (true);
    setAlwaysOnTop(true);
    setContentNonOwned(&settingsComponent, true);
    setResizable(false, false);
    setUsingNativeTitleBar(true);
    centreWithSize(500, 200);
    setBackgroundColour(lookAndFeel.getBackgroundColour());
    setVisible(true);
}

void SpeakerGroupSettingsWindow::closeButtonPressed()
{
  parent.closeSettingsWindow();
}

SpeakerGroupSettingsComponent::SpeakerGroupSettingsComponent(SpeakerGroupComponent& sgc)
    : speakerGroupComponent(sgc)
{
    auto initLabel = [this](juce::Label & label, juce::Slider & associatedSlider, juce::StringRef text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::Flags::centredRight);
        label.setFont(lookAndFeel.getFont());
        label.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
        addAndMakeVisible(label);
        label.attachToComponent (&associatedSlider, true);
    };
    auto initSlider = [this](juce::Slider & slider, float value) {
        slider.setRange (0, 360.0, 0.1);
#if USE_JUCE_8
        slider.setTextValueSuffix(juce::String::fromUTF8(u8"\u00B0"));
#endif
        slider.setValue(value);
        addAndMakeVisible (slider);
        slider.addListener(this);
    };
    auto tv = speakerGroupComponent.speakerTreeVt;
    initLabel(yawLabel, yawSlider, "Yaw");
    initLabel(pitchLabel, pitchSlider, "Pitch");
    initLabel(rollLabel, rollSlider, "Roll");
    initSlider(yawSlider, tv.getProperty(YAW, 0.0));
    initSlider(pitchSlider, tv.getProperty(PITCH, 0.0));
    initSlider(rollSlider, tv.getProperty(ROLL, 0.0));
    setLookAndFeel(&lookAndFeel);
}

SpeakerGroupSettingsComponent::~SpeakerGroupSettingsComponent()
{
    setLookAndFeel(nullptr);
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
    constexpr auto sliderLeft = 60;
    yawSlider.setBounds (sliderLeft, 20, getWidth() - sliderLeft - 10, 20);
    pitchSlider.setBounds (sliderLeft, 50, getWidth() - sliderLeft - 10, 20);
    rollSlider.setBounds (sliderLeft, 80, getWidth() - sliderLeft - 10, 20);
}
} // namespace gris
