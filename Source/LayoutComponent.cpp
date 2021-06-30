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

#include "LayoutComponent.hpp"

#include "GrisLookAndFeel.hpp"

static auto const MAX_ELEM = [](auto const a, auto const b) {
    static_assert(std::is_same_v<decltype(a), decltype(b)>);
    return a < b ? b : a;
};

//==============================================================================
LayoutComponent::LayoutComponent(Orientation const orientation,
                                 bool const isHorizontalScrollable,
                                 bool const isVerticalScrollable,
                                 GrisLookAndFeel & lookAndFeel) noexcept
    : mOrientation(orientation)
    , mIsHorizontalScrollable(isHorizontalScrollable)
    , mIsVerticalScrollable(isVerticalScrollable)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mViewport.setScrollBarsShown(isVerticalScrollable, isHorizontalScrollable);
    mViewport.setScrollBarThickness(SCROLL_BAR_WIDTH);
    mViewport.getHorizontalScrollBar().setColour(juce::ScrollBar::ColourIds::thumbColourId,
                                                 lookAndFeel.getScrollBarColour());
    mViewport.getVerticalScrollBar().setColour(juce::ScrollBar::ColourIds::thumbColourId,
                                               lookAndFeel.getScrollBarColour());
    mViewport.setViewedComponent(new juce::Component{}, true);
    addAndMakeVisible(mViewport);
}

//==============================================================================
LayoutComponent::Section & LayoutComponent::addSection(juce::Component * component) noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSections.add(Section{});
    auto & section{ mSections.getReference(mSections.size() - 1) };
    section.mComponent = component;
    mViewport.getViewedComponent()->addAndMakeVisible(component);
    return section;
}

//==============================================================================
void LayoutComponent::clearSections()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mViewport.getViewedComponent()->removeAllChildren();
    mSections.clearQuick();
}

//==============================================================================
void LayoutComponent::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const outerWidth{ getWidth() };
    auto const outerHeight{ getHeight() };

    auto const minInnerWidth{ getMinInnerWidth() };
    auto const minInnerHeight{ getMinInnerHeight() };

    auto const correctForHorizontalScrollBar{ minInnerWidth > outerWidth && mIsHorizontalScrollable };
    auto const correctForVerticalScrollBar{ minInnerHeight > outerHeight && mIsVerticalScrollable };

    auto const innerWidth{ std::max(outerWidth, minInnerWidth) - (correctForVerticalScrollBar ? SCROLL_BAR_WIDTH : 0) };
    auto const innerHeight{ std::max(outerHeight, minInnerHeight)
                            - (correctForHorizontalScrollBar ? SCROLL_BAR_WIDTH : 0) };

    auto const constantDimension{ mOrientation == Orientation::horizontal ? innerHeight : innerWidth };
    auto const dimensionToSplit{ mOrientation == Orientation::horizontal ? innerWidth : innerHeight };

    auto const minSpace{ mOrientation == Orientation::horizontal ? minInnerWidth : minInnerHeight };
    auto const spaceToShare{ std::max(dimensionToSplit - minSpace, 0) };

    auto const totalRelativeUnits{ std::transform_reduce(
        mSections.begin(),
        mSections.end(),
        0.0f,
        std::plus(),
        [](Section const & section) { return section.mRelativeSize; }) };
    auto const pixelsPerRelativeUnit{ totalRelativeUnits == 0.0f ? 0.0f : spaceToShare / totalRelativeUnits };

    if (mOrientation == Orientation::horizontal) {
        int offset{};
        for (auto & section : mSections) {
            auto const componentWidth{ section.computeComponentWidth(mOrientation, pixelsPerRelativeUnit) };
            if (section.mComponent) {
                auto const x{ offset + section.mLeftPadding };
                auto const y{ section.mTopPadding };
                auto const height{ constantDimension - section.mBottomPadding - section.mTopPadding };
                section.mComponent->setBounds(x, y, componentWidth, height);
            }
            auto const sectionWidth{ componentWidth + section.mLeftPadding + section.mRightPadding };
            offset += sectionWidth;
        }
    } else {
        jassert(mOrientation == Orientation::vertical);
        int offset{};
        for (auto & section : mSections) {
            auto const size{ section.computeComponentHeight(mOrientation, pixelsPerRelativeUnit) };
            if (section.mComponent) {
                section.mComponent->setBounds(section.mLeftPadding,
                                              offset + section.mTopPadding,
                                              constantDimension - section.mRightPadding,
                                              size);
            }
            offset += section.mTopPadding + size + section.mBottomPadding;
        }
    }

    mViewport.getViewedComponent()->setSize(innerWidth, innerHeight);
    mViewport.setSize(outerWidth, outerHeight);
}

//==============================================================================
int LayoutComponent::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mIsHorizontalScrollable) {
        return 0;
    }
    return getMinInnerWidth() + (mIsVerticalScrollable ? SCROLL_BAR_WIDTH : 0);
}

//==============================================================================
int LayoutComponent::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mIsVerticalScrollable) {
        return 0;
    }
    return getMinInnerHeight() + (mIsHorizontalScrollable ? SCROLL_BAR_WIDTH : 0);
}

//==============================================================================
int LayoutComponent::getMinInnerWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mOrientation == Orientation::horizontal) {
        return std::transform_reduce(mSections.begin(), mSections.end(), 0, std::plus(), [=](Section const & section) {
            return section.getMinSectionWidth(mOrientation);
        });
    }
    jassert(mOrientation == Orientation::vertical);
    return std::transform_reduce(mSections.begin(), mSections.end(), 0, MAX_ELEM, [=](Section const & section) {
        return section.getMinSectionWidth(mOrientation);
    });
}

//==============================================================================
int LayoutComponent::getMinInnerHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mOrientation == Orientation::vertical) {
        return std::transform_reduce(mSections.begin(), mSections.end(), 0, std::plus(), [=](Section const & section) {
            return section.getMinSectionHeight(mOrientation);
        });
    }
    jassert(mOrientation == Orientation::horizontal);
    return std::transform_reduce(mSections.begin(), mSections.end(), 0, MAX_ELEM, [=](Section const & section) {
        return section.getMinSectionHeight(mOrientation);
    });
}
