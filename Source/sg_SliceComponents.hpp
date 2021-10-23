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

#include "sg_DirectOutSelectorComponent.hpp"
#include "sg_LayoutComponent.hpp"
#include "sg_LogicStrucs.hpp"
#include "sg_MinSizedComponent.hpp"
#include "sg_MuteSoloComponent.hpp"
#include "sg_SmallToggleButton.hpp"
#include "sg_SourceIdButton.hpp"
#include "sg_SpeakerIdButton.hpp"
#include "sg_VuMeterComponent.hpp"

#include <JuceHeader.h>

static dbfs_t constexpr MIN_LEVEL_COMP{ -60.0f };
static dbfs_t constexpr MAX_LEVEL_COMP{ 0.0f };

class GrisLookAndFeel;
class SmallGrisLookAndFeel;

//==============================================================================
class AbstractSliceComponent
    : public MinSizedComponent
    , public MuteSoloComponent::Listener
{
protected:
    static constexpr auto ID_BUTTON_HEIGHT = 17;
    static constexpr auto MUTE_AND_SOLO_BUTTONS_HEIGHT = 15;
    static constexpr auto INNER_ELEMENTS_PADDING = 1;

    GrisLookAndFeel & mLookAndFeel;
    SmallGrisLookAndFeel & mSmallLookAndFeel;

    VuMeterComponent mVuMeter;

    MuteSoloComponent mMuteSoloComponent{ *this, mLookAndFeel, mSmallLookAndFeel };

public:
    //==============================================================================
    explicit AbstractSliceComponent(juce::String const & id,
                                    GrisLookAndFeel & lookAndFeel,
                                    SmallGrisLookAndFeel & smallLookAndFeel);
    //==============================================================================
    AbstractSliceComponent() = delete;
    ~AbstractSliceComponent() override = default;
    //==============================================================================
    AbstractSliceComponent(AbstractSliceComponent const &) = delete;
    AbstractSliceComponent(AbstractSliceComponent &&) = delete;
    AbstractSliceComponent & operator=(AbstractSliceComponent const &) = delete;
    AbstractSliceComponent & operator=(AbstractSliceComponent &&) = delete;
    //==============================================================================
    void setLevel(dbfs_t const level) { mVuMeter.setLevel(level); }
    void resetClipping() { mVuMeter.resetClipping(); }
    void setState(PortState state, bool soloMode);
    //==============================================================================
    void resized() override;
    [[nodiscard]] int getMinWidth() const noexcept final;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(AbstractSliceComponent)
}; // class LevelComponent

//==============================================================================
class SourceSliceComponent final
    : public AbstractSliceComponent
    , private DirectOutSelectorComponent::Listener
    , private SourceIdButton::Listener
{
public:
    //==============================================================================
    class Owner
    {
    public:
        Owner() = default;
        virtual ~Owner() = default;
        //==============================================================================
        Owner(Owner const &) = default;
        Owner(Owner &&) = default;
        Owner & operator=(Owner const &) = default;
        Owner & operator=(Owner &&) = default;
        //==============================================================================
        virtual void setSourceDirectOut(source_index_t sourceIndex, tl::optional<output_patch_t> outputPatch) = 0;
        virtual void setSourceColor(source_index_t sourceIndex, juce::Colour colour) = 0;
        virtual void setSourceState(source_index_t sourceIndex, PortState state) = 0;
        virtual void setSourceHybridSpatMode(source_index_t sourceIndex, SpatMode spatMode) = 0;
        [[nodiscard]] virtual SpeakersData const & getSpeakersData() const = 0;
    };

private:
    //==============================================================================
    static constexpr auto DIRECT_OUT_BUTTON_HEIGHT = 17;
    static juce::String const NO_DIRECT_OUT_TEXT;

    source_index_t mSourceIndex{};
    Owner & mOwner;

    SourceIdButton mIdButton;

    juce::Label mDomeLabel;
    juce::TextButton mDomeButton;
    juce::Label mCubeLabel;
    juce::TextButton mCubeButton;

    DirectOutSelectorComponent mDirectOutSelectorComponent;

public:
    //==============================================================================
    SourceSliceComponent(source_index_t sourceIndex,
                         tl::optional<output_patch_t> directOut,
                         SpatMode projectSpatMode,
                         SpatMode hybridSpatMode,
                         juce::Colour colour,
                         std::shared_ptr<DirectOutSelectorComponent::Choices> directOutChoices,
                         Owner & owner,
                         GrisLookAndFeel & lookAndFeel,
                         SmallGrisLookAndFeel & smallLookAndFeel);
    ~SourceSliceComponent() override = default;
    //==============================================================================
    SourceSliceComponent(SourceSliceComponent const &) = delete;
    SourceSliceComponent(SourceSliceComponent &&) = delete;
    SourceSliceComponent & operator=(SourceSliceComponent const &) = delete;
    SourceSliceComponent & operator=(SourceSliceComponent &&) = delete;
    //==============================================================================
    void setDirectOut(tl::optional<output_patch_t> directOut);
    void setDirectOutChoices(std::shared_ptr<DirectOutSelectorComponent::Choices> choices);
    void setSourceColour(juce::Colour colour);
    void setProjectSpatMode(SpatMode spatMode);
    void setHybridSpatMode(SpatMode spatMode);
    //==============================================================================
    void muteSoloButtonClicked(PortState state) override;
    void resized() override;
    void sourceIdButtonColorChanged(SourceIdButton * button, juce::Colour color) override;
    void sourceIdButtonCopyColorToNextSource(SourceIdButton * button, juce::Colour color) override;
    [[nodiscard]] int getMinHeight() const noexcept override;
    void directOutSelectorComponentClicked(tl::optional<output_patch_t> directOut) override;

private:
    //==============================================================================
    void directOutButtonClicked() const;
    void domeButtonClicked() const;
    void cubeButtonClicked() const;
    //==============================================================================
    JUCE_LEAK_DETECTOR(AbstractSliceComponent)
}; // class LevelComponent

