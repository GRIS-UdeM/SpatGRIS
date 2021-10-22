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

#include "sg_SliceComponents.hpp"

#include "sg_GrisLookAndFeel.hpp"
#include "sg_LogicStrucs.hpp"
#include "sg_MainComponent.hpp"
#include "sg_constants.hpp"

juce::String const SourceSliceComponent::NO_DIRECT_OUT_TEXT = "-";

//==============================================================================
AbstractSliceComponent::AbstractSliceComponent(juce::String const & id,
                                               GrisLookAndFeel & lookAndFeel,
                                               SmallGrisLookAndFeel & smallLookAndFeel)
    : mLookAndFeel(lookAndFeel)
    , mSmallLookAndFeel(smallLookAndFeel)
    , mLevelBox(smallLookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const initColors = [&](Component & component) {
        component.setLookAndFeel(&smallLookAndFeel);
        component.setColour(juce::Label::textColourId, smallLookAndFeel.getFontColour());
        component.setColour(juce::TextButton::textColourOnId, smallLookAndFeel.getFontColour());
        component.setColour(juce::TextButton::textColourOffId, smallLookAndFeel.getFontColour());
        component.setColour(juce::TextButton::buttonColourId, smallLookAndFeel.getBackgroundColour());
        addAndMakeVisible(component);
    };

    auto const initButton = [&](juce::Button & button) {
        button.addListener(this);
        button.addMouseListener(this, true);
        initColors(button);
    };

    auto const initLabel = [&](juce::Label & label, juce::String const & text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setInterceptsMouseClicks(false, false);
        initColors(label);
        label.setFont(juce::Font{ 0.8f });
    };

    // Id
    initButton(mIdButton);
    initLabel(mIdLabel, id);

    // Level box
    addAndMakeVisible(mLevelBox);

    addAndMakeVisible(mMuteSoloComponent);
}

//==============================================================================
void AbstractSliceComponent::setState(PortState const state, bool const soloMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mMuteSoloComponent.setPortState(state);
    mLevelBox.setMuted(soloMode ? state != PortState::solo : state == PortState::muted);

    repaint();
}

//==============================================================================
void SpeakerSliceComponent::setSelected(bool const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (value) {
        mIdButton.setColour(juce::TextButton::textColourOnId, mSmallLookAndFeel.getWinBackgroundColour());
        mIdButton.setColour(juce::TextButton::textColourOffId, mSmallLookAndFeel.getWinBackgroundColour());
        mIdButton.setColour(juce::TextButton::buttonColourId, mSmallLookAndFeel.getOnColour());
    } else {
        mIdButton.setColour(juce::TextButton::textColourOnId, mSmallLookAndFeel.getFontColour());
        mIdButton.setColour(juce::TextButton::textColourOffId, mSmallLookAndFeel.getFontColour());
        mIdButton.setColour(juce::TextButton::buttonColourId, mSmallLookAndFeel.getBackgroundColour());
    }
    repaint();
}

//==============================================================================
void SpeakerSliceComponent::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (button == &mIdButton) {
        mOwner.setSelectedSpeakers(mOutputPatch);
        return;
    }
    jassertfalse;
}

//==============================================================================
void SpeakerSliceComponent::muteSoloButtonClicked(PortState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mOwner.setSpeakerState(mOutputPatch, state);
}

//==============================================================================
int SpeakerSliceComponent::getMinHeight() const noexcept
{
    return INNER_ELEMENTS_PADDING + ID_BUTTON_HEIGHT + INNER_ELEMENTS_PADDING + mLevelBox.getMinHeight()
           + INNER_ELEMENTS_PADDING + MUTE_AND_SOLO_BUTTONS_HEIGHT + INNER_ELEMENTS_PADDING;
}

//==============================================================================
StereoSliceComponent::StereoSliceComponent(juce::String const & id,
                                           GrisLookAndFeel & lookAndFeel,
                                           SmallGrisLookAndFeel & smallLookAndFeel)
    : AbstractSliceComponent(id, lookAndFeel, smallLookAndFeel)
{
}

//==============================================================================
int StereoSliceComponent::getMinHeight() const noexcept
{
    return INNER_ELEMENTS_PADDING + ID_BUTTON_HEIGHT + INNER_ELEMENTS_PADDING + mLevelBox.getMinHeight()
           + INNER_ELEMENTS_PADDING;
}

//==============================================================================
void AbstractSliceComponent::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto yOffset{ INNER_ELEMENTS_PADDING };
    static constexpr auto AVAILABLE_WIDTH{ SLICES_WIDTH - INNER_ELEMENTS_PADDING * 2 };

    juce::Rectangle<int> const idBounds{ INNER_ELEMENTS_PADDING, yOffset, AVAILABLE_WIDTH, ID_BUTTON_HEIGHT };
    mIdLabel.setBounds(idBounds.withSizeKeepingCentre(100, 100));
    mIdButton.setBounds(idBounds);

    yOffset += ID_BUTTON_HEIGHT + INNER_ELEMENTS_PADDING;

    auto const vuMeterHeight{ std::max(mLevelBox.getMinHeight(),
                                       getHeight() - getMinHeight() + mLevelBox.getMinHeight()) };

    juce::Rectangle<int> const levelBoxBounds{ INNER_ELEMENTS_PADDING, yOffset, AVAILABLE_WIDTH, vuMeterHeight };
    mLevelBox.setBounds(levelBoxBounds);

    yOffset += vuMeterHeight + INNER_ELEMENTS_PADDING;

    mMuteSoloComponent.setBounds(INNER_ELEMENTS_PADDING, yOffset, AVAILABLE_WIDTH, MUTE_AND_SOLO_BUTTONS_HEIGHT);
}

