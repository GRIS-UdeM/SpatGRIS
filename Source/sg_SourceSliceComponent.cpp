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

#include "sg_SourceSliceComponent.hpp"

//==============================================================================
SourceSliceComponent::SourceSliceComponent(source_index_t const sourceIndex,
                                           tl::optional<output_patch_t> const directOut,
                                           SpatMode const projectSpatMode,
                                           SpatMode const hybridSpatMode,
                                           juce::Colour const colour,
                                           std::shared_ptr<DirectOutSelectorComponent::Choices> directOutChoices,
                                           Listener & owner,
                                           GrisLookAndFeel & lookAndFeel,
                                           SmallGrisLookAndFeel & smallLookAndFeel)
    : AbstractSliceComponent(lookAndFeel, smallLookAndFeel)
    , mOwner(owner)
    , mSourceIndex(sourceIndex)
    , mIdButton(sourceIndex, colour, *this, smallLookAndFeel)
    , mHybridSpatModeSelectorComponent(hybridSpatMode, *this, smallLookAndFeel)
    , mDirectOutSelectorComponent(directOut, std::move(directOutChoices), *this, smallLookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    setProjectSpatMode(projectSpatMode);
}

//==============================================================================
void SourceSliceComponent::setDirectOut(tl::optional<output_patch_t> const directOut)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mDirectOutSelectorComponent.setDirectOut(directOut);
}

//==============================================================================
void SourceSliceComponent::setDirectOutChoices(std::shared_ptr<DirectOutSelectorComponent::Choices> choices)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mDirectOutSelectorComponent.setChoices(std::move(choices));
}

//==============================================================================
void SourceSliceComponent::setSourceColour(juce::Colour const colour)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mIdButton.setColor(colour);
}

//==============================================================================
void SourceSliceComponent::setProjectSpatMode(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mShouldShowHybridSpatModes = spatMode == SpatMode::hybrid;
    rebuildLayout();
}

//==============================================================================
void SourceSliceComponent::setHybridSpatMode(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mHybridSpatModeSelectorComponent.setSpatMode(spatMode);
}

//==============================================================================
void SourceSliceComponent::muteSoloButtonClicked(PortState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mOwner.setSourceState(mSourceIndex, state);
}

//==============================================================================
void SourceSliceComponent::sourceIdButtonColorChanged([[maybe_unused]] SourceIdButton * button,
                                                      juce::Colour const color)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(button == &mIdButton);

    mOwner.setSourceColor(mSourceIndex, color);
}

//==============================================================================
void SourceSliceComponent::sourceIdButtonCopyColorToNextSource([[maybe_unused]] SourceIdButton * button,
                                                               juce::Colour const color)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(button == &mIdButton);

    source_index_t const nextSourceIndex{ mSourceIndex.get() + 1 };
    mOwner.setSourceColor(nextSourceIndex, color);
}

//==============================================================================
void SourceSliceComponent::directOutSelectorComponentClicked(tl::optional<output_patch_t> const directOut)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mOwner.setSourceDirectOut(mSourceIndex, directOut);
}

//==============================================================================
void SourceSliceComponent::hybridSpatModeSelectorClicked(SpatMode const newHybridSpatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mOwner.setSourceHybridSpatMode(mSourceIndex, newHybridSpatMode);
}

//==============================================================================
void SourceSliceComponent::rebuildLayout()
{
    mLayout.clearSections();

    mLayout.addSection(mIdButton).withChildMinSize().withPadding(INNER_ELEMENTS_PADDING);
    if (mShouldShowHybridSpatModes) {
        mLayout.addSection(mHybridSpatModeSelectorComponent)
            .withChildMinSize()
            .withHorizontalPadding(INNER_ELEMENTS_PADDING)
            .withBottomPadding(INNER_ELEMENTS_PADDING);
    }
    mLayout.addSection(mVuMeter).withRelativeSize(1.0f).withHorizontalPadding(INNER_ELEMENTS_PADDING);
    mLayout.addSection(mMuteSoloComponent).withChildMinSize().withPadding(INNER_ELEMENTS_PADDING);
    mLayout.addSection(mDirectOutSelectorComponent)
        .withChildMinSize()
        .withHorizontalPadding(INNER_ELEMENTS_PADDING)
        .withBottomPadding(INNER_ELEMENTS_PADDING);
}
