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

#include "sg_ControlPanel.hpp"

#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"

namespace
{
constexpr auto COL_1_WIDTH = 140;
constexpr auto COL_2_WIDTH = 160;

constexpr auto NUM_SPAT_MODES = 3;

constexpr auto ROW_1_CONTENT_HEIGHT = 20;
constexpr auto ROW_2_CONTENT_HEIGHT = 20;

constexpr auto COL_PADDING = 5;
constexpr auto ROW_PADDING = 5;

constexpr auto LABEL_HEIGHT = 18;
constexpr auto COL_INNER_PADDING = 3;
} // namespace

namespace gris
{
//==============================================================================
GainsSubPanel::GainsSubPanel(MainContentComponent & mainContentComponent, GrisLookAndFeel & lnf)
    : SubPanelComponent(LayoutComponent::Orientation::horizontal, lnf)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lnf)
{
    JUCE_ASSERT_MESSAGE_THREAD;
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
        mMainContentComponent.masterGainChanged(dbfs_t{ value });
        return;
    }

    jassert(slider == &mInterpolationSlider);
    mMainContentComponent.interpolationChanged(value);
}

//==============================================================================
SpatSettingsSubPanel::SpatSettingsSubPanel(ControlPanel & controlPanel,
                                           MainContentComponent & mainContentComponent,
                                           GrisLookAndFeel & glaf)
    : SubPanelComponent(LayoutComponent::Orientation::horizontal, glaf)
    , mControlPanel(controlPanel)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(glaf)
    , mAttenuationDbSlider(glaf, -48, 0, 1, 0, 0, "dB", "")
    , mAttenuationHzSlider(glaf, 125, 8000, 1, 0, 8000, "Hz", "")
{
    auto const initLabel = [&](juce::Label & label,
                               juce::String const & text,
                               juce::Justification const justification = juce::Justification::centredTop) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(justification);
        label.setColour(juce::Label::ColourIds::textColourId, glaf.getFontColour());
    };

    auto const initButton = [&](juce::Button & button,
                                juce::String const & text,
                                juce::String const & tooltip,
                                int radioButtonId = SPAT_MODE_BUTTONS_RADIO_GROUP_ID) {
        button.setClickingTogglesState(true);
        if (radioButtonId)
            button.setRadioGroupId(radioButtonId, juce::dontSendNotification);
        button.setButtonText(text);
        button.setTooltip(tooltip);
        button.addListener(this);
    };

    initLabel(mAlgorithmSelectionLabel, "Algorithm selection");
    initLabel(mStereoReductionLabel, "Stereo reduction");
    initLabel(mStereoRoutingLabel, "Stereo routing");
    initLabel(mMulticoreLabel, "Multicore DSP Settings");
    initLabel(mLeftLabel, "L :", juce::Justification::topRight);
    initLabel(mRightLabel, "R :", juce::Justification::topRight);

    mAttenuationSettingsButton.setColour(juce::ToggleButton::ColourIds::textColourId, glaf.getFontColour());
    mAttenuationSettingsButton.onClick = [this] {
        updateAttenuationState();
        updateLayout();
    };
    mAttenuationDbSlider.onValueChange = [this] {
        auto const attenuation{ static_cast<dbfs_t>(static_cast<float>(mAttenuationDbSlider.getValue())) };
        mMainContentComponent.cubeAttenuationDbChanged(attenuation);
    };
    mAttenuationHzSlider.onValueChange = [this] {
        auto const freq{ static_cast<hz_t>(static_cast<float>(mAttenuationHzSlider.getValue())) };
        mMainContentComponent.cubeAttenuationHzChanged(freq);
    };

    initButton(mDomeButton, spatModeToString(SpatMode::vbap), spatModeToTooltip(SpatMode::vbap));
    initButton(mCubeButton, spatModeToString(SpatMode::mbap), spatModeToTooltip(SpatMode::mbap));
    initButton(mHybridButton, spatModeToString(SpatMode::hybrid), spatModeToTooltip(SpatMode::hybrid));

    initButton(mMulticoreDSPToggle, "Multicore DSP", "Experimental : This will use more CPU resources but can perform better on large speaker setups. Does not parallelize stereo or binaural reductions.", NOT_A_RADIO_BUTTON_ID);
    initButton(mMulticoreDSPCPUPresetToggle, "A", "This preset uses less idle CPU but can perform worse.", MULTICORE_PRESETS_RADIO_GROUP_ID);
    initButton(mMulticoreDSPLatencyPresetToggle, "B", "This preset uses more idle CPU but can perform better.", MULTICORE_PRESETS_RADIO_GROUP_ID);

    juce::StringArray items{ "None" };
    items.addArray(STEREO_MODE_STRINGS);
    mStereoReductionCombo.addItemList(items, 1);
    mStereoReductionCombo.addListener(this);

    mLeftCombo.addListener(this);
    mRightCombo.addListener(this);

    mDomeButton.setToggleState(true, juce::dontSendNotification);

    static constexpr auto ALGORITHM_BUTTONS_WIDTH
        = (COL_1_WIDTH - (COL_INNER_PADDING * (NUM_SPAT_MODES - 1))) / NUM_SPAT_MODES;
    mAlgorithmButtonsLayout.addSection(mDomeButton)
        .withFixedSize(ALGORITHM_BUTTONS_WIDTH)
        .withRightPadding(COL_INNER_PADDING);
    mAlgorithmButtonsLayout.addSection(mCubeButton)
        .withFixedSize(ALGORITHM_BUTTONS_WIDTH)
        .withRightPadding(COL_INNER_PADDING);
    mAlgorithmButtonsLayout.addSection(mHybridButton).withFixedSize(ALGORITHM_BUTTONS_WIDTH);

    mCol1Layout.addSection(mAlgorithmSelectionLabel).withFixedSize(LABEL_HEIGHT);
    mCol1Layout.addSection(mAlgorithmButtonsLayout).withFixedSize(ROW_1_CONTENT_HEIGHT).withBottomPadding(ROW_PADDING);
    mCol1Layout.addSection(mStereoReductionLabel).withFixedSize(LABEL_HEIGHT);
    mCol1Layout.addSection(mStereoReductionCombo).withFixedSize(ROW_2_CONTENT_HEIGHT);

    static constexpr auto COL_2_HALF_WIDTH = (COL_2_WIDTH - COL_INNER_PADDING) / 2;
    mAttenuationLayout.addSection(mAttenuationDbSlider)
        .withFixedSize(COL_2_HALF_WIDTH - 3)
        .withVerticalPadding(1)
        .withLeftPadding(1)
        .withRightPadding(COL_INNER_PADDING);
    mAttenuationLayout.addSection(mAttenuationHzSlider)
        .withFixedSize(COL_2_HALF_WIDTH - 3)
        .withVerticalPadding(1)
        .withLeftPadding(1)
        .withRightPadding(COL_INNER_PADDING);

    static constexpr auto COL_2_QUARTER_WIDTH = COL_2_HALF_WIDTH / 2;
    static constexpr auto COL_2_SPACER = 15;
    static constexpr auto COL_2_TOP_PADDING = ROW_2_CONTENT_HEIGHT / 4;
    mStereoRoutingLayout.clearSections();
    mStereoRoutingLayout.addSection(mLeftLabel)
        .withFixedSize(COL_2_QUARTER_WIDTH - COL_2_SPACER)
        .withTopPadding(COL_2_TOP_PADDING);
    mStereoRoutingLayout.addSection(mLeftCombo).withFixedSize(COL_2_QUARTER_WIDTH + COL_2_SPACER);
    mStereoRoutingLayout.addSection(mRightLabel)
        .withFixedSize(COL_2_QUARTER_WIDTH - COL_2_SPACER)
        .withTopPadding(COL_2_TOP_PADDING);
    mStereoRoutingLayout.addSection(mRightCombo).withFixedSize(COL_2_QUARTER_WIDTH + COL_2_SPACER);

    mMulticoreLayout.addSection(mMulticoreDSPToggle).withFixedSize(COL_2_HALF_WIDTH);
    mMulticoreLayout.addSection(mMulticoreDSPCPUPresetToggle).withFixedSize(COL_2_QUARTER_WIDTH);
    mMulticoreLayout.addSection(mMulticoreDSPLatencyPresetToggle).withFixedSize(COL_2_QUARTER_WIDTH);

    mCol2Layout.addSection(mAttenuationSettingsButton).withFixedSize(LABEL_HEIGHT);
    mCol2Layout.addSection(mAttenuationLayout).withFixedSize(ROW_1_CONTENT_HEIGHT).withBottomPadding(ROW_PADDING);

