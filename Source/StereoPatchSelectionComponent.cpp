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

#include "StereoPatchSelectionComponent.hpp"

#include "GrisLookAndFeel.hpp"

static constexpr auto LABELS_WIDTH = 45;
static constexpr auto ROWS_HEIGHT = 20;
static constexpr auto COMBO_BOXES_WIDTH = 40;
static constexpr auto PADDING = 5;

//==============================================================================
StereoPatchSelectionComponent::StereoPatchSelectionComponent(Listener & listener, GrisLookAndFeel & lookAndFeel)
    : mListener(listener)
{
    auto const initLabel = [&](juce::Label & label, juce::String const & text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredRight);
        label.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
        addAndMakeVisible(label);
    };

    initLabel(mLeftLabel, "Left: ");
    initLabel(mRightLabel, "Right: ");

    mLeftCombobox.addListener(this);
    mRightCombobox.addListener(this);

    addAndMakeVisible(mLeftCombobox);
    addAndMakeVisible(mRightCombobox);
}

//==============================================================================
void StereoPatchSelectionComponent::setStereoRouting(StereoRouting const & routing)
{
    jassert(routing.left != routing.right);

    mLeftCombobox.setSelectedId(routing.left.get(), juce::dontSendNotification);
    mRightCombobox.setSelectedId(routing.right.get(), juce::dontSendNotification);

    updateEnabledItems();
}

//==============================================================================
void StereoPatchSelectionComponent::updateSpeakers(SpeakersOrdering speakers)
{
    speakers.sort();

    auto const selectedLeftId{ mLeftCombobox.getSelectedId() };
    auto const selectedRightId{ mRightCombobox.getSelectedId() };

    mLeftCombobox.clear(juce::dontSendNotification);
    mRightCombobox.clear(juce::dontSendNotification);

    for (auto const & outputPatch : speakers) {
        auto const outputPatchInt{ outputPatch.get() };
        mLeftCombobox.addItem(juce::String{ outputPatchInt }, outputPatchInt);
        mRightCombobox.addItem(juce::String{ outputPatchInt }, outputPatchInt);
    }

    mLeftCombobox.setSelectedId(selectedLeftId, juce::dontSendNotification);
    mRightCombobox.setSelectedId(selectedRightId, juce::dontSendNotification);

    updateEnabledItems();
}

//==============================================================================
void StereoPatchSelectionComponent::comboBoxChanged(juce::ComboBox * /*comboBoxThatHasChanged*/)
{
    StereoRouting const routing{ output_patch_t{ mLeftCombobox.getSelectedId() },
                                 output_patch_t{ mRightCombobox.getSelectedId() } };
    jassert(routing.left != routing.right);

    mListener.handleStereoRoutingChanged(routing);
    updateEnabledItems();
}

//==============================================================================
void StereoPatchSelectionComponent::resized()
{
    auto const availableWidth{ getWidth() };
    auto const availableHeight{ getHeight() };

    auto const width{ getMinWidth() };
    auto const height{ getMinHeight() };

    auto const xOffset{ std::max((availableWidth - width) / 2, 0) };
    auto const yOffset{ std::max((availableHeight - height) / 2, 0) };

    auto x{ xOffset };
    auto y{ yOffset };

    mLeftLabel.setBounds(x, y, LABELS_WIDTH, ROWS_HEIGHT);
    x += LABELS_WIDTH + PADDING;
    mLeftCombobox.setBounds(x, y, COMBO_BOXES_WIDTH, ROWS_HEIGHT);

    x = xOffset;
    y += ROWS_HEIGHT + PADDING;
    mRightLabel.setBounds(x, y, LABELS_WIDTH, ROWS_HEIGHT);
    x += LABELS_WIDTH + PADDING;
    mRightCombobox.setBounds(x, y, COMBO_BOXES_WIDTH, ROWS_HEIGHT);
}

//==============================================================================
int StereoPatchSelectionComponent::getMinWidth() const noexcept
{
    return LABELS_WIDTH + PADDING + COMBO_BOXES_WIDTH;
}

//==============================================================================
int StereoPatchSelectionComponent::getMinHeight() const noexcept
{
    return ROWS_HEIGHT * 2 + PADDING;
}

//==============================================================================
void StereoPatchSelectionComponent::updateEnabledItems()
{
    auto const leftId{ mLeftCombobox.getSelectedId() };
    auto const rightId{ mRightCombobox.getSelectedId() };

    jassert(mLeftCombobox.getNumItems() == mRightCombobox.getNumItems());
    jassert((leftId == 0 && rightId == 0) || (mLeftCombobox.getSelectedId() != mRightCombobox.getSelectedId()));

    for (int i{}; i < mLeftCombobox.getNumItems(); ++i) {
        auto const itemId{ mLeftCombobox.getItemId(i) };
        auto const leftEnabled{ itemId != rightId };
        auto const rightEnabled{ itemId != leftId };
        mLeftCombobox.setItemEnabled(itemId, leftEnabled);
        mRightCombobox.setItemEnabled(itemId, rightEnabled);
    }
}
