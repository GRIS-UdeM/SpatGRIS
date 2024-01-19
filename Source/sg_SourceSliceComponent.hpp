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

#include "sg_AbstractSliceComponent.hpp"
#include "sg_DirectOutSelectorComponent.hpp"
#include "sg_HybridSpatModeSelectorComponent.hpp"
#include "sg_SourceIdButton.hpp"

namespace gris
{
class GrisLookAndFeel;
class SmallGrisLookAndFeel;

//==============================================================================
class SourceSliceComponent final
    : public AbstractSliceComponent
    , private DirectOutSelectorComponent::Listener
    , private SourceIdButton::Listener
    , private HybridSpatModeSelectorComponent::Listener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        SG_DEFAULT_COPY_AND_MOVE(Listener)
        //==============================================================================
        virtual source_index_t getNextProjectSourceIndex(source_index_t currentSourceIndex) = 0;
        //==============================================================================
        virtual void setSourceDirectOut(source_index_t sourceIndex, tl::optional<output_patch_t> outputPatch) = 0;
        virtual void setSourceColor(source_index_t sourceIndex, juce::Colour colour) = 0;
        virtual void setSourceState(source_index_t sourceIndex, SliceState state) = 0;
        virtual void setSourceHybridSpatMode(source_index_t sourceIndex, SpatMode spatMode) = 0;
        virtual void setSourceNewSourceIndex(source_index_t oldSourceIndex, source_index_t newSourceIndex) = 0;
    };

private:
    //==============================================================================
    Listener & mOwner;
    source_index_t mSourceIndex{};

    SourceIdButton mIdButton;
    HybridSpatModeSelectorComponent mHybridSpatModeSelectorComponent;
    DirectOutSelectorComponent mDirectOutSelectorComponent;

    bool mShouldShowHybridSpatModes{};

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
    SG_DELETE_COPY_AND_MOVE(SourceSliceComponent)
    //==============================================================================
    void setDirectOut(tl::optional<output_patch_t> directOut);
    void setDirectOutChoices(std::shared_ptr<DirectOutSelectorComponent::Choices> choices);
    void setSourceColour(juce::Colour colour);
    void setProjectSpatMode(SpatMode spatMode);
    void setHybridSpatMode(SpatMode spatMode);
    //==============================================================================
    void muteSoloButtonClicked(SliceState state) override;
    void sourceIdButtonColorChanged(SourceIdButton * button, juce::Colour color) override;
    void sourceIdButtonCopyColorToNextSource(SourceIdButton * button, juce::Colour color) override;
    void sourceIdButtonSourceIndexChanged(SourceIdButton * button, source_index_t newSourceIndex) override;
    void directOutSelectorComponentClicked(tl::optional<output_patch_t> directOut) override;
    void hybridSpatModeSelectorClicked(SpatMode newHybridSpatMode) override;

private:
    //==============================================================================
    void rebuildLayout();
    //==============================================================================
    JUCE_LEAK_DETECTOR(AbstractSliceComponent)
}; // class LevelComponent

} // namespace gris