#if MULTICORE_DSP
    mTopLeftComponentSwapper.addComponent(multicoreLabelName, juce::Component::SafePointer<juce::Component>(&mMulticoreLabel));
#endif
    mTopLeftComponentSwapper.addComponent(stereoRoutingLabelName, juce::Component::SafePointer<juce::Component>(&mStereoRoutingLabel));

    mCol2Layout.addSection(mTopLeftComponentSwapper).withFixedSize(LABEL_HEIGHT);

#if MULTICORE_DSP
    mBottomLeftComponentSwapper.addComponent(multicoreLayoutName, juce::Component::SafePointer<juce::Component>(&mMulticoreLayout));
#endif
    mBottomLeftComponentSwapper.addComponent(stereoRoutingLayoutName,juce::Component::SafePointer<juce::Component>(&mStereoRoutingLayout));
    addAndMakeVisible(mBottomLeftComponentSwapper);

    mCol2Layout.addSection(mBottomLeftComponentSwapper).withFixedSize(ROW_2_CONTENT_HEIGHT);

    updateLayout();
}

//==============================================================================
SpatMode SpatSettingsSubPanel::getSpatMode() const
{
    if (mDomeButton.getToggleState()) {
        return SpatMode::vbap;
    }
    if (mCubeButton.getToggleState()) {
        return SpatMode::mbap;
    }

    jassert(mHybridButton.getToggleState());
    return SpatMode::hybrid;
}

