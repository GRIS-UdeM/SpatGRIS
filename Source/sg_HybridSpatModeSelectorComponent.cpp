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

#include "sg_HybridSpatModeSelectorComponent.hpp"

namespace gris
{
//==============================================================================
HybridSpatModeSelectorComponent::HybridSpatModeSelectorComponent(SpatMode const hybridSpatMode,
                                                                 Listener & listener,
                                                                 SmallGrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mDomeButton(true, "dom", "Set this source to DOME mode", *this, lookAndFeel)
    , mCubeButton(true, "cub", "Set this source to CUBE mode", *this, lookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    addAndMakeVisible(mDomeButton);
    addAndMakeVisible(mCubeButton);
    setSpatMode(hybridSpatMode);
}

//==============================================================================
void HybridSpatModeSelectorComponent::setSpatMode(SpatMode const spatMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const isDome{ [&]() {
        switch (spatMode) {
        case SpatMode::lbap:
            return false;
        case SpatMode::vbap:
            return true;
        case SpatMode::hybrid:
            break;
        }
        jassertfalse;
        return true;
    }() };

    mDomeButton.setToggleState(isDome);
    mCubeButton.setToggleState(!isDome);
}

//==============================================================================
void HybridSpatModeSelectorComponent::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const localBounds{ getLocalBounds() };
    auto const halfHeight{ localBounds.getHeight() / 2 };
    mDomeButton.setBounds(localBounds.withBottom(halfHeight));
    mCubeButton.setBounds(localBounds.withTop(halfHeight));
}

//==============================================================================
int HybridSpatModeSelectorComponent::getMinHeight() const noexcept
{
    return mDomeButton.getMinHeight() + mCubeButton.getMinHeight();
}

//==============================================================================
void HybridSpatModeSelectorComponent::smallButtonClicked(SmallToggleButton * button,
                                                         bool state,
                                                         bool /*isLeftMouseButton*/)
{
    if (button == &mDomeButton) {
        auto const spatMode{ state ? SpatMode::vbap : SpatMode::lbap };
        mListener.hybridSpatModeSelectorClicked(spatMode);
        return;
    }
    jassert(button == &mCubeButton);
    auto const spatMode{ state ? SpatMode::lbap : SpatMode::vbap };
    mListener.hybridSpatModeSelectorClicked(spatMode);
}

} // namespace gris