//==============================================================================
class SpeakerSliceComponent final
    : public AbstractSliceComponent
    , private SpeakerIdButton::Listener
{
public:
    //==============================================================================
    class Owner
    {
    public:
        Owner() = default;
        virtual ~Owner() = default;
        //==============================================================================
        Owner(Owner const &) = default;
        Owner(Owner &&) = default;
        Owner & operator=(Owner const &) = default;
        Owner & operator=(Owner &&) = default;
        //==============================================================================
        virtual void setSelectedSpeakers(juce::Array<output_patch_t> selection) = 0;
        virtual void setSpeakerState(output_patch_t outputPatch, PortState state) = 0;
    };

private:
    //==============================================================================
    output_patch_t mOutputPatch{};
    Owner & mOwner;
    SpeakerIdButton mIdButton;

public:
    //==============================================================================
    SpeakerSliceComponent(output_patch_t outputPatch,
                          Owner & owner,
                          GrisLookAndFeel & lookAndFeel,
                          SmallGrisLookAndFeel & smallLookAndFeel);
    ~SpeakerSliceComponent() override = default;
    //==============================================================================
    SpeakerSliceComponent(SpeakerSliceComponent const &) = delete;
    SpeakerSliceComponent(SpeakerSliceComponent &&) = delete;
    SpeakerSliceComponent & operator=(SpeakerSliceComponent const &) = delete;
    SpeakerSliceComponent & operator=(SpeakerSliceComponent &&) = delete;
    //==============================================================================
    void setSelected(bool value);
    //==============================================================================
    void speakerIdButtonClicked(SpeakerIdButton * button) override;
    void muteSoloButtonClicked(PortState state) override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerSliceComponent)
}; // class LevelComponent

//==============================================================================
class StereoSliceComponent final
    : public AbstractSliceComponent
    , private SpeakerIdButton::Listener
{
public:
    StereoSliceComponent(juce::String const & id,
                         GrisLookAndFeel & lookAndFeel,
                         SmallGrisLookAndFeel & smallLookAndFeel);
    ~StereoSliceComponent() override = default;

    StereoSliceComponent(StereoSliceComponent const &) = delete;
    StereoSliceComponent(StereoSliceComponent &&) = delete;
    StereoSliceComponent & operator=(StereoSliceComponent const &) = delete;
    StereoSliceComponent & operator=(StereoSliceComponent &&) = delete;

    [[nodiscard]] int getMinHeight() const noexcept override;
    void muteSoloButtonClicked(PortState) override { jassertfalse; }
    void speakerIdButtonClicked(SpeakerIdButton * /*button*/) override { jassertfalse; }
};