//==============================================================================
int AbstractSliceComponent::getMinWidth() const noexcept
{
    return SLICES_WIDTH;
}

//==============================================================================
SourceSliceComponent::SourceSliceComponent(source_index_t const sourceIndex,
                                           tl::optional<output_patch_t> const directOut,
                                           SpatMode const projectSpatMode,
                                           SpatMode const hybridSpatMode,
                                           juce::Colour const colour,
                                           Owner & owner,
                                           GrisLookAndFeel & lookAndFeel,
                                           SmallGrisLookAndFeel & smallLookAndFeel)
    : AbstractSliceComponent(juce::String{ sourceIndex.get() }, lookAndFeel, smallLookAndFeel)
    , mSourceIndex(sourceIndex)
    , mOwner(owner)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const initButton = [&](juce::TextButton & button, bool const setColors = true) {
        button.addListener(this);

        addAndMakeVisible(button);

        if (!setColors) {
            return;
        }

        button.setColour(juce::Label::textColourId, smallLookAndFeel.getFontColour());
        button.setLookAndFeel(&smallLookAndFeel);
    };

    initButton(mIdButton, false);
    mIdButton.addMouseListener(this, true);

    initButton(mDirectOutButton);
    setDirectOut(directOut);

    initButton(mDomeButton);
    initButton(mCubeButton);

    setSourceColour(colour);
    setProjectSpatMode(projectSpatMode);
    setHybridSpatMode(hybridSpatMode);
}

//==============================================================================
void SourceSliceComponent::setDirectOut(tl::optional<output_patch_t> const outputPatch)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    // TODO : the following code always results in some app-crashing invalid string when the optional holds a value. Why
    // is that ? Is it possible that the perfect-forwarding of a juce::String does something weird ?

    /*static auto const PATCH_TO_STRING
        = [](output_patch_t const outputPatch) -> juce::String { return juce::String{ outputPatch.get() }; };

    auto const newText{ outputPatch.map_or(PATCH_TO_STRING, NO_DIRECT_OUT_TEXT) };*/

    auto const newText{ outputPatch ? juce::String{ outputPatch->get() } : NO_DIRECT_OUT_TEXT };
    mDirectOutButton.setButtonText(newText);
}

//==============================================================================
void SourceSliceComponent::setSourceColour(juce::Colour const colour)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mIdButton.setColour(juce::TextButton::buttonColourId, colour);
    mIdLabel.setColour(juce::Label::textColourId, colour.contrasting(1.0f));
}

//==============================================================================
void SourceSliceComponent::setProjectSpatMode(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const showHybridButtons{ spatMode == SpatMode::hybrid };
    mDomeButton.setVisible(showHybridButtons);
    mCubeButton.setVisible(showHybridButtons);
}

//==============================================================================
void SourceSliceComponent::setHybridSpatMode(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mDomeButton.setToggleState(spatMode == SpatMode::vbap, juce::dontSendNotification);
    mCubeButton.setToggleState(spatMode == SpatMode::lbap, juce::dontSendNotification);
}

//==============================================================================
void SourceSliceComponent::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (button == &mDirectOutButton) {
        directOutButtonClicked();
        return;
    }
    if (button == &mDomeButton) {
        domeButtonClicked();
        return;
    }
    if (button == &mCubeButton) {
        cubeButtonClicked();
        return;
    }
    jassertfalse;
}

//==============================================================================
void SourceSliceComponent::muteSoloButtonClicked(PortState const state)
{
    mOwner.setSourceState(mSourceIndex, state);
}

//==============================================================================
void SourceSliceComponent::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    AbstractSliceComponent::resized();

    juce::Rectangle<int> const domeButtonBounds{
        INNER_ELEMENTS_PADDING,
        getHeight() - (DIRECT_OUT_BUTTON_HEIGHT + INNER_ELEMENTS_PADDING * 3 + MUTE_AND_SOLO_BUTTONS_HEIGHT * 2),
        SLICES_WIDTH - INNER_ELEMENTS_PADDING * 2,
        MUTE_AND_SOLO_BUTTONS_HEIGHT
    };
    auto const cubeButtonBounds{ domeButtonBounds.translated(0,
                                                             MUTE_AND_SOLO_BUTTONS_HEIGHT + INNER_ELEMENTS_PADDING) };
    auto const directOutButtonBounds{
        cubeButtonBounds.translated(0, MUTE_AND_SOLO_BUTTONS_HEIGHT + INNER_ELEMENTS_PADDING)
    };

    mDomeButton.setBounds(domeButtonBounds);
    mCubeButton.setBounds(cubeButtonBounds);
    mDirectOutButton.setBounds(directOutButtonBounds);
}