//==============================================================================
tl::optional<StereoMode> SpatSettingsSubPanel::getStereoMode() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return stringToStereoMode(mStereoReductionCombo.getText());
}

//==============================================================================
bool SpatSettingsSubPanel::shouldShowAttenuationSettings() const
{
    auto const spatMode{ getSpatMode() };
    return (spatMode == SpatMode::mbap || spatMode == SpatMode::hybrid) ? true : false;
}

//==============================================================================
bool SpatSettingsSubPanel::shouldShowStereoRouting() const
{
    return getStereoMode().has_value();
}

//==============================================================================
void SpatSettingsSubPanel::updateAttenuationState()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const attenuationButtonState{ mAttenuationSettingsButton.getToggleStateValue() };
    const bool bypassAttenuation{ !attenuationButtonState.getValue() };
    auto const attenuationBypassState{ bypassAttenuation ? AttenuationBypassSate::on : AttenuationBypassSate::off };

    mMainContentComponent.cubeAttenuationBypassState(attenuationBypassState);
}

//==============================================================================
void SpatSettingsSubPanel::updateMaxOutputPatch(output_patch_t const maxOutputPatch, StereoRouting const & routing)
{
    mLeftCombo.clear(juce::dontSendNotification);
    mRightCombo.clear(juce::dontSendNotification);

    for (output_patch_t i{ output_patch_t::OFFSET }; i <= maxOutputPatch; ++i) {
        mLeftCombo.addItem(juce::String{ i.get() }, i.get());
        mRightCombo.addItem(juce::String{ i.get() }, i.get());
    }

    mLeftCombo.setSelectedId(routing.left.get(), juce::dontSendNotification);
    mRightCombo.setSelectedId(routing.right.get(), juce::dontSendNotification);

    updateEnabledStereoRoutings();
}

