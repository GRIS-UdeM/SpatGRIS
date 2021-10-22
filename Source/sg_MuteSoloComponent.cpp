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

#include "sg_MuteSoloComponent.hpp"

//==============================================================================
MuteSoloComponent::MuteSoloComponent(Listener & listener,
                                     GrisLookAndFeel & lookAndFeel,
                                     SmallGrisLookAndFeel & smallLookAndFeel)
    : mListener(listener)
    , mLookAndFeel(lookAndFeel)
    , mSmallLookAndFeel(smallLookAndFeel)
{
    mLayout.addSection(mMuteButton).withRelativeSize(0.5f);
    mLayout.addSection(mSoloButton).withRelativeSize(0.5f);
    addAndMakeVisible(mLayout);
}

//==============================================================================
void MuteSoloComponent::setPortState(PortState const state)
{
    mMuteButton.setToggleState(state == PortState::muted);
    mSoloButton.setToggleState(state == PortState::solo);
}

//==============================================================================
int MuteSoloComponent::getMinWidth() const noexcept
{
    return mLayout.getMinWidth();
}

//==============================================================================
int MuteSoloComponent::getMinHeight() const noexcept
{
    return mLayout.getMinHeight();
}

//==============================================================================
void MuteSoloComponent::resized()
{
    mLayout.setBounds(getLocalBounds());
}

//==============================================================================
void MuteSoloComponent::smallButtonClicked(SmallToggleButton * button, bool const state)
{
    if (button == &mMuteButton) {
        auto const portState{ state ? PortState::muted : PortState::normal };
        mListener.muteSoloButtonClicked(portState);
        return;
    }
    if (button == &mSoloButton) {
        auto const portState{ state ? PortState::solo : PortState::normal };
        mListener.muteSoloButtonClicked(portState);
        return;
    }
    jassertfalse;
}
