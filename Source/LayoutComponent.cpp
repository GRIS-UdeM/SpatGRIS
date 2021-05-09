#include "LayoutComponent.h"

#include "GrisLookAndFeel.h"

static auto const MAX_ELEM = [](auto const a, auto const b) {
    static_assert(std::is_same_v<decltype(a), decltype(b)>);
    return a < b ? b : a;
};

//==============================================================================
LayoutComponent::LayoutComponent(Orientation const orientation, GrisLookAndFeel & lookAndFeel) noexcept
    : mOrientation(orientation)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mViewport.setScrollBarsShown(true, true);
    mViewport.setScrollBarThickness(15);
    mViewport.getHorizontalScrollBar().setColour(juce::ScrollBar::ColourIds::thumbColourId,
                                                 lookAndFeel.getScrollBarColour());
    mViewport.setViewedComponent(new juce::Component{}, true);
    addAndMakeVisible(mViewport);
}

//==============================================================================
LayoutComponent::Section & LayoutComponent::addSection(MinSizedComponent * component) noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSections.add(Section{});
    auto & section{ mSections.getReference(mSections.size() - 1) };
    section.mComponent = component;
    mViewport.getViewedComponent()->addAndMakeVisible(component);
    return section;
}

//==============================================================================
void LayoutComponent::clear()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    removeAllChildren();
    mSections.clearQuick();
}

//==============================================================================
void LayoutComponent::paint(juce::Graphics & g)
{
    for (auto const & section : mSections) {
        if (!section.mComponent) {
            continue;
        }
        auto const bounds{ section.mComponent->getBounds() };
        g.setColour(juce::Colours::blue.withAlpha(0.5f));
        g.fillRect(bounds);
        g.setColour(juce::Colours::red.withAlpha(0.5f));
        g.fillRect(bounds.reduced(2));
    }
}

//==============================================================================
void LayoutComponent::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const minWidth{ getMinWidth() };
    auto const minHeight{ getMinHeight() };

    auto const availableWidth{ std::max(getWidth(), minWidth) };
    auto const availableHeight{ std::max(getHeight(), minHeight) };

    auto const constantDimension{ mOrientation == Orientation::horizontal ? availableHeight : availableWidth };
    auto const dimensionToSplit{ mOrientation == Orientation::horizontal ? availableWidth : availableHeight };

    auto const minSpace{ mOrientation == Orientation::horizontal ? minWidth : minHeight };
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
            auto const size{ section.getInnerSize(mOrientation, pixelsPerRelativeUnit) };
            if (section.mComponent) {
                section.mComponent->setBounds(offset + section.mLeftPadding,
                                              section.mTopPadding,
                                              size,
                                              constantDimension);
            }
            offset += section.mLeftPadding + size + section.mRightPadding;
        }
    } else {
        jassert(mOrientation == Orientation::vertical);
        int offset{};
        for (auto & section : mSections) {
            auto const size{ section.getInnerSize(mOrientation, pixelsPerRelativeUnit) };
            if (section.mComponent) {
                section.mComponent->setBounds(offset + section.mTopPadding,
                                              section.mLeftPadding,
                                              constantDimension,
                                              size);
            }
            offset += section.mTopPadding + size + section.mBottomPadding;
        }
    }

    mViewport.getViewedComponent()->setSize(availableWidth, availableHeight);
    mViewport.setSize(getWidth(), getHeight());
}

//==============================================================================
int LayoutComponent::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    return mOrientation == Orientation::horizontal
               ? 0
               : std::transform_reduce(mSections.begin(), mSections.end(), 0, MAX_ELEM, [](Section const & section) {
                     return (section.mComponent ? section.mComponent->getMinWidth() : 0) + section.mLeftPadding
                            + section.mRightPadding;
                 });
}

//==============================================================================
int LayoutComponent::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    return mOrientation == Orientation::vertical
               ? 0
               : std::transform_reduce(mSections.begin(), mSections.end(), 0, MAX_ELEM, [](Section const & section) {
                     return (section.mComponent ? section.mComponent->getMinHeight() : 0) + section.mTopPadding
                            + section.mBottomPadding;
                 });
}
