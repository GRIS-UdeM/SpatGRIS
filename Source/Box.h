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

class GrisLookAndFeel;
class AbstractVuMeterComponent;
class MainContentComponent;

//==============================================================================
class Box final : public juce::Component
{
    GrisLookAndFeel & mLookAndFeel;

    juce::Component mContent;
    juce::Viewport mViewport;
    juce::String mTitle;

public:
    //==============================================================================
    explicit Box(GrisLookAndFeel & feel,
                 juce::String const & title = "",
                 bool verticalScrollbar = false,
                 bool horizontalScrollbar = true);
    ~Box() override { this->mContent.deleteAllChildren(); }

    Box(Box const &) = delete;
    Box(Box &&) = delete;

    Box & operator=(Box const &) = delete;
    Box & operator=(Box &&) = delete;
    //==============================================================================
    [[nodiscard]] juce::Component * getContent() { return &this->mContent; }
    [[nodiscard]] juce::Component const * getContent() const { return &this->mContent; }
    [[nodiscard]] juce::Viewport * getViewport() { return &mViewport; }

    void resized() override { this->mViewport.setSize(this->getWidth(), this->getHeight()); }
    void correctSize(int width, int height);
    void paint(juce::Graphics & g) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Box)
};

//==============================================================================
class MainUiSection final : public MinSizedComponent
{
    static constexpr auto TITLE_HEIGHT = 18;

    juce::String mTitle{};
    MinSizedComponent * mContentComponent;
    GrisLookAndFeel & mLookAndFeel;

public:
    //==============================================================================
    MainUiSection(juce::String title, MinSizedComponent * contentComponent, GrisLookAndFeel & lookAndFeel);
    ~MainUiSection() override = default;
    //==============================================================================
    MainUiSection(MainUiSection const &) = delete;
    MainUiSection(MainUiSection &&) = delete;
    MainUiSection & operator=(MainUiSection const &) = delete;
    MainUiSection & operator=(MainUiSection &&) = delete;
    //==============================================================================
    void resized() override;
    void paint(juce::Graphics & g) override;

    [[nodiscard]] int getMinWidth() const noexcept override { return mContentComponent->getMinWidth(); }
    [[nodiscard]] int getMinHeight() const noexcept override
    {
        return mContentComponent->getMinHeight() + TITLE_HEIGHT;
    }

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(MainUiSection)
};

//==============================================================================
class SpatSlider final
    : public MinSizedComponent
    , juce::Slider::Listener
{
public:
    class Listener
    {
    public:
        virtual void sliderMoved(float value, SpatSlider * slider) = 0;
    };

private:
    //==============================================================================
    Listener & mListener;
    juce::Slider mSlider{};
    int mMinSize{};
    int mMaxSize{};

public:
    //==============================================================================
    SpatSlider(float minValue,
               float maxValue,
               float step,
               juce::String const & suffix,
               int minSize,
               int maxSize,
               Listener & listener);
    ~SpatSlider() override = default;
    //==============================================================================
    SpatSlider(SpatSlider const &) = delete;
    SpatSlider(SpatSlider &&) = delete;
    SpatSlider & operator=(SpatSlider const &) = delete;
    SpatSlider & operator=(SpatSlider &&) = delete;
    //==============================================================================
    void setValue(float value);
    //==============================================================================
    void resized() override;
    void sliderValueChanged(juce::Slider * slider) override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatSlider)
};

//==============================================================================
class SpatTextEditor
    : public MinSizedComponent
    , public juce::TextEditor::Listener
{
public:
    class Listener
    {
    public:
        virtual void textEditorChanged(juce::String const & value, SpatTextEditor * editor) = 0;
    };

private:
    Listener & mListener;
    int mMinSize;
    int mMaxSize;
    juce::TextEditor mEditor{};

public:
    SpatTextEditor(juce::String const & tooltip, int minSize, int maxSize, Listener & listener);
    ~SpatTextEditor() override = default;
    SpatTextEditor(SpatTextEditor const &) = delete;
    SpatTextEditor(SpatTextEditor &&) = delete;
    SpatTextEditor & operator=(SpatTextEditor const &) = delete;
    SpatTextEditor & operator=(SpatTextEditor &&) = delete;

    void setText(juce::String const & text);
    void textEditorFocusLost(juce::TextEditor & editor) override;
    void resized() override;

private:
    JUCE_LEAK_DETECTOR(SpatTextEditor)
};