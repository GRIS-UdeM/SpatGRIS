#include "ControlPanel.h"

//==============================================================================
ControlPanel::ControlPanel(Listener & listener, GrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLookAndFeel(lookAndFeel)
{
    mLayout.addSection(&mMasterGainSlider).withChildMinSize().widthPadding(PADDING);
    mLayout.addSection(&mInterpolationSlider).withChildMinSize().withVerticalPadding(PADDING).withRightPadding(PADDING);
    mLayout.addSection(&mSpatModeComponent).withChildMinSize().withVerticalPadding(PADDING).withRightPadding(PADDING);
    mLayout.addSection(&mNumSourcesTextEditor)
        .withChildMinSize()
        .withVerticalPadding(PADDING)
        .withRightPadding(PADDING);
    mLayout.addSection(&mRecordButton).withChildMinSize().withVerticalPadding(PADDING).withRightPadding(PADDING);
    addAndMakeVisible(mLayout);
}
