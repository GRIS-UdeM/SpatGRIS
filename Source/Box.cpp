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

#include "Box.h"

#include "GrisLookAndFeel.h"

//==============================================================================
Box::Box(GrisLookAndFeel & feel,
         juce::String const & title,
         bool const verticalScrollbar,
         bool const horizontalScrollbar)
    : mLookAndFeel(feel)
    , mTitle(title)
{
    this->mViewport.setViewedComponent(&this->mContent, false);
    this->mViewport.setScrollBarsShown(verticalScrollbar, horizontalScrollbar);
    this->mViewport.setScrollBarThickness(15);
    this->mViewport.getVerticalScrollBar().setColour(juce::ScrollBar::ColourIds::thumbColourId,
                                                     feel.getScrollBarColour());
    this->mViewport.getHorizontalScrollBar().setColour(juce::ScrollBar::ColourIds::thumbColourId,
                                                       feel.getScrollBarColour());

    this->mViewport.setLookAndFeel(&feel);
    addAndMakeVisible(this->mViewport);
}

//==============================================================================
void Box::correctSize(int width, int const height)
{
    if (this->mTitle != "") {
        this->mViewport.setTopLeftPosition(0, 20);
        this->mViewport.setSize(getWidth(), getHeight() - 20);
        if (width < 80) {
            width = 80;
        }
    } else {
        this->mViewport.setTopLeftPosition(0, 0);
    }
    this->getContent()->setSize(width, height);
}

//==============================================================================
void Box::paint(juce::Graphics & g)
{
    g.setColour(this->mLookAndFeel.getBackgroundColour());
    g.fillRect(getLocalBounds());
    if (this->mTitle != "") {
        g.setColour(this->mLookAndFeel.getWinBackgroundColour());
        g.fillRect(0, 0, getWidth(), 18);
        g.setColour(this->mLookAndFeel.getFontColour());
        g.drawText(mTitle, 0, 0, this->mContent.getWidth(), 20, juce::Justification::left);
    }
}

//==============================================================================
MainUiSection::MainUiSection(juce::String title, MinSizedComponent * contentComponent, GrisLookAndFeel & lookAndFeel)
    : mTitle(std::move(title))
    , mContentComponent(contentComponent)
    , mLookAndFeel(lookAndFeel)
{
    addAndMakeVisible(mContentComponent);
}

//==============================================================================
void MainUiSection::resized()
{
    auto const availableHeight{ std::max(getHeight() - TITLE_HEIGHT, 0) };
    juce::Rectangle<int> const contentBounds{ 0, TITLE_HEIGHT, getWidth(), availableHeight };
    mContentComponent->setBounds(contentBounds);
}

//==============================================================================
void MainUiSection::paint(juce::Graphics & g)
{
    g.setColour(mLookAndFeel.getBackgroundColour());
    g.fillRect(getLocalBounds());
    if (mTitle != "") {
        g.setColour(mLookAndFeel.getWinBackgroundColour());
        g.fillRect(0, 0, getWidth(), TITLE_HEIGHT);
        g.setColour(mLookAndFeel.getFontColour());
        g.drawText(mTitle, 0, 0, getWidth(), TITLE_HEIGHT + 2, juce::Justification::left);
    }
}

//==============================================================================
SpatSlider::SpatSlider(float const minValue,
                       float const maxValue,
                       float const step,
                       juce::String const & suffix,
                       int const minSize,
                       int const maxSize,
                       Listener & listener)
    : mListener(listener)
    , mSlider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , mMinSize(minSize)
    , mMaxSize(maxSize)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSlider.setRange(minValue, maxValue, step);
    mSlider.setTextValueSuffix(suffix);
    mSlider.addListener(this);
    addAndMakeVisible(mSlider);
}

//==============================================================================
void SpatSlider::setValue(float const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSlider.setValue(value, juce::dontSendNotification);
}

//==============================================================================
void SpatSlider::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const extraWidth{ std::max(getWidth() - mMinSize, 0) };
    auto const extraHeight{ std::max(getHeight() - mMinSize, 0) };

    auto const width{ std::min(mMinSize + extraWidth, mMaxSize) };
    auto const height{ std::min(mMinSize + extraHeight, mMaxSize) };

    mSlider.setBounds(extraWidth / 2, extraHeight / 2, width, height);
}

//==============================================================================
void SpatSlider::sliderValueChanged(juce::Slider * slider)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(slider == &mSlider);
    mListener.sliderMoved(static_cast<float>(slider->getValue()), this);
}

//==============================================================================
int SpatSlider::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return mMinSize;
}

//==============================================================================
int SpatSlider::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return mMinSize;
}

//==============================================================================
SpatTextEditor::SpatTextEditor(juce::String const & tooltip, int const minSize, int const maxSize, Listener & listener)
    : mListener(listener)
    , mMinSize(minSize)
    , mMaxSize(maxSize)
{
    mEditor.setTooltip(tooltip);
    addAndMakeVisible(mEditor);
}
