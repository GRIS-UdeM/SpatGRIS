#pragma once

#include "lib/tl/optional.hpp"

#include "MinSizedComponent.hpp"
#include "narrow.hpp"

class GrisLookAndFeel;

//==============================================================================
class LayoutComponent final : public MinSizedComponent
{
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
        [[nodiscard]] int getMinComponentSize(Orientation const orientation) const noexcept
        {
            if (!mComponent) {
                return 0;
            }
            return orientation == Orientation::horizontal ? mComponent->getMinWidth() : mComponent->getMinHeight();
        }
        [[nodiscard]] int getMinSize(Orientation const orientation) const noexcept
        {
            auto const padding{ orientation == Orientation::horizontal ? mLeftPadding + mRightPadding
                                                                       : mTopPadding + mBottomPadding };
            auto const minComponentSize{ getMinComponentSize(orientation) };
            switch (mMode) {
            case Mode::childMinSize:
                return minComponentSize + padding;
            case Mode::fixed:
                return std::max(mFixedSize, minComponentSize) + padding;
            case Mode::relative:
                return minComponentSize + padding;
            case Mode::undefined:
            default:
                jassertfalse;
                return 0;
            }
        }
        [[nodiscard]] int getInnerSize(Orientation const orientation, float const pixelPerRelativeUnit) const noexcept
        {
            auto const minComponentSize{ getMinComponentSize(orientation) };
            switch (mMode) {
            case Mode::childMinSize:
                return minComponentSize;
            case Mode::fixed:
                return std::max(mFixedSize, minComponentSize);
            case Mode::relative:
                jassert(pixelPerRelativeUnit >= 0.0f);
                return std::max(narrow<int>(std::round(pixelPerRelativeUnit * mRelativeSize)), minComponentSize);
            case Mode::undefined:
            default:
                jassertfalse;
                return 0;
            }
        }
    };

private:
    //==============================================================================
    Orientation mOrientation{};
    juce::Array<Section> mSections{};
    juce::Viewport mViewport{};

public:
    //==============================================================================
    explicit LayoutComponent(Orientation orientation, GrisLookAndFeel & lookAndFeel) noexcept;
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
};