//==============================================================================
void SpatSettingsSubPanel::setSpatMode(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    [&] {
        switch (spatMode) {
        case SpatMode::vbap:
            mDomeButton.setToggleState(true, juce::dontSendNotification);
            return;
        case SpatMode::mbap:
            mCubeButton.setToggleState(true, juce::dontSendNotification);
            return;
        case SpatMode::hybrid:
            mHybridButton.setToggleState(true, juce::dontSendNotification);
            return;
        case SpatMode::invalid:
            return;
        }
        jassertfalse;
    }();

    updateLayout();
    mControlPanel.forceLayoutUpdate();
}

//==============================================================================
void SpatSettingsSubPanel::setMulticoreDSP(bool useMulticoreDSP)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mMulticoreDSPToggle.setToggleState(useMulticoreDSP, juce::dontSendNotification);
}

//==============================================================================
void SpatSettingsSubPanel::setMulticoreDSPPreset(int preset)
{
    if (preset == OPTIMIZE_CPU_MULTICORE_PRESET) {
        mMulticoreDSPCPUPresetToggle.setToggleState(true, juce::dontSendNotification);
    } else if (preset == OPTIMIZE_LATENCY_MULTICORE_PRESET) {
        mMulticoreDSPLatencyPresetToggle.setToggleState(true, juce::dontSendNotification);
    }
    JUCE_ASSERT_MESSAGE_THREAD;
}

//==============================================================================
void SpatSettingsSubPanel::setStereoMode(tl::optional<StereoMode> const & stereoMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    if (!stereoMode) {
        mStereoReductionCombo.setSelectedItemIndex(0, juce::dontSendNotification);
    } else {
        auto const index{ static_cast<int>(*stereoMode) + 1 };
        mStereoReductionCombo.setSelectedItemIndex(index, juce::dontSendNotification);
    }
    updateLayout();
    mControlPanel.forceLayoutUpdate();
}

//==============================================================================
void SpatSettingsSubPanel::setAttenuationDb(dbfs_t const attenuation)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mAttenuationDbSlider.setValue(attenuation.get());
}

//==============================================================================
void SpatSettingsSubPanel::setAttenuationHz(hz_t const freq)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mAttenuationHzSlider.setValue(freq.get());
}

//==============================================================================
void SpatSettingsSubPanel::setAttenuationBypass(AttenuationBypassSate state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto bypassState{ state };

    // for Project file compatibility
    if (bypassState == AttenuationBypassSate::invalid) {
        mMainContentComponent.cubeAttenuationBypassState(AttenuationBypassSate::off);
        bypassState = AttenuationBypassSate::off;
    }

    mAttenuationSettingsButton.setToggleState(bypassState == AttenuationBypassSate::off, juce::dontSendNotification);
    updateLayout();
}

//==============================================================================
void SpatSettingsSubPanel::setStereoRouting(StereoRouting const & routing)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    jassert(routing.left != routing.right);

    mLeftCombo.setSelectedId(routing.left.get(), juce::dontSendNotification);
    mRightCombo.setSelectedId(routing.right.get(), juce::dontSendNotification);

    updateEnabledStereoRoutings();
}

//==============================================================================
void SpatSettingsSubPanel::updateEnabledStereoRoutings()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const leftId{ mLeftCombo.getSelectedId() };
    auto const rightId{ mRightCombo.getSelectedId() };

    jassert(mLeftCombo.getNumItems() == mRightCombo.getNumItems());
    jassert((leftId == 0 && rightId == 0) || mLeftCombo.getSelectedId() != mRightCombo.getSelectedId());

    for (int i{}; i < mLeftCombo.getNumItems(); ++i) {
        auto const itemId{ mLeftCombo.getItemId(i) };
        auto const leftEnabled{ itemId != rightId };
        auto const rightEnabled{ itemId != leftId };
        mLeftCombo.setItemEnabled(itemId, leftEnabled);
        mRightCombo.setItemEnabled(itemId, rightEnabled);
    }
}

