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

#include "sg_AbstractSliceComponent.hpp"
#include "sg_DirectOutSelectorComponent.hpp"
#include "sg_SourceIdButton.hpp"

class GrisLookAndFeel;
class SmallGrisLookAndFeel;

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