#include "ControlPanel.h"

//==============================================================================
ControlPanel::ControlPanel(Listener & listener, GrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLookAndFeel(lookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mLayout.addSection(&mMasterGainSlider).withChildMinSize();
    mLayout.addSection(&mInterpolationSlider).withChildMinSize().withRightPadding(15);
    mLayout.addSection(&mSpatModeComponent).withChildMinSize().withRightPadding(15);
    mLayout.addSection(&mNumSourcesTextEditor).withChildMinSize().withRightPadding(15);
    mLayout.addSection(nullptr).withRelativeSize(1.0f);
    mLayout.addSection(&mRecordButton).withChildMinSize().withRightPadding(20);
    ;
    addAndMakeVisible(mLayout);
}

//==============================================================================
void ControlPanel::setMasterGain(dbfs_t const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mMasterGainSlider.setValue(gain.get());
}

//==============================================================================
void ControlPanel::setInterpolation(float const interpolation)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mInterpolationSlider.setValue(interpolation);
}

//==============================================================================
void ControlPanel::setSpatMode(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mSpatModeComponent.setSpatMode(spatMode);
}

//==============================================================================
void ControlPanel::setNumSources(int const numSources)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mNumSourcesTextEditor.setText(juce::String{ numSources });
}

//==============================================================================
void ControlPanel::setRecordButtonState(RecordButton::State const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mRecordButton.setState(state);
}

//==============================================================================
void ControlPanel::resized()
{
    mLayout.setBounds(0, 0, getWidth(), getHeight());
}

//==============================================================================
void ControlPanel::handleSpatModeChanged(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListener.spatModeChanged(spatMode);
}

//==============================================================================
void ControlPanel::sliderMoved(float const value, SpatSlider * slider)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    if (slider == &mMasterGainSlider) {
        mListener.masterGainChanged(dbfs_t{ value });
        return;
    }
    jassert(slider == &mInterpolationSlider);
    mListener.interpolationChanged(value);
}

//==============================================================================
void ControlPanel::textEditorChanged(juce::String const & value, [[maybe_unused]] SpatTextEditor * editor)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(editor == &mNumSourcesTextEditor);
    auto const numSources{ std::clamp(value.getIntValue(), 1, MAX_NUM_SOURCES) };
    mListener.numSourcesChanged(numSources);
}

//==============================================================================
void ControlPanel::recordButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListener.recordButtonPressed();
}
