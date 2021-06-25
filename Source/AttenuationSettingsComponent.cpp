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

#include "AttenuationSettingsComponent.hpp"

#include "GrisLookAndFeel.hpp"
#include "constants.hpp"

static constexpr auto LABEL_WIDTH = 120;
static constexpr auto LABEL_HEIGHT = 18;
static constexpr auto COMBOBOX_WIDTH = 80;
static constexpr auto COMBOBOX_HEIGHT = 22;
static constexpr auto COMBOBOX_PADDING = 3;

//==============================================================================
AttenuationSettingsComponent::AttenuationSettingsComponent(Listener & listener, GrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLookAndFeel(lookAndFeel)
    , mComboboxInfos(getInfos())
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mLabel.setText("Attenuation Settings", juce::dontSendNotification);
    mLabel.setJustificationType(juce::Justification::centredTop);
    mLabel.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mLabel);

    auto const initCombobox = [&](juce::ComboBox & combobox, juce::StringArray const & itemInfos) {
        combobox.setJustificationType(juce::Justification::centred);
        combobox.addItemList(itemInfos, 1);
        combobox.addListener(this);

        addAndMakeVisible(combobox);
    };

    initCombobox(mDbCombobox, mComboboxInfos.dbStrings);
    initCombobox(mHzComboBox, mComboboxInfos.hzStrings);
}

//==============================================================================
void AttenuationSettingsComponent::setAttenuationDb(dbfs_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const index{ std::max(mComboboxInfos.dbValues.indexOf(value), 0) };
    mDbCombobox.setSelectedItemIndex(index, juce::dontSendNotification);
}

//==============================================================================
void AttenuationSettingsComponent::setAttenuationHz(hz_t const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const index{ std::max(mComboboxInfos.hzValues.indexOf(value), 0) };
    mHzComboBox.setSelectedItemIndex(index, juce::dontSendNotification);
}

//==============================================================================
void AttenuationSettingsComponent::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const totalBounds{ getLocalBounds() };

    auto const labelBounds{ totalBounds.withHeight(LABEL_HEIGHT) };
    auto const boundsAvailableForComboboxes{ totalBounds.withTrimmedTop(LABEL_HEIGHT)
                                                 .withSizeKeepingCentre(COMBOBOX_WIDTH,
                                                                        COMBOBOX_HEIGHT * 2 + COMBOBOX_PADDING) };

    auto const dbComboboxBounds{ boundsAvailableForComboboxes.withHeight(COMBOBOX_HEIGHT) };
    auto const hzComboboxBounds{ dbComboboxBounds.translated(0, COMBOBOX_HEIGHT + COMBOBOX_PADDING) };

    mLabel.setBounds(labelBounds);
    mDbCombobox.setBounds(dbComboboxBounds);
    mHzComboBox.setBounds(hzComboboxBounds);
}

//==============================================================================
void AttenuationSettingsComponent::comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged)
{
    auto const index{ std::max(comboBoxThatHasChanged->getSelectedItemIndex(), 0) };
    if (comboBoxThatHasChanged == &mDbCombobox) {
        auto const value{ mComboboxInfos.dbValues[index] };
        mListener.cubeAttenuationDbChanged(value);
        return;
    }

    if (comboBoxThatHasChanged == &mHzComboBox) {
        auto const value{ mComboboxInfos.hzValues[index] };
        mListener.cubeAttenuationHzChanged(value);
        return;
    }

    jassertfalse;
}

//==============================================================================
int AttenuationSettingsComponent::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static constexpr auto MIN_WIDTH{ COMBOBOX_WIDTH > LABEL_WIDTH ? COMBOBOX_WIDTH : LABEL_WIDTH };

    return MIN_WIDTH;
}

//==============================================================================
int AttenuationSettingsComponent::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    return LABEL_HEIGHT + COMBOBOX_HEIGHT * 2 + COMBOBOX_PADDING;
}

//==============================================================================
AttenuationSettingsComponent::Infos AttenuationSettingsComponent::getInfos()
{
    Infos result{};

    for (auto const & string : ATTENUATION_DB_STRINGS) {
        result.dbValues.add(dbfs_t{ string.getFloatValue() });
        result.dbStrings.add(string + " db");
    }

    for (auto const & string : ATTENUATION_FREQUENCY_STRINGS) {
        result.hzValues.add(hz_t{ string.getFloatValue() });
        result.hzStrings.add(string + " Hz");
    }

    return result;
}
