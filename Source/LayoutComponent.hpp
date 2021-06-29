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

#pragma once

#include "MinSizedComponent.hpp"
#include "Narrow.hpp"

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
        MinSizedComponent * mComponent{};
        Mode mMode{ Mode::undefined };
        int mFixedSize{};
        float mRelativeSize{};
        int mTopPadding{};
        int mLeftPadding{};
        int mBottomPadding{};
        int mRightPadding{};

    public:
        //==============================================================================

        Section & withFixedSize(int const value)
        {
            jassert(mMode == Mode::undefined);
            jassert(value >= 0);
            mFixedSize = value;
            mMode = Mode::fixed;
            return *this;
        }
        Section & withRelativeSize(float const value)
        {
            jassert(mMode == Mode::undefined);
            jassert(value > 0.0f);
            mRelativeSize = value;
            mMode = Mode::relative;
            return *this;
        }
        Section & withChildMinSize()
        {
            jassert(mMode == Mode::undefined);
            jassert(mComponent != nullptr);
            mMode = Mode::childMinSize;
            return *this;
        }
        Section & withTopPadding(int const value)
        {
            jassert(mTopPadding == 0);
            mTopPadding = value;
            return *this;
        }
        Section & withLeftPadding(int const value)
        {
            jassert(mLeftPadding == 0);
            mLeftPadding = value;
            return *this;
        }
        Section & withBottomPadding(int const value)
        {
            jassert(mBottomPadding == 0);
            mBottomPadding = value;
            return *this;
        }
        Section & withRightPadding(int const value)
        {
            jassert(mRightPadding == 0);
            mRightPadding = value;
            return *this;
        }
        Section & withHorizontalPadding(int const value)
        {
            jassert(mLeftPadding == 0 && mRightPadding == 0);
            mLeftPadding = value;
            mRightPadding = value;
            return *this;
        }
        Section & withVerticalPadding(int const value)
        {
            jassert(mTopPadding == 0 && mBottomPadding == 0);
            mBottomPadding = value;
            mTopPadding = value;
            return *this;
        }
        Section & widthPadding(int const value)
        {
            jassert(mLeftPadding == 0 && mRightPadding == 0 && mTopPadding == 0 && mBottomPadding == 0);
            mBottomPadding = value;
            mTopPadding = value;
            mLeftPadding = value;
            mRightPadding = value;
            return *this;
        }

    private:
        [[nodiscard]] int getMinComponentWidth(Orientation const orientation) const noexcept
        {
            if (orientation == Orientation::horizontal && mMode == Mode::fixed) {
                return mFixedSize;
            }
            if (!mComponent) {
                return 0;
            }
            return mComponent->getMinWidth();
        }
        [[nodiscard]] int getMinSectionWidth(Orientation const orientation) const noexcept
        {
            return mLeftPadding + getMinComponentWidth(orientation) + mRightPadding;
        }
        [[nodiscard]] int computeComponentWidth(Orientation const orientation,
                                                float const pixelsPerRelativeUnit) const noexcept
        {
            auto const minComponentWidth{ getMinComponentWidth(orientation) };
            switch (mMode) {
            case Mode::childMinSize:
            case Mode::fixed:
                return minComponentWidth;
            case Mode::relative:
                return minComponentWidth + narrow<int>(std::round(pixelsPerRelativeUnit * mRelativeSize));
            case Mode::undefined:
            default:
                break;
            }
            jassertfalse;
            return 0;
        }
        [[nodiscard]] int computeSectionWidth(Orientation const orientation,
                                              float const pixelsPerRelativeUnit) const noexcept
        {
            return mLeftPadding + computeComponentWidth(orientation, pixelsPerRelativeUnit) + mRightPadding;
        }
        [[nodiscard]] int getMinComponentHeight(Orientation const orientation) const noexcept
        {
            if (orientation == Orientation::vertical && mMode == Mode::fixed) {
                return mFixedSize;
            }
            if (!mComponent) {
                return 0;
            }
            return mComponent->getMinHeight();
        }
        [[nodiscard]] int getMinSectionHeight(Orientation const orientation) const noexcept
        {
            return mTopPadding + getMinComponentHeight(orientation) + mBottomPadding;
        }
        [[nodiscard]] int computeComponentHeight(Orientation const orientation,
                                                 float const pixelsPerRelativeUnit) const noexcept
        {
            auto const minComponentHeight{ getMinComponentHeight(orientation) };
            switch (mMode) {
            case Mode::childMinSize:
            case Mode::fixed:
                return minComponentHeight;
            case Mode::relative:
                return minComponentHeight + narrow<int>(std::round(pixelsPerRelativeUnit * mRelativeSize));
            case Mode::undefined:
            default:
                break;
            }
            jassertfalse;
            return 0;
        }
        [[nodiscard]] int computeSectionHeight(Orientation const orientation,
                                               float const pixelsPerRelativeUnit) const noexcept
        {
            return mTopPadding + computeComponentHeight(orientation, pixelsPerRelativeUnit) + mBottomPadding;
        }
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
    //==============================================================================
    LayoutComponent(LayoutComponent const &) = delete;
    LayoutComponent(LayoutComponent &&) = delete;
    LayoutComponent & operator=(LayoutComponent const &) = delete;
    LayoutComponent & operator=(LayoutComponent &&) = delete;
    //==============================================================================
    Section & addSection(MinSizedComponent * component) noexcept;
    Section & addSection(MinSizedComponent & component) noexcept { return addSection(&component); }
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