//==============================================================================
void SourceSliceComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto * colorSelector{ dynamic_cast<juce::ColourSelector *>(source) };
    jassert(colorSelector);
    if (colorSelector != nullptr) {
        mOwner.setSourceColor(mSourceIndex, colorSelector->getCurrentColour());
    }
}

//==============================================================================
int SourceSliceComponent::getMinHeight() const noexcept
{
    return INNER_ELEMENTS_PADDING + ID_BUTTON_HEIGHT + INNER_ELEMENTS_PADDING + mLevelBox.getMinHeight()
           + INNER_ELEMENTS_PADDING + MUTE_AND_SOLO_BUTTONS_HEIGHT + INNER_ELEMENTS_PADDING
           + MUTE_AND_SOLO_BUTTONS_HEIGHT + INNER_ELEMENTS_PADDING + MUTE_AND_SOLO_BUTTONS_HEIGHT
           + INNER_ELEMENTS_PADDING + DIRECT_OUT_BUTTON_HEIGHT + INNER_ELEMENTS_PADDING;
}

//==============================================================================
void SourceSliceComponent::mouseUp(juce::MouseEvent const & event)
{
    if (!mIdButton.getScreenBounds().contains(event.getScreenPosition())) {
        return;
    }

    if (event.mods.isLeftButtonDown()) {
        colorSelectorLeftButtonClicked();
    } else if (event.mods.isRightButtonDown()) {
        colorSelectorRightButtonClicked();
    }
}

//==============================================================================
void SourceSliceComponent::colorSelectorLeftButtonClicked()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto colourSelector{ std::make_unique<juce::ColourSelector>(juce::ColourSelector::showColourAtTop
                                                                    | juce::ColourSelector::showSliders
                                                                    | juce::ColourSelector::showColourspace,
                                                                4,
                                                                4) };
    colourSelector->setName("background");
    colourSelector->setCurrentColour(getSourceColor());
    colourSelector->addChangeListener(this);
    colourSelector->setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
    colourSelector->setSize(300, 400);
    juce::CallOutBox::launchAsynchronously(std::move(colourSelector), getScreenBounds(), nullptr);
}

//==============================================================================
void SourceSliceComponent::colorSelectorRightButtonClicked() const
{
    source_index_t const nextSourceIndex{ mSourceIndex.get() + 1 };
    auto const currentColor{ getSourceColor() };
    mOwner.setSourceColor(nextSourceIndex, currentColor);
}

//==============================================================================
void SourceSliceComponent::directOutButtonClicked() const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static constexpr auto CHOICE_NOT_DIRECT_OUT = std::numeric_limits<int>::min();
    static constexpr auto CHOICE_CANCELED = 0;

    juce::PopupMenu menu{};
    juce::Array<output_patch_t> directOutSpeakers{};
    juce::Array<output_patch_t> nonDirectOutSpeakers{};
    for (auto const speaker : mOwner.getSpeakersData()) {
        auto & destination{ speaker.value->isDirectOutOnly ? directOutSpeakers : nonDirectOutSpeakers };
        destination.add(speaker.key);
    }
    for (auto const outputPatch : directOutSpeakers) {
        menu.addItem(outputPatch.get(), juce::String{ outputPatch.get() });
    }
    menu.addItem(CHOICE_NOT_DIRECT_OUT, NO_DIRECT_OUT_TEXT);
    for (auto const outputPatch : nonDirectOutSpeakers) {
        menu.addItem(outputPatch.get(), juce::String{ outputPatch.get() });
    }

    auto const result{ menu.show() };

    if (result == CHOICE_CANCELED) {
        return;
    }

    tl::optional<output_patch_t> newOutputPatch{};
    if (result != CHOICE_NOT_DIRECT_OUT) {
        newOutputPatch = output_patch_t{ result };
    }

    mOwner.setSourceDirectOut(mSourceIndex, newOutputPatch);
}

//==============================================================================
void SourceSliceComponent::domeButtonClicked() const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mOwner.setSourceHybridSpatMode(mSourceIndex, SpatMode::vbap);
}

//==============================================================================
void SourceSliceComponent::cubeButtonClicked() const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mOwner.setSourceHybridSpatMode(mSourceIndex, SpatMode::lbap);
}

//==============================================================================
juce::Colour SourceSliceComponent::getSourceColor() const
{
    return mIdButton.findColour(juce::TextButton::buttonColourId);
}

//==============================================================================
SpeakerSliceComponent::SpeakerSliceComponent(output_patch_t const outputPatch,
                                             Owner & owner,
                                             GrisLookAndFeel & lookAndFeel,
                                             SmallGrisLookAndFeel & smallLookAndFeel)
    : AbstractSliceComponent(juce::String{ outputPatch.get() }, lookAndFeel, smallLookAndFeel)
    , mOutputPatch(outputPatch)
    , mOwner(owner)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    setSelected(false);
}
