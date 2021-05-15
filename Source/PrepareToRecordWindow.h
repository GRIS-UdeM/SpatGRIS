#pragma once

#include "AudioManager.h"
#include "LayoutComponent.h"

class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class PrepareToRecordComponent final
    : public juce::Component
    , public juce::TextButton::Listener
{
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;

    juce::TextEditor mPathEditor{};
    juce::TextButton mBrowseButton{};
    juce::TextButton mMonoButton{};
    juce::TextButton mInterleavedButton{};
    juce::TextButton mRecordButton{};

    juce::FileChooser mFileChooser;

public:
    //==============================================================================
    PrepareToRecordComponent(juce::File const & recordingDirectory,
                             RecordingOptions const & recordingOptions,
                             MainContentComponent & mainContentComponent,
                             GrisLookAndFeel & lookAndFeel);
    ~PrepareToRecordComponent() override = default;
    //==============================================================================
    PrepareToRecordComponent(PrepareToRecordComponent const &) = delete;
    PrepareToRecordComponent(PrepareToRecordComponent &&) = delete;
    PrepareToRecordComponent & operator=(PrepareToRecordComponent const &) = delete;
    PrepareToRecordComponent & operator=(PrepareToRecordComponent &&) = delete;
    //==============================================================================
    void resized() override;
    void buttonClicked(juce::Button * button) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(PrepareToRecordComponent)
};

//==============================================================================
class PrepareToRecordWindow final : public juce::DocumentWindow
{
    MainContentComponent & mMainContentComponent;
    PrepareToRecordComponent mContentComponent;

public:
    //==============================================================================
    PrepareToRecordWindow(juce::File const & recordingDirectory,
                          RecordingOptions const & recordingOptions,
                          MainContentComponent & mainContentComponent,
                          GrisLookAndFeel & lookAndFeel);
    ~PrepareToRecordWindow() override = default;
    //==============================================================================
    PrepareToRecordWindow(PrepareToRecordWindow const &) = delete;
    PrepareToRecordWindow(PrepareToRecordWindow &&) = delete;
    PrepareToRecordWindow & operator=(PrepareToRecordWindow const &) = delete;
    PrepareToRecordWindow & operator=(PrepareToRecordWindow &&) = delete;
    //==============================================================================
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(PrepareToRecordWindow)
};