//==============================================================================
void SpatSettingsSubPanel::updateLayout()
{
    auto const showAttenuation{ shouldShowAttenuationSettings() };
    auto const showRouting{ shouldShowStereoRouting() };
    auto const isAttenuationActive{ mAttenuationSettingsButton.getToggleState() };

    mAttenuationDbSlider.setEnabled(isAttenuationActive);
    mAttenuationHzSlider.setEnabled(isAttenuationActive);

    mAttenuationSettingsLabel.setVisible(showAttenuation);
    mAttenuationSettingsButton.setVisible(showAttenuation);
    mAttenuationDbSlider.setVisible(showAttenuation);
    mAttenuationHzSlider.setVisible(showAttenuation);

    mStereoRoutingLabel.setVisible(showRouting);

    // We may have previously set the visibility of these to false.
    mBottomLeftComponentSwapper.setVisible(true);
    mTopLeftComponentSwapper.setVisible(true);
    if (showRouting) {
        mBottomLeftComponentSwapper.showComponent(stereoRoutingLayoutName);
        mTopLeftComponentSwapper.showComponent(stereoRoutingLabelName);
    } // we should not show the multicoreDSPToggle in hybrid mode because it should have no effect.
    else if (getSpatMode() != SpatMode::hybrid) {
        mBottomLeftComponentSwapper.showComponent(multicoreLayoutName);
        mTopLeftComponentSwapper.showComponent(multicoreLabelName);
    } else {
        // if we are in hybrid mode and we should not show routing, hide everything.
        mBottomLeftComponentSwapper.setVisible(false);
        mTopLeftComponentSwapper.setVisible(false);
    }

    clearSections();
    addSection(mCol1Layout).withFixedSize(COL_1_WIDTH).withHorizontalPadding(COL_PADDING);
    addSection(mCol2Layout).withFixedSize(COL_2_WIDTH).withLeftPadding(COL_PADDING);
}

//==============================================================================
void SpatSettingsSubPanel::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    if (button == &mDomeButton || button == &mCubeButton || button == &mHybridButton) {
        if (!button->getToggleState()) {
            return;
        }

        auto const getSpatMode = [&]() {
            if (button == &mDomeButton) {
                return SpatMode::vbap;
            }
            if (button == &mCubeButton) {
                return SpatMode::mbap;
            }
            jassert(button == &mHybridButton);
            return SpatMode::hybrid;
        };

        auto const spatMode{ getSpatMode() };
        mMainContentComponent.setSpatMode(spatMode);
        updateLayout();
        mControlPanel.forceLayoutUpdate();
    } else if (button == &mMulticoreDSPToggle) {
        mMainContentComponent.setMulticoreDSPState(button->getToggleState());
    } else if (button == &mMulticoreDSPCPUPresetToggle && button->getToggleState()) {
        mMainContentComponent.setMulticoreDSPPreset(OPTIMIZE_CPU_MULTICORE_PRESET);
    } else if (button == &mMulticoreDSPLatencyPresetToggle && button->getToggleState()) {
        mMainContentComponent.setMulticoreDSPPreset(OPTIMIZE_LATENCY_MULTICORE_PRESET);
    }
}

