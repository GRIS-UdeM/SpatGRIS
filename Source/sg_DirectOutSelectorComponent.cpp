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

#include "sg_DirectOutSelectorComponent.hpp"

#include "sg_GrisLookAndFeel.hpp"

//==============================================================================
DirectOutSelectorComponent::DirectOutSelectorComponent(tl::optional<output_patch_t> const & directOut,
                                                       std::shared_ptr<Choices> choices,
                                                       Listener & listener,
                                                       SmallGrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mChoices(std::move(choices))
    , mButton("", "Direct out")
{
    mButton.addListener(this);

    addAndMakeVisible(mButton);

    mButton.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    mButton.setLookAndFeel(&lookAndFeel);

    setDirectOut(directOut);
}

//==============================================================================
void DirectOutSelectorComponent::buttonClicked([[maybe_unused]] juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(button == &mButton);
    jassert(mChoices);

    static constexpr auto CHOICE_NOT_DIRECT_OUT = std::numeric_limits<int>::min();
    static constexpr auto CHOICE_CANCELED = 0;

    juce::PopupMenu menu{};
    juce::Array<output_patch_t> directOutSpeakers{};
    juce::Array<output_patch_t> nonDirectOutSpeakers{};
    for (auto const outputPatch : mChoices->directOutputPatches) {
        menu.addItem(outputPatch.get(), juce::String{ outputPatch.get() });
    }
    menu.addItem(CHOICE_NOT_DIRECT_OUT, NO_DIRECT_OUT_STRING);
    for (auto const outputPatch : mChoices->nonDirectOutputPatches) {
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

    mListener.directOutSelectorComponentClicked(newOutputPatch);
}

//==============================================================================
int DirectOutSelectorComponent::getMinWidth() const noexcept
{
    return 0;
}

//==============================================================================
int DirectOutSelectorComponent::getMinHeight() const noexcept
{
    return 17;
}

//==============================================================================
void DirectOutSelectorComponent::setDirectOut(tl::optional<output_patch_t> const & directOut)
{
    mButton.setButtonText(directOut.has_value() ? juce::String{ directOut->get() } : NO_DIRECT_OUT_STRING);

    // if (!directOut) {
    //    return;
    //}

    // jassert(mChoices->nonDirectOutputPatches.contains(*directOut)
    //        || mChoices->directOutputPatches.contains(*directOut));
}

//==============================================================================
void DirectOutSelectorComponent::setChoices(std::shared_ptr<Choices> choices)
{
    mChoices = std::move(choices);
}

//==============================================================================
void DirectOutSelectorComponent::resized()
{
    mButton.setBounds(getLocalBounds());
}
