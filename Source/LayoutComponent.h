#pragma once

#include "lib/tl/optional.hpp"

#include "MinSizedComponent.hpp"
#include "narrow.hpp"

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
                jassert(pixelsPerRelativeUnit >= 0.0f);
                return std::max(narrow<int>(std::round(pixelsPerRelativeUnit * mRelativeSize)), minComponentWidth);
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
                return std::max(narrow<int>(std::round(pixelsPerRelativeUnit * mRelativeSize)), minComponentHeight);
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
    void clear();
    void paint(juce::Graphics & g) override;
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