//==============================================================================
void SpatSettingsSubPanel::comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (comboBoxThatHasChanged == &mStereoReductionCombo) {
        auto const stereoMode{ getStereoMode() };
        mMainContentComponent.setStereoMode(stereoMode);
        updateLayout();
        mControlPanel.forceLayoutUpdate();

        return;
    }

    if (comboBoxThatHasChanged == &mLeftCombo || comboBoxThatHasChanged == &mRightCombo) {
        StereoRouting const routing{ output_patch_t{ mLeftCombo.getSelectedId() },
                                     output_patch_t{ mRightCombo.getSelectedId() } };
        jassert(routing.left != routing.right);

        mMainContentComponent.setStereoRouting(routing);
        updateEnabledStereoRoutings();

        return;
    }

    jassertfalse;
}

//==============================================================================
ControlPanel::ControlPanel(MainContentComponent & mainContentComponent, GrisLookAndFeel & glaf)
    : mMainContentComponent(mainContentComponent)
    , mLookAndFeel(glaf)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static constexpr auto PADDING = 4;

    mLayout.addSection(mGainsSubPanel).withChildMinSize().withPadding(PADDING);
    mLayout.addSection(mSpatSettingsSubPanel).withChildMinSize().withTopPadding(PADDING).withBottomPadding(PADDING);
    mLayout.addSection(mGeneralMuteButton).withChildMinSize().withLeftPadding(PADDING);
    mLayout.addSection(nullptr).withRelativeSize(1.0f);
    mLayout.addSection(mRecordButton).withChildMinSize().withLeftPadding(PADDING).withRightPadding(30);
    mLayout.resized();

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
    mSpatSettingsSubPanel.setSpatMode(spatMode);
}

// used to set the multicore dsp button's toggle state from the main component at
// project load time.
void ControlPanel::setMulticoreDSP(bool useMulticoreDSP)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mSpatSettingsSubPanel.setMulticoreDSP(useMulticoreDSP);
}
void ControlPanel::setMulticoreDSPPreset(int preset)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mSpatSettingsSubPanel.setMulticoreDSPPreset(preset);
}


//==============================================================================
void ControlPanel::setStereoMode(tl::optional<StereoMode> const & mode)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mSpatSettingsSubPanel.setStereoMode(mode);
}

//==============================================================================
void ControlPanel::setCubeAttenuationDb(dbfs_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mSpatSettingsSubPanel.setAttenuationDb(value);
}

//==============================================================================
void ControlPanel::setCubeAttenuationHz(hz_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mSpatSettingsSubPanel.setAttenuationHz(value);
}

//==============================================================================
void ControlPanel::setCubeAttenuationBypass(AttenuationBypassSate value)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mSpatSettingsSubPanel.setAttenuationBypass(value);
}

//==============================================================================
void ControlPanel::setGeneralMuteButtonState(GeneralMuteButton::State state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mGeneralMuteButton.setState(state);
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
    JUCE_ASSERT_MESSAGE_THREAD;
    mSpatSettingsSubPanel.setStereoRouting(routing);
}

//==============================================================================
void ControlPanel::updateMaxOutputPatch(output_patch_t const maxOutputPatch, StereoRouting const & routing)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mSpatSettingsSubPanel.updateMaxOutputPatch(maxOutputPatch, routing);
}

//==============================================================================
GeneralMuteButton::State ControlPanel::getGeneralMuteButtonState()
{
    return mGeneralMuteButton.getGeneralMuteButtonState();
}

//==============================================================================
void ControlPanel::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mLayout.setBounds(0, 0, getWidth(), getHeight());
}

//==============================================================================
void ControlPanel::forceLayoutUpdate()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const generalMuteState{ mMainContentComponent.getData().speakerSetup.generalMute
                                     ? GeneralMuteButton::State::allMuted
                                     : GeneralMuteButton::State::allUnmuted };

    resized();
    mLayout.resized();
    mGeneralMuteButton.setState(generalMuteState);
}

//==============================================================================
void ControlPanel::generalMuteButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mMainContentComponent.generalMuteButtonPressed();
}

//==============================================================================
void ControlPanel::recordButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mMainContentComponent.recordButtonPressed();
}

} // namespace gris
