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

#include "ControlPanel.hpp"

//==============================================================================
GainsSubPanel::GainsSubPanel(Listener & listener, GrisLookAndFeel & lookAndFeel)
    : SubPanelComponent(LayoutComponent::Orientation::horizontal, lookAndFeel)
    , mListener(listener)
    , mLookAndFeel(lookAndFeel)
{
    addSection(mMasterGainSlider).withChildMinSize();
    addSection(mInterpolationSlider).withChildMinSize();
}

//==============================================================================
void GainsSubPanel::setMasterGain(dbfs_t const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mMasterGainSlider.setValue(gain.get());
}

//==============================================================================
void GainsSubPanel::setInterpolation(float const interpolation)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mInterpolationSlider.setValue(interpolation);
}

//==============================================================================
void GainsSubPanel::sliderMoved(float const value, SpatSlider * const slider)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (slider == &mMasterGainSlider) {
        mListener.masterGainChanged(dbfs_t{ value });
        return;
    }

    jassert(slider == &mInterpolationSlider);
    mListener.gainInterpolationChanged(value);
}

//==============================================================================
ControlPanel::ControlPanel(Listener & listener, GrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLookAndFeel(lookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    refreshLayout();
    addAndMakeVisible(mLayout);
}

//==============================================================================
void ControlPanel::setMasterGain(dbfs_t const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mGainsSubPanel.setMasterGain(gain);
}

//==============================================================================
void ControlPanel::setInterpolation(float const interpolation)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mGainsSubPanel.setInterpolation(interpolation);
}

//==============================================================================
void ControlPanel::setSpatMode(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    if (spatMode == mSpatModeComponent.getSpatMode()) {
        return;
    }
    mSpatModeComponent.setSpatMode(spatMode);
    refreshLayout();
}

//==============================================================================
void ControlPanel::setStereoMode(tl::optional<StereoMode> const & mode)
{
    mSpatModeComponent.setStereoMode(mode);
    refreshLayout();
}

//==============================================================================
void ControlPanel::setCubeAttenuationDb(dbfs_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mCubeSettingsComponent.setAttenuationDb(value);
}

//==============================================================================
void ControlPanel::setCubeAttenuationHz(hz_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mCubeSettingsComponent.setAttenuationHz(value);
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
void ControlPanel::setStereoRouting(StereoRouting const & routing)
{
    mStereoRoutingComponent.setStereoRouting(routing);
}

//==============================================================================
void ControlPanel::updateSpeakers(SpeakersOrdering ordering, StereoRouting const & routing)
{
    mStereoRoutingComponent.updateSpeakers(std::move(ordering), routing);
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
    mListener.setSpatMode(spatMode);
    refreshLayout();
}

//==============================================================================
void ControlPanel::handleStereoModeChanged(tl::optional<StereoMode> const stereoMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListener.setStereoMode(stereoMode);
    refreshLayout();
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

//==============================================================================
void ControlPanel::cubeAttenuationDbChanged(dbfs_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListener.cubeAttenuationDbChanged(value);
}

//==============================================================================
void ControlPanel::cubeAttenuationHzChanged(hz_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListener.cubeAttenuationHzChanged(value);
}

//==============================================================================
void ControlPanel::handleStereoRoutingChanged(StereoRouting const & routing)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListener.stereoRoutingChanged(routing);
}

//==============================================================================
void ControlPanel::masterGainChanged(dbfs_t const gain)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListener.masterGainChanged(gain);
}

//==============================================================================
void ControlPanel::gainInterpolationChanged(float const interpolation)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mListener.interpolationChanged(interpolation);
}

//==============================================================================
void ControlPanel::refreshLayout()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mLayout.clearSections();

    mLayout.addSection(mGainsSubPanel).withChildMinSize().widthPadding(5);
    mLayout.addSection(mSpatModeComponent).withChildMinSize();

    if (mSpatModeComponent.getStereoMode()) {
        mLayout.addSection(&mStereoRoutingComponent).withChildMinSize();
    }

    if (mSpatModeComponent.getSpatMode() == SpatMode::lbap) {
        mLayout.addSection(mCubeSettingsComponent)
            .withChildMinSize()
            .withRightPadding(15)
            .withTopPadding(10)
            .withBottomPadding(10);
    }

    mLayout.addSection(mNumSourcesTextEditor).withChildMinSize();
    mLayout.addSection(nullptr).withRelativeSize(1.0f);
    mLayout.addSection(mRecordButton).withChildMinSize();
    mLayout.resized();
}
