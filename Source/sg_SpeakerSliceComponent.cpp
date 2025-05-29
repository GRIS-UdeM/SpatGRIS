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

#include "sg_SpeakerSliceComponent.hpp"

namespace gris
{
//==============================================================================
void SpeakerSliceComponent::setSelected(bool const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mIdButton.setSelected(value);
}

//==============================================================================
void SpeakerSliceComponent::speakerIdButtonClicked([[maybe_unused]] SpeakerIdButton * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(button == &mIdButton);

    mOwner.setSelectedSpeakers(mOutputPatch);
}

//==============================================================================
void SpeakerSliceComponent::muteSoloButtonClicked(SpeakerIOState const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mOwner.setSpeakerState(mOutputPatch, state);
}

//==============================================================================
SpeakerSliceComponent::SpeakerSliceComponent(output_patch_t const outputPatch,
                                             Listener & owner,
                                             GrisLookAndFeel & lookAndFeel,
                                             SmallGrisLookAndFeel & smallLookAndFeel)
    : AbstractSliceComponent(lookAndFeel, smallLookAndFeel)
    , mOwner(owner)
    , mOutputPatch(outputPatch)
    , mIdButton(outputPatch, *this, smallLookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mLayout.addSection(mIdButton).withChildMinSize();
    mLayout.addSection(mVuMeter).withRelativeSize(1.0f).withHorizontalPadding(INNER_ELEMENTS_PADDING);
    mLayout.addSection(mMuteSoloComponent).withChildMinSize().withHorizontalPadding(INNER_ELEMENTS_PADDING);

    setSelected(false);
}

} // namespace gris
