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

#include "SpatModeComponent.hpp"

#include "Constants.hpp"
#include "GrisLookAndFeel.hpp"

static constexpr auto BUTTONS_WIDTH = 60;
static constexpr auto BUTTONS_HEIGHT = 25;
static constexpr auto LABELS_HEIGHT = 20;
static constexpr auto INNER_PADDING = 1;

//==============================================================================
SpatModeComponent::SpatModeComponent(Listener & listener, GrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mSpatModeLabel("", "Algorithm Selection")
    , mStereoModeLabel("", "Stereo Reduction")
{
    using flag = juce::Button::ConnectedEdgeFlags;

    auto const initLabel = [&](juce::Label & label) {
        label.setJustificationType(juce::Justification::centred);
        label.setFont(lookAndFeel.getFont());
        label.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
        addAndMakeVisible(label);
    };

    auto const initButton = [&](juce::TextButton & button, juce::String const & text, juce::String const & tooltip) {
        button.setButtonText(text);
        button.setTooltip(tooltip);
        button.setClickingTogglesState(true);
        button.setRadioGroupId(SPAT_MODE_BUTTONS_RADIO_GROUP_ID, juce::dontSendNotification);
        button.addListener(this);
        addAndMakeVisible(button);
    };

    initLabel(mSpatModeLabel);
    initLabel(mStereoModeLabel);

    initButton(mDomeButton, spatModeToString(SpatMode::vbap), spatModeToTooltip(SpatMode::vbap));
    initButton(mCubeButton, spatModeToString(SpatMode::lbap), spatModeToTooltip(SpatMode::lbap));
    mDomeButton.setToggleState(true, juce::dontSendNotification);

    auto stereoModeStrings{ STEREO_MODE_STRINGS };
    stereoModeStrings.insert(0, "None");
    mStereoComboBox.addItemList(stereoModeStrings, 1);
    mStereoComboBox.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mStereoComboBox.addListener(this);
    addAndMakeVisible(mStereoComboBox);
}

//==============================================================================
void SpatModeComponent::setSpatMode(SpatMode const spatMode)
{
    switch (spatMode) {
    case SpatMode::vbap:
        mDomeButton.setToggleState(true, juce::dontSendNotification);
        return;
    case SpatMode::lbap:
        mCubeButton.setToggleState(true, juce::dontSendNotification);
        return;
    }
    jassertfalse;
}

//==============================================================================
void SpatModeComponent::setStereoMode(tl::optional<StereoMode> const & mode)
{
    if (!mode) {
        mStereoComboBox.setSelectedItemIndex(0, juce::dontSendNotification);
        return;
    }

    auto const index{ static_cast<int>(*mode) + 1 };
    mStereoComboBox.setSelectedItemIndex(index, juce::dontSendNotification);
}

//==============================================================================
SpatMode SpatModeComponent::getSpatMode() const noexcept
{
    if (mDomeButton.getToggleState()) {
        return SpatMode::vbap;
    }
    jassert(mCubeButton.getToggleState());
    return SpatMode::lbap;
}

//==============================================================================
tl::optional<StereoMode> SpatModeComponent::getStereoMode() const noexcept
{
    auto const selectedIndex{ mStereoComboBox.getSelectedItemIndex() };

    jassert(selectedIndex >= 0 && selectedIndex <= 2);

    if (selectedIndex == 0) {
        return tl::nullopt;
    }

    return static_cast<StereoMode>(selectedIndex - 1);
}

//==============================================================================
void SpatModeComponent::buttonClicked(juce::Button * button)
{
    if (!button->getToggleState()) {
        return;
    }

    if (button != &mDomeButton && button != &mCubeButton) {
        jassertfalse;
        return;
    }

    mListener.handleSpatModeChanged(getSpatMode());
}

//==============================================================================
void SpatModeComponent::comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged != &mStereoComboBox) {
        jassertfalse;
        return;
    }

    mListener.handleStereoModeChanged(getStereoMode());
}

//==============================================================================
void SpatModeComponent::resized()
{
    auto const availableWidth{ getWidth() };
    auto const availableHeight{ getHeight() };

    auto const minWidth{ getMinWidth() };
    auto const minHeight{ getMinHeight() };

    auto const xPadding{ std::max(availableWidth - minWidth, 0) / 2 };
    auto const yPadding{ std::max(availableHeight - minHeight, 0) / 2 };

    juce::Rectangle<int> const labelBaseBounds{ availableWidth, LABELS_HEIGHT };
    juce::Rectangle<int> const buttonBaseBounds{ BUTTONS_WIDTH, BUTTONS_HEIGHT };

    auto x{ 0 };
    auto y{ yPadding };

    mSpatModeLabel.setBounds(labelBaseBounds.translated(x, y));
    x = xPadding;
    y += LABELS_HEIGHT + INNER_PADDING;
    mDomeButton.setBounds(buttonBaseBounds.translated(x, y));
    x += BUTTONS_WIDTH + INNER_PADDING;
    mCubeButton.setBounds(buttonBaseBounds.translated(x, y));
    x = 0;
    y += BUTTONS_HEIGHT + INNER_PADDING;
    mStereoModeLabel.setBounds(labelBaseBounds.translated(x, y));
    x = 0;
    y += LABELS_HEIGHT + INNER_PADDING;
    mStereoComboBox.setBounds(labelBaseBounds.translated(x, y));
}

//==============================================================================
int SpatModeComponent::getMinWidth() const noexcept
{
    static auto constexpr MIN_WIDTH = BUTTONS_WIDTH * 2 + INNER_PADDING;
    return MIN_WIDTH;
}

//==============================================================================
int SpatModeComponent::getMinHeight() const noexcept
{
    static auto constexpr MIN_HEIGHT = BUTTONS_HEIGHT * 2 + LABELS_HEIGHT * 2 + INNER_PADDING * 3;
    return MIN_HEIGHT;
}
