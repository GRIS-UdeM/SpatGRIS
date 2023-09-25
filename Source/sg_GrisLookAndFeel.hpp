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

#include "sg_Macros.hpp"
#include "sg_Narrow.hpp"

#include "BinaryData.h"

namespace gris
{
//==============================================================================
/** Custom Look And Feel */
class GrisLookAndFeel : public juce::LookAndFeel_V3
{
    float mFontSize;

    juce::Font mFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      narrow<size_t>(BinaryData::SinkinSans400Regular_otfSize)));
    juce::Font mBigFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      narrow<size_t>(BinaryData::SinkinSans400Regular_otfSize)));
    juce::Font mBiggerFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      narrow<size_t>(BinaryData::SinkinSans400Regular_otfSize)));
    juce::Font mMonoFont
        = juce::Font(juce::CustomTypeface::createSystemTypefaceFor(BinaryData::FreeFarsiMono_otf,
                                                                   narrow<size_t>(BinaryData::FreeFarsiMono_otfSize)));

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
    juce::Colour mOutlineOffColor;
    juce::Colour mGreenColor;
    juce::Colour mRedColor;
    juce::Colour mSourceColor;
    juce::Colour mSubColor;
    juce::Colour mInactiveColor;

public:
    //==============================================================================
    juce::Font mSmallFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      narrow<size_t>(BinaryData::SinkinSans400Regular_otfSize)));
    juce::Font mSmallerFont = juce::Font(
        juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                      narrow<size_t>(BinaryData::SinkinSans400Regular_otfSize)));
    //==============================================================================
    GrisLookAndFeel();
    ~GrisLookAndFeel() override = default;
    SG_DELETE_COPY_AND_MOVE(GrisLookAndFeel)
    //==============================================================================
    [[nodiscard]] juce::Font getLabelFont(juce::Label & label) override;
    [[nodiscard]] juce::Font getFont() const { return this->mFont; }
    [[nodiscard]] juce::Font getComboBoxFont(juce::ComboBox & /*comboBox*/) override { return this->mFont; }
    [[nodiscard]] juce::Font getTextButtonFont(juce::TextButton &, int /*buttonHeight*/) override
    {
        return this->mFont;
    }
    [[nodiscard]] juce::Font
        getMenuBarFont(juce::MenuBarComponent &, int /*itemIndex*/, const juce::String & /*itemText*/) override
    {
        return this->mFont;
    }

    [[nodiscard]] juce::Colour getWinBackgroundColour() const { return this->mWinBackGroundAndFieldColour; }
    [[nodiscard]] juce::Colour getBackgroundColour() const { return this->mBackGroundAndFieldColour; }
    [[nodiscard]] juce::Colour getFieldColour() const { return this->mBackGroundAndFieldColour; }
    [[nodiscard]] juce::Colour getFontColour() const { return this->mLightColour; }
    [[nodiscard]] juce::Colour getScrollBarColour() const { return this->mGreyColour; }
    [[nodiscard]] juce::Colour getDarkColour() const { return this->mDarkColour; }
    [[nodiscard]] juce::Colour getLightColour() const { return this->mLightColour; }
    [[nodiscard]] juce::Colour getEditBackgroundColour() const { return this->mEditBgcolor; }
    [[nodiscard]] juce::Colour getHighlightColour() const { return this->mHlBgcolor; }
    [[nodiscard]] juce::Colour getOnColour() const { return this->mOnColor; }
    [[nodiscard]] juce::Colour getOffColour() const { return this->mOffColor; }
    [[nodiscard]] juce::Colour getGreenColour() const { return this->mGreenColor; }
    [[nodiscard]] juce::Colour getRedColour() const { return this->mRedColor; }
    [[nodiscard]] juce::Colour getSourceColor() const { return this->mSourceColor; }
    [[nodiscard]] juce::Colour getSubColor() const { return this->mSubColor; }
    [[nodiscard]] juce::Colour getInactiveColor() const { return this->mInactiveColor; }

    void drawComboBox(juce::Graphics & g,
                      int width,
                      int height,
                      bool isButtonDown,
                      int buttonXF,
                      int buttonYF,
                      int buttonWF,
                      int buttonHF,
                      juce::ComboBox & box) override;
    void drawRoundThumb(juce::Graphics & g,
                        float x,
                        float y,
                        float diameter,
                        juce::Colour const & colour,
                        float outlineThickness) const;
    void drawButtonBackground(juce::Graphics & g,
                              juce::Button & button,
                              const juce::Colour & backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override;
    void drawTickBox(juce::Graphics & g,
                     juce::Component & component,
                     float x,
                     float y,
                     float w,
                     float h,
                     bool ticked,
                     bool isEnabled,
                     bool isMouseOverButton,
                     bool isButtonDown) override;
    void drawLinearSliderThumb(juce::Graphics & g,
                               int x,
                               int y,
                               int width,
                               int height,
                               float sliderPos,
                               float minSliderPos,
                               float maxSliderPos,
                               juce::Slider::SliderStyle style,
                               juce::Slider & slider) override;
    void drawLinearSlider(juce::Graphics & g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float minSliderPos,
                          float maxSliderPos,
                          juce::Slider::SliderStyle style,
                          juce::Slider & slider) override;
    void drawLinearSliderBackground(juce::Graphics & g,
                                    int x,
                                    int y,
                                    int width,
                                    int height,
                                    float /*sliderPos*/,
                                    float /*minSliderPos*/,
                                    float /*maxSliderPos*/,
                                    juce::Slider::SliderStyle /*style*/,
                                    juce::Slider & slider) override;
    void fillTextEditorBackground(juce::Graphics & g, int width, int height, juce::TextEditor & t) override;
    void drawTextEditorOutline(juce::Graphics & g, int width, int height, juce::TextEditor & t) override;
    void drawToggleButton(juce::Graphics & g,
                          juce::ToggleButton & button,
                          bool isMouseOverButton,
                          bool isButtonDown) override;
    void drawTabButton(juce::TabBarButton & button, juce::Graphics & g, bool isMouseOver, bool isMouseDown) override;
    void createTabTextLayout(const juce::TabBarButton & button,
                             float length,
                             float depth,
                             juce::Colour colour,
                             juce::TextLayout & textLayout) const;
    void drawRotarySlider(juce::Graphics & g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider & slider) override;

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
    [[nodiscard]] juce::Font getTextButtonFont(juce::TextButton &, int /*buttonHeight*/) override
    {
        return this->mSmallerFont;
    }

    void drawToggleButton(juce::Graphics & g,
                          juce::ToggleButton & button,
                          bool isMouseOverButton,
                          bool isButtonDown) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SmallGrisLookAndFeel)
}; // class SmallGrisLookAndFeel

} // namespace gris
