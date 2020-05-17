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

#ifndef GRISLOOKANDFEEL_H
#define GRISLOOKANDFEEL_H

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/** Custom Look And Feel subclasss. */
class GrisLookAndFeel : public LookAndFeel_V3
{
private:
    float fontSize;

    juce::Font font
        = juce::Font(juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                                   (size_t)BinaryData::SinkinSans400Regular_otfSize));
    juce::Font bigFont
        = juce::Font(juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                                   (size_t)BinaryData::SinkinSans400Regular_otfSize));
    juce::Font biggerFont
        = juce::Font(juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                                   (size_t)BinaryData::SinkinSans400Regular_otfSize));

    juce::Colour backGroundAndFieldColour;
    juce::Colour winBackGroundAndFieldColour;
    juce::Colour lightColour;
    juce::Colour darkColour;
    juce::Colour greyColour;
    juce::Colour editBgcolor;
    juce::Colour hlBgcolor;
    juce::Colour onColor;
    juce::Colour onColorOver;
    juce::Colour onColorDown;
    juce::Colour offColor;
    juce::Colour greenColor;
    juce::Colour redColor;

public:
    //==============================================================================
    juce::Font smallFont
        = juce::Font(juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                                   (size_t)BinaryData::SinkinSans400Regular_otfSize));
    juce::Font smallerFont
        = juce::Font(juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf,
                                                                   (size_t)BinaryData::SinkinSans400Regular_otfSize));
    //==============================================================================
    GrisLookAndFeel();
    ~GrisLookAndFeel() override = default;
    //==============================================================================
    juce::Font getLabelFont(Label & label) final;
    juce::Font getFont() const { return this->font; }
    juce::Font getComboBoxFont(ComboBox & comboBox) final { return this->font; }
    juce::Font getTextButtonFont(TextButton &, int buttonHeight) override { return this->font; }
    juce::Font getMenuBarFont(MenuBarComponent &, int itemIndex, const String & itemText) final { return this->font; }

    juce::Colour getWinBackgroundColour() const { return this->winBackGroundAndFieldColour; }
    juce::Colour getBackgroundColour() const { return this->backGroundAndFieldColour; }
    juce::Colour getFieldColour() const { return this->backGroundAndFieldColour; }
    juce::Colour getFontColour() const { return this->lightColour; }
    juce::Colour getScrollBarColour() const { return this->greyColour; }
    juce::Colour getDarkColour() const { return this->darkColour; }
    juce::Colour getLightColour() const { return this->lightColour; }
    juce::Colour getEditBackgroundColour() const { return this->editBgcolor; }
    juce::Colour getHighlightColour() const { return this->hlBgcolor; }
    juce::Colour getOnColour() const { return this->onColor; }
    juce::Colour getOffColour() const { return this->offColor; }
    juce::Colour getGreenColour() const { return this->greenColor; }
    juce::Colour getRedColour() const { return this->redColor; }

    void drawComboBox(Graphics & g,
                      int        width,
                      int        height,
                      bool       isButtonDown,
                      int        buttonX,
                      int        buttonY,
                      int        buttonW,
                      int        buttonH,
                      ComboBox & box) final;
    void drawRoundThumb(Graphics &           g,
                        float const          x,
                        float const          y,
                        float const          diameter,
                        juce::Colour const & colour,
                        float const          outlineThickness) const;
    void drawButtonBackground(Graphics &           g,
                              Button &             button,
                              const juce::Colour & backgroundColour,
                              bool                 isMouseOverButton,
                              bool                 isButtonDown) final;
    void drawTickBox(Graphics &  g,
                     Component & component,
                     float       x,
                     float       y,
                     float       w,
                     float       h,
                     bool        ticked,
                     bool        isEnabled,
                     bool        isMouseOverButton,
                     bool        isButtonDown) final;
    void drawLinearSliderThumb(Graphics &                g,
                               int                       x,
                               int                       y,
                               int                       width,
                               int                       height,
                               float                     sliderPos,
                               float                     minSliderPos,
                               float                     maxSliderPos,
                               const Slider::SliderStyle style,
                               Slider &                  slider) final;
    void drawLinearSlider(Graphics &                g,
                          int                       x,
                          int                       y,
                          int                       width,
                          int                       height,
                          float                     sliderPos,
                          float                     minSliderPos,
                          float                     maxSliderPos,
                          const Slider::SliderStyle style,
                          Slider &                  slider) final;
    void drawLinearSliderBackground(Graphics & g,
                                    int        x,
                                    int        y,
                                    int        width,
                                    int        height,
                                    float /*sliderPos*/,
                                    float /*minSliderPos*/,
                                    float /*maxSliderPos*/,
                                    const Slider::SliderStyle /*style*/,
                                    Slider & slider) final;
    void fillTextEditorBackground(Graphics & g, int width, int height, TextEditor & t) final;
    void drawTextEditorOutline(Graphics & g, int width, int height, TextEditor & t) final;
    void drawToggleButton(Graphics & g, ToggleButton & button, bool isMouseOverButton, bool isButtonDown) override;
    void drawTabButton(TabBarButton & button, Graphics & g, bool isMouseOver, bool isMouseDown) final;
    void createTabTextLayout(const TabBarButton & button,
                             float                length,
                             float                depth,
                             juce::Colour const   colour,
                             TextLayout &         textLayout) const;
    void drawRotarySlider(Graphics & g,
                          int        x,
                          int        y,
                          int        width,
                          int        height,
                          float      sliderPos,
                          float      rotaryStartAngle,
                          float      rotaryEndAngle,
                          Slider &   slider) final;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrisLookAndFeel);
};

//==============================================================================
class SmallGrisLookAndFeel final : public GrisLookAndFeel
{
public:
    SmallGrisLookAndFeel() = default;
    ~SmallGrisLookAndFeel() final = default;
    //==============================================================================
    juce::Font getTextButtonFont(TextButton &, int buttonHeight) final { return this->smallerFont; }

    void drawToggleButton(Graphics & g, ToggleButton & button, bool isMouseOverButton, bool isButtonDown) final;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmallGrisLookAndFeel);
};

#endif // GRISLOOKANDFEEL_H
