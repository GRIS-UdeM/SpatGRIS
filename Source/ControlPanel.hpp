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

#pragma once

#include "AttenuationSettingsComponent.hpp"
#include "Constants.hpp"
#include "LayoutComponent.hpp"
#include "RecordButton.hpp"
#include "SpatModeComponent.hpp"
#include "SpatSlider.hpp"
#include "SpatTextEditor.hpp"
#include "StereoPatchSelectionComponent.hpp"
#include "StrongTypes.hpp"
#include "TitledComponent.hpp"

//==============================================================================
class ControlPanel final
    : public MinSizedComponent
    , public SpatModeComponent::Listener
    , public AttenuationSettingsComponent::Listener
    , public SpatSlider::Listener
    , public SpatTextEditor::Listener
    , public RecordButton::Listener
    , public StereoPatchSelectionComponent::Listener
{
    static constexpr auto PADDING = 5;

public:
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        //==============================================================================
        Listener(Listener const &) = delete;
        Listener(Listener &&) = delete;
        Listener & operator=(Listener const &) = delete;
        Listener & operator=(Listener &&) = delete;
        //==============================================================================
        virtual void masterGainChanged(dbfs_t gain) = 0;
        virtual void interpolationChanged(float interpolation) = 0;
        virtual bool setSpatMode(SpatMode spatMode) = 0;
        virtual void setStereoMode(tl::optional<StereoMode> stereoMode) = 0;
        virtual void cubeAttenuationDbChanged(dbfs_t value) = 0;
        virtual void cubeAttenuationHzChanged(hz_t value) = 0;
        virtual void numSourcesChanged(int numSources) = 0;
        virtual void recordButtonPressed() = 0;
        virtual void stereoRoutingChanged(StereoRouting const & routing) = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    Listener & mListener;
    GrisLookAndFeel & mLookAndFeel;
    LayoutComponent mLayout{ LayoutComponent::Orientation::horizontal, true, false, mLookAndFeel };
    TitledComponent mSection{ "Controls", &mLayout, mLookAndFeel };

    SpatSlider mMasterGainSlider{ LEGAL_MASTER_GAIN_RANGE.getStart().get(),
                                  LEGAL_MASTER_GAIN_RANGE.getEnd().get(),
                                  0.1f,
                                  " db",
                                  "Output gain",
                                  "Gain applied to every speaker",
                                  *this,
                                  mLookAndFeel };
    SpatSlider mInterpolationSlider{
        0.0f, 1.0f, 0.01f, "", "Gain smoothing", "Determines how much source panning is smoothed", *this, mLookAndFeel
    };
    SpatModeComponent mSpatModeComponent{ *this, mLookAndFeel };
    AttenuationSettingsComponent mCubeSettingsComponent{ *this, mLookAndFeel };
    StereoPatchSelectionComponent mStereoRoutingComponent{ *this, mLookAndFeel };
    SpatTextEditor mNumSourcesTextEditor{ "Sources:", "Number of available sources", *this, mLookAndFeel };
    RecordButton mRecordButton{ *this, mLookAndFeel };

public:
    //==============================================================================
    ControlPanel(Listener & listener, GrisLookAndFeel & lookAndFeel);
    ~ControlPanel() override = default;
    //==============================================================================
    ControlPanel(ControlPanel const &) = delete;
    ControlPanel(ControlPanel &&) = delete;
    ControlPanel & operator=(ControlPanel const &) = delete;
    ControlPanel & operator=(ControlPanel &&) = delete;
    //==============================================================================
    void setMasterGain(dbfs_t gain);
    void setInterpolation(float interpolation);
    void setSpatMode(SpatMode spatMode);
    void setStereoMode(tl::optional<StereoMode> const & mode);
    void setCubeAttenuationDb(dbfs_t value);
    void setCubeAttenuationHz(hz_t value);
    void setNumSources(int numSources);
    void setRecordButtonState(RecordButton::State state);
    void setStereoRouting(StereoRouting const & routing);
    void updateSpeakers(SpeakersOrdering ordering);
    //==============================================================================
    void resized() override;
    int getMinWidth() const noexcept override { return mLayout.getMinWidth(); }
    int getMinHeight() const noexcept override { return mLayout.getMinHeight(); }
    void handleSpatModeChanged(SpatMode spatMode) override;
    void handleStereoModeChanged(tl::optional<StereoMode> stereoMode) override;
    void sliderMoved(float value, SpatSlider * slider) override;
    void textEditorChanged(juce::String const & value, SpatTextEditor * editor) override;
    void recordButtonPressed() override;
    void cubeAttenuationDbChanged(dbfs_t value) override;
    void cubeAttenuationHzChanged(hz_t value) override;
    void handleStereoRoutingChanged(StereoRouting const & routing) override;

private:
    //==============================================================================
    void refreshLayout();
    //==============================================================================
    JUCE_LEAK_DETECTOR(ControlPanel)
};