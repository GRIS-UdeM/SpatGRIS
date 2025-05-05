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

#include "AlgoGRIS/Data/StrongTypes/sg_Dbfs.hpp"
#include "sg_MinSizedComponent.hpp"

namespace gris
{
class SmallGrisLookAndFeel;

class VuMeterComponent final : public MinSizedComponent
{
    static constexpr dbfs_t MIN_LEVEL_COMP{ -60.0f };
    static constexpr dbfs_t MAX_LEVEL_COMP{ 0.0f };

    SmallGrisLookAndFeel & mLookAndFeel;

    juce::ColourGradient mColorGrad;
    juce::Image mVuMeterBit;
    juce::Image mVuMeterBackBit;
    juce::Image mVuMeterMutedBit;
    bool mIsClipping{};
    bool mIsMuted{};
    dbfs_t mLevel{ MIN_LEVEL_COMP };

public:
    //==============================================================================
    explicit VuMeterComponent(SmallGrisLookAndFeel & lnf) : mLookAndFeel(lnf) {}
    ~VuMeterComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(VuMeterComponent)
    //==============================================================================
    void resized() override;
    void resetClipping();
    void setLevel(dbfs_t level);
    void setMuted(bool muted);
    //==============================================================================
    void paint(juce::Graphics & g) override;
    void mouseDown(const juce::MouseEvent & e) override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(VuMeterComponent)
};

} // namespace gris
