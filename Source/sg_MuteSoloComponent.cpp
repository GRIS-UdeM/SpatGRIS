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

#include "sg_MuteSoloComponent.hpp"

namespace gris
{
//==============================================================================
MuteSoloComponent::MuteSoloComponent(Listener & listener,
                                     GrisLookAndFeel & lookAndFeel,
                                     SmallGrisLookAndFeel & smallLookAndFeel)
    : mListener(listener)
    , mLookAndFeel(lookAndFeel)
    , mSmallLookAndFeel(smallLookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mLayout.addSection(mMuteButton).withRelativeSize(0.5f);
    mLayout.addSection(mSoloButton).withRelativeSize(0.5f);
    addAndMakeVisible(mLayout);
}

//==============================================================================
void MuteSoloComponent::setPortState(SpeakerIOState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mMuteButton.setToggleState(state == SpeakerIOState::muted);
    mSoloButton.setToggleState(state == SpeakerIOState::solo);
}

//==============================================================================
int MuteSoloComponent::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    return mLayout.getMinWidth();
}

//==============================================================================
int MuteSoloComponent::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    return mLayout.getMinHeight();
}

//==============================================================================
void MuteSoloComponent::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mLayout.setBounds(getLocalBounds());
}

//==============================================================================
void MuteSoloComponent::smallButtonClicked(SmallToggleButton * button, bool const state, bool /*isLeftMouseButton*/)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (button == &mMuteButton) {
        auto const portState{ state ? SpeakerIOState::muted : SpeakerIOState::normal };
        mListener.muteSoloButtonClicked(portState);
        return;
    }
    if (button == &mSoloButton) {
        auto const portState{ state ? SpeakerIOState::solo : SpeakerIOState::normal };
        mListener.muteSoloButtonClicked(portState);
        return;
    }
    jassertfalse;
}

} // namespace gris
