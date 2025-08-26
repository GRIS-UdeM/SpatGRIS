#pragma once
#include "../../../sg_GrisLookAndFeel.hpp"
#include "SpeakerTreeComponent.hpp"

namespace gris
{

class SpeakerGroupSettingsComponent : public juce::Component,
                                      public juce::Slider::Listener
{
 public:
    SpeakerGroupSettingsComponent(SpeakerGroupComponent& parent, GrisLookAndFeel& glaf);
    ~SpeakerGroupSettingsComponent();
    SpeakerGroupSettingsComponent() = delete;
    SG_DELETE_COPY_AND_MOVE(SpeakerGroupSettingsComponent);

    void resized() override;

 private:
    GrisLookAndFeel& lookAndFeel;
    SpeakerGroupComponent& speakerGroupComponent;
    juce::Label yawLabel;
    juce::Label pitchLabel;
    juce::Label rollLabel;
    juce::Slider yawSlider;
    juce::Slider pitchSlider;
    juce::Slider rollSlider;
    void sliderValueChanged (juce::Slider* slider) override;

    JUCE_LEAK_DETECTOR(SpeakerGroupSettingsComponent);
};

class SpeakerGroupSettingsWindow : public juce::DocumentWindow
{
 public:
    SpeakerGroupSettingsWindow(SpeakerGroupComponent& parent);
    SpeakerGroupSettingsWindow() = delete;
    SG_DELETE_COPY_AND_MOVE(SpeakerGroupSettingsWindow);
    void closeButtonPressed();

 private:
    SpeakerGroupComponent& parent;
    GrisLookAndFeel lookAndFeel;
    SpeakerGroupSettingsComponent settingsComponent;
    JUCE_LEAK_DETECTOR(SpeakerGroupSettingsWindow);
};
} // namespace gris
