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
    static constexpr auto INNER_ELEMENTS_PADDING = 1;

    LayoutComponent mLayout;
    VuMeterComponent mVuMeter;
    MuteSoloComponent mMuteSoloComponent;

public:
    //==============================================================================
    explicit AbstractSliceComponent(GrisLookAndFeel & lookAndFeel, SmallGrisLookAndFeel & smallLookAndFeel);
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
    [[nodiscard]] int getMinWidth() const noexcept final { return SLICES_WIDTH; }
    [[nodiscard]] int getMinHeight() const noexcept final { return mLayout.getMinHeight(); }
    void resized() final { mLayout.setBounds(getLocalBounds()); }

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
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        //==============================================================================
        Listener(Listener const &) = default;
        Listener(Listener &&) = default;
        Listener & operator=(Listener const &) = default;
        Listener & operator=(Listener &&) = default;
        //==============================================================================
        virtual void setSourceDirectOut(source_index_t sourceIndex, tl::optional<output_patch_t> outputPatch) = 0;
        virtual void setSourceColor(source_index_t sourceIndex, juce::Colour colour) = 0;
        virtual void setSourceState(source_index_t sourceIndex, PortState state) = 0;
        virtual void setSourceHybridSpatMode(source_index_t sourceIndex, SpatMode spatMode) = 0;
    };

private:
    //==============================================================================
    Listener & mOwner;
    source_index_t mSourceIndex{};

    SourceIdButton mIdButton;
    DirectOutSelectorComponent mDirectOutSelectorComponent;

public:
    //==============================================================================
    SourceSliceComponent(source_index_t sourceIndex,
                         tl::optional<output_patch_t> directOut,
                         SpatMode projectSpatMode,
                         SpatMode hybridSpatMode,
                         juce::Colour colour,
                         std::shared_ptr<DirectOutSelectorComponent::Choices> directOutChoices,
                         Listener & owner,
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
    void sourceIdButtonColorChanged(SourceIdButton * button, juce::Colour color) override;
    void sourceIdButtonCopyColorToNextSource(SourceIdButton * button, juce::Colour color) override;
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
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        //==============================================================================
        Listener(Listener const &) = default;
        Listener(Listener &&) = default;
        Listener & operator=(Listener const &) = default;
        Listener & operator=(Listener &&) = default;
        //==============================================================================
        virtual void setSelectedSpeakers(juce::Array<output_patch_t> selection) = 0;
        virtual void setSpeakerState(output_patch_t outputPatch, PortState state) = 0;
    };

private:
    //==============================================================================
    Listener & mOwner;
    output_patch_t mOutputPatch{};

    SpeakerIdButton mIdButton;

public:
    //==============================================================================
    SpeakerSliceComponent(output_patch_t outputPatch,
                          Listener & owner,
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

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerSliceComponent)
}; // class LevelComponent

//==============================================================================
class StereoSliceComponent final
    : public AbstractSliceComponent
    , private SmallToggleButton::Listener
{
    SmallToggleButton mIdButton;

public:
    //==============================================================================
    StereoSliceComponent(juce::String const & id,
                         GrisLookAndFeel & lookAndFeel,
                         SmallGrisLookAndFeel & smallLookAndFeel);
    ~StereoSliceComponent() override = default;
    //==============================================================================
    StereoSliceComponent(StereoSliceComponent const &) = delete;
    StereoSliceComponent(StereoSliceComponent &&) = delete;
    StereoSliceComponent & operator=(StereoSliceComponent const &) = delete;
    StereoSliceComponent & operator=(StereoSliceComponent &&) = delete;
    //==============================================================================
    void muteSoloButtonClicked(PortState) override { jassertfalse; }
    void smallButtonClicked(SmallToggleButton * /*button*/, bool /*state*/, bool /*isLeftMouseButton*/) override {}

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(StereoSliceComponent)
};