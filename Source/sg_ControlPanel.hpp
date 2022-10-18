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

#include "sg_LayoutComponent.hpp"
#include "sg_LogicStrucs.hpp"
#include "sg_RecordButton.hpp"
#include "sg_SpatSlider.hpp"
#include "sg_SubPanelComponent.hpp"
#include "sg_TitledComponent.hpp"
#include "sg_constants.hpp"

namespace gris
{
class MainContentComponent;
class ControlPanel;

//==============================================================================
class GainsSubPanel final
    : public SubPanelComponent
    , public SpatSlider::Listener
{
    //==============================================================================
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;
    SpatSlider mMasterGainSlider{ LEGAL_MASTER_GAIN_RANGE.getStart().get(),
                                  LEGAL_MASTER_GAIN_RANGE.getEnd().get(),
                                  0.1f,
                                  " db",
                                  "Output gain",
                                  "Gain applied to every speaker",
                                  *this,
                                  mLookAndFeel };
    SpatSlider mInterpolationSlider{
        0.0f, 1.0f, 0.01f, "", "Interpolation", "Determines how much source panning is smoothed", *this, mLookAndFeel
    };

public:
    //==============================================================================
    GainsSubPanel(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel);
    ~GainsSubPanel() override = default;
    SG_DELETE_COPY_AND_MOVE(GainsSubPanel)
    //==============================================================================
    void setMasterGain(dbfs_t gain);
    void setInterpolation(float interpolation);
    //==============================================================================
    void sliderMoved(float value, SpatSlider * slider) override;

private:
    JUCE_LEAK_DETECTOR(GainsSubPanel)
};

//==============================================================================
class SpatSettingsSubPanel final
    : public SubPanelComponent
    , juce::ComboBox::Listener
    , juce::TextButton::Listener
{
    struct AttenuationValues {
        juce::Array<dbfs_t> dbValues{};
        juce::StringArray dbStrings{};
        juce::Array<hz_t> hzValues{};
        juce::StringArray hzStrings{};
    };

    ControlPanel & mControlPanel;
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;

    AttenuationValues mAttenuationValues{};

    LayoutComponent mCol1Layout{ LayoutComponent::Orientation::vertical, false, false, mLookAndFeel };
    LayoutComponent mAlgorithmButtonsLayout{ LayoutComponent::Orientation::horizontal, false, false, mLookAndFeel };

    LayoutComponent mCol2Layout{ LayoutComponent::Orientation::vertical, false, false, mLookAndFeel };
    LayoutComponent mAttenuationLayout{ LayoutComponent::Orientation::horizontal, false, false, mLookAndFeel };
    LayoutComponent mStereoRoutingLayout{ LayoutComponent::Orientation::horizontal, false, false, mLookAndFeel };

    juce::Label mAlgorithmSelectionLabel{};
    juce::Label mAttenuationSettingsLabel{};
    juce::ToggleButton mAttenuationSettingsButton{ "Attenuation settings" };

    juce::TextButton mDomeButton{};
    juce::TextButton mCubeButton{};
    juce::TextButton mHybridButton{};

    juce::ComboBox mAttenuationDbCombo{};
    juce::ComboBox mAttenuationHzCombo{};

    juce::Label mStereoReductionLabel{};
    juce::Label mStereoRoutingLabel{};

    juce::ComboBox mStereoReductionCombo{};

    juce::Label mLeftLabel{};
    juce::ComboBox mLeftCombo{};
    juce::Label mRightLabel{};
    juce::ComboBox mRightCombo{};

public:
    //==============================================================================
    SpatSettingsSubPanel(ControlPanel & controlPanel,
                         MainContentComponent & mainContentComponent,
                         GrisLookAndFeel & lookAndFeel);
    ~SpatSettingsSubPanel() override = default;
    SG_DELETE_COPY_AND_MOVE(SpatSettingsSubPanel)
    //==============================================================================
    void updateMaxOutputPatch(output_patch_t maxOutputPatch, StereoRouting const & routing);
    //==============================================================================
    void setSpatMode(SpatMode spatMode);
    void setStereoMode(tl::optional<StereoMode> const & stereoMode);
    void setAttenuationDb(dbfs_t attenuation);
    void setAttenuationHz(hz_t freq);
    void setAttenuationBypass(bool attenuationIsBypassed);
    void setStereoRouting(StereoRouting const & routing);

private:
    //==============================================================================
    [[nodiscard]] SpatMode getSpatMode() const;
    [[nodiscard]] tl::optional<StereoMode> getStereoMode() const;
    [[nodiscard]] bool shouldShowAttenuationSettings() const;
    [[nodiscard]] bool shouldShowStereoRouting() const;
    void updateAttenuationState();
    void updateEnabledStereoRoutings();
    void updateVisibility();
    void updateLayout();
    //==============================================================================
    void buttonClicked(juce::Button * button) override;
    void comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged) override;
    //==============================================================================
    static AttenuationValues getAttenuationValues();
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatSettingsSubPanel)
};

//==============================================================================
class ControlPanel final
    : public MinSizedComponent
    , public RecordButton::Listener
{
    //==============================================================================
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;
    LayoutComponent mLayout{ LayoutComponent::Orientation::horizontal, true, false, mLookAndFeel };
    TitledComponent mSection{ "Controls", &mLayout, mLookAndFeel };

    GainsSubPanel mGainsSubPanel{ mMainContentComponent, mLookAndFeel };
    SpatSettingsSubPanel mSpatSettingsSubPanel{ *this, mMainContentComponent, mLookAndFeel };
    RecordButton mRecordButton{ *this, mLookAndFeel };

public:
    //==============================================================================
    ControlPanel(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel);
    ~ControlPanel() override = default;
    SG_DELETE_COPY_AND_MOVE(ControlPanel)
    //==============================================================================
    void setMasterGain(dbfs_t gain);
    void setInterpolation(float interpolation);
    void setSpatMode(SpatMode spatMode);
    void setStereoMode(tl::optional<StereoMode> const & mode);
    void setCubeAttenuationDb(dbfs_t value);
    void setCubeAttenuationHz(hz_t value);
    void setCubeAttenuationBypass(bool value);
    void setRecordButtonState(RecordButton::State state);
    void setStereoRouting(StereoRouting const & routing);
    void updateMaxOutputPatch(output_patch_t maxOutputPatch, StereoRouting const & routing);
    //==============================================================================
    void resized() override;
    void forceLayoutUpdate();
    [[nodiscard]] int getMinWidth() const noexcept override { return mLayout.getMinWidth(); }
    [[nodiscard]] int getMinHeight() const noexcept override { return mLayout.getMinHeight(); }

private:
    //==============================================================================
    void recordButtonPressed() override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(ControlPanel)
};

} // namespace gris