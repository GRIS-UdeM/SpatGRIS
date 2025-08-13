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

#include "Data/sg_Narrow.hpp"
#include "sg_MinSizedComponent.hpp"

namespace gris
{
class GrisLookAndFeel;

//==============================================================================
class LayoutComponent final : public MinSizedComponent
{
    static constexpr auto SCROLL_BAR_WIDTH = 15;

public:
    enum class Orientation { horizontal, vertical };
    //==============================================================================
    class Section
    {
    public:
        enum class Mode { undefined, childMinSize, fixed, relative };
        friend LayoutComponent;

    private:
        juce::Component * mComponent{};
        Mode mMode{ Mode::undefined };
        int mFixedSize{};
        float mRelativeSize{};
        int mTopPadding{};
        int mLeftPadding{};
        int mBottomPadding{};
        int mRightPadding{};

    public:
        //==============================================================================
        Section & withFixedSize(int value);
        Section & withRelativeSize(float value);
        Section & withChildMinSize();
        Section & withTopPadding(int value);
        Section & withLeftPadding(int value);
        Section & withBottomPadding(int value);
        Section & withRightPadding(int value);
        Section & withHorizontalPadding(int value);
        Section & withVerticalPadding(int value);
        Section & withPadding(int value);

    private:
        [[nodiscard]] int getMinComponentWidth(Orientation orientation) const noexcept;
        [[nodiscard]] int getMinSectionWidth(Orientation orientation) const noexcept;
        [[nodiscard]] int computeComponentWidth(Orientation orientation, float pixelsPerRelativeUnit) const noexcept;
        [[nodiscard]] int computeSectionWidth(Orientation orientation, float pixelsPerRelativeUnit) const noexcept;
        [[nodiscard]] int getMinComponentHeight(Orientation orientation) const noexcept;
        [[nodiscard]] int getMinSectionHeight(Orientation orientation) const noexcept;
        [[nodiscard]] int computeComponentHeight(Orientation orientation, float pixelsPerRelativeUnit) const noexcept;
        [[nodiscard]] int computeSectionHeight(Orientation orientation, float pixelsPerRelativeUnit) const noexcept;
    };

private:
    //==============================================================================
    Orientation mOrientation{};
    juce::Array<Section> mSections{};
    juce::Viewport mViewport{};
    bool mIsHorizontalScrollable{};
    bool mIsVerticalScrollable{};

public:
    //==============================================================================
    explicit LayoutComponent(Orientation orientation,
                             bool isHorizontalScrollable,
                             bool isVerticalScrollable,
                             GrisLookAndFeel & lookAndFeel) noexcept;
    ~LayoutComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(LayoutComponent)
    //==============================================================================
    Section & addSection(juce::Component * component) noexcept;
    Section & addSection(juce::Component & component) noexcept;
    Section & addSection(MinSizedComponent * component) noexcept;
    Section & addSection(MinSizedComponent & component) noexcept;
    Section & addSection(std::nullptr_t) noexcept;
    void clearSections();
    //==============================================================================
    void resized() override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    [[nodiscard]] int getMinInnerWidth() const noexcept;
    [[nodiscard]] int getMinInnerHeight() const noexcept;
    //==============================================================================
    JUCE_LEAK_DETECTOR(LayoutComponent)
};

} // namespace gris
