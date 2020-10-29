/*
 This file is part of SpatGRIS2.

 Developers: Olivier Belanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "BinaryData.h"

//==============================================================================
/** Custom Look And Feel */
class GrisLookAndFeel : public LookAndFeel_V3
{
    float mFontSize;

    juce::Font mFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      static_cast<size_t>(BinaryData::SinkinSans400Regular_otfSize)));
    juce::Font mBigFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      static_cast<size_t>(BinaryData::SinkinSans400Regular_otfSize)));
    juce::Font mBiggerFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      static_cast<size_t>(BinaryData::SinkinSans400Regular_otfSize)));

    juce::Colour mBackGroundAndFieldColour;
    juce::Colour mWinBackGroundAndFieldColour;
    juce::Colour mLightColour;
    juce::Colour mDarkColour;
    juce::Colour mGreyColour;
    juce::Colour mEditBgcolor;
    juce::Colour mHlBgcolor;
    juce::Colour mOnColor;
    juce::Colour mOnColorOver;
    juce::Colour mOnColorDown;
    juce::Colour mOffColor;
    juce::Colour mGreenColor;
    juce::Colour mRedColor;

public:
    //==============================================================================
    juce::Font mSmallFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      static_cast<size_t>(BinaryData::SinkinSans400Regular_otfSize)));
    juce::Font mSmallerFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      static_cast<size_t>(BinaryData::SinkinSans400Regular_otfSize)));
    //==============================================================================
    GrisLookAndFeel();
    ~GrisLookAndFeel() override = default;

    GrisLookAndFeel(GrisLookAndFeel const &) = delete;
    GrisLookAndFeel(GrisLookAndFeel &&) = delete;

    GrisLookAndFeel & operator=(GrisLookAndFeel const &) = delete;
    GrisLookAndFeel & operator=(GrisLookAndFeel &&) = delete;
    //==============================================================================
    juce::Font getLabelFont(Label & label) override;
    juce::Font getFont() const { return this->mFont; }
    juce::Font getComboBoxFont(ComboBox & /*comboBox*/) override { return this->mFont; }
    juce::Font getTextButtonFont(TextButton &, int /*buttonHeight*/) override { return this->mFont; }
    juce::Font getMenuBarFont(MenuBarComponent &, int /*itemIndex*/, const String & /*itemText*/) override
    {
        return this->mFont;
    }

    juce::Colour getWinBackgroundColour() const { return this->mWinBackGroundAndFieldColour; }
    juce::Colour getBackgroundColour() const { return this->mBackGroundAndFieldColour; }
    juce::Colour getFieldColour() const { return this->mBackGroundAndFieldColour; }
    juce::Colour getFontColour() const { return this->mLightColour; }
    juce::Colour getScrollBarColour() const { return this->mGreyColour; }
    juce::Colour getDarkColour() const { return this->mDarkColour; }
    juce::Colour getLightColour() const { return this->mLightColour; }
    juce::Colour getEditBackgroundColour() const { return this->mEditBgcolor; }
    juce::Colour getHighlightColour() const { return this->mHlBgcolor; }
    juce::Colour getOnColour() const { return this->mOnColor; }
    juce::Colour getOffColour() const { return this->mOffColor; }
    juce::Colour getGreenColour() const { return this->mGreenColor; }
    juce::Colour getRedColour() const { return this->mRedColor; }

    void drawComboBox(Graphics & g,
                      int width,
                      int height,
                      bool isButtonDown,
                      int buttonX,
                      int buttonY,
                      int buttonW,
                      int buttonH,
                      ComboBox & box) override;
    void drawRoundThumb(Graphics & g,
                        float x,
                        float y,
                        float diameter,
                        juce::Colour const & colour,
                        float outlineThickness) const;
    void drawButtonBackground(Graphics & g,
                              Button & button,
                              const juce::Colour & backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override;
    void drawTickBox(Graphics & g,
                     Component & component,
                     float x,
                     float y,
                     float w,
                     float h,
                     bool ticked,
                     bool isEnabled,
                     bool isMouseOverButton,
                     bool isButtonDown) override;
    void drawLinearSliderThumb(Graphics & g,
                               int x,
                               int y,
                               int width,
                               int height,
                               float sliderPos,
                               float minSliderPos,
                               float maxSliderPos,
                               const Slider::SliderStyle style,
                               Slider & slider) override;
    void drawLinearSlider(Graphics & g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float minSliderPos,
                          float maxSliderPos,
                          const Slider::SliderStyle style,
                          Slider & slider) override;
    void drawLinearSliderBackground(Graphics & g,
                                    int x,
                                    int y,
                                    int width,
                                    int height,
                                    float /*sliderPos*/,
                                    float /*minSliderPos*/,
                                    float /*maxSliderPos*/,
                                    Slider::SliderStyle /*style*/,
                                    Slider & slider) override;
    void fillTextEditorBackground(Graphics & g, int width, int height, TextEditor & t) override;
    void drawTextEditorOutline(Graphics & g, int width, int height, TextEditor & t) override;
    void drawToggleButton(Graphics & g, ToggleButton & button, bool isMouseOverButton, bool isButtonDown) override;
    void drawTabButton(TabBarButton & button, Graphics & g, bool isMouseOver, bool isMouseDown) override;
    void createTabTextLayout(const TabBarButton & button,
                             float length,
                             float depth,
                             juce::Colour colour,
                             TextLayout & textLayout) const;
    void drawRotarySlider(Graphics & g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          Slider & slider) override;

private:
    JUCE_LEAK_DETECTOR(GrisLookAndFeel)
}; // class GrisLookAndFeel

//==============================================================================
class SmallGrisLookAndFeel final : public GrisLookAndFeel
{
public:
    //==============================================================================
    SmallGrisLookAndFeel() = default;
    ~SmallGrisLookAndFeel() override = default;

    SmallGrisLookAndFeel(SmallGrisLookAndFeel const &) = delete;
    SmallGrisLookAndFeel(SmallGrisLookAndFeel &&) = delete;

    SmallGrisLookAndFeel & operator=(SmallGrisLookAndFeel const &) = delete;
    SmallGrisLookAndFeel & operator=(SmallGrisLookAndFeel &&) = delete;
    //==============================================================================
    juce::Font getTextButtonFont(TextButton &, int /*buttonHeight*/) override { return this->mSmallerFont; }

    void drawToggleButton(Graphics & g, ToggleButton & button, bool isMouseOverButton, bool isButtonDown) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SmallGrisLookAndFeel)
}; // class SmallGrisLookAndFeel
