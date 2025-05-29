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

#pragma once

#include "sg_MuteSoloComponent.hpp"
#include "sg_VuMeterComponent.hpp"

namespace gris
{
class GrisLookAndFeel;
class SmallGrisLookAndFeel;

//==============================================================================
class AbstractSliceComponent
    : public MinSizedComponent
    , public MuteSoloComponent::Listener
{
protected:
    static constexpr auto INNER_ELEMENTS_PADDING = 1;

    LayoutComponent mLayout;
    VuMeterComponent mVuMeter;
    MuteSoloComponent mMuteSoloComponent;

public:
    //==============================================================================
    explicit AbstractSliceComponent(GrisLookAndFeel & lookAndFeel, SmallGrisLookAndFeel & smallLookAndFeel);
    //==============================================================================
    AbstractSliceComponent() = delete;
    ~AbstractSliceComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(AbstractSliceComponent)
    //==============================================================================
    void setLevel(dbfs_t const level) { mVuMeter.setLevel(level); }
    void resetClipping() { mVuMeter.resetClipping(); }
    void setState(SpeakerIOState state, bool soloMode);
    //==============================================================================
    [[nodiscard]] int getMinWidth() const noexcept final { return SLICES_WIDTH; }
    [[nodiscard]] int getMinHeight() const noexcept final { return mLayout.getMinHeight(); }
    void resized() final { mLayout.setBounds(getLocalBounds()); }

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(AbstractSliceComponent)
}; // class LevelComponent

} // namespace gris
