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

#include "sg_GrisLookAndFeel.hpp"

//==============================================================================
GrisLookAndFeel::GrisLookAndFeel()
{
    this->mBackGroundAndFieldColour = juce::Colours::blue.withBrightness(0.35f).withSaturation(0.05f);
    this->mWinBackGroundAndFieldColour = juce::Colour::fromRGB(46, 46, 46);

    this->mLightColour = juce::Colour::fromRGB(235, 245, 250); // Colours::whiteSmoke;
    this->mDarkColour = juce::Colour::fromRGB(15, 10, 5);      // Colours::black;
    this->mGreyColour = juce::Colour::fromRGB(120, 120, 120);  // Colours::grey;
    this->mEditBgcolor = juce::Colour::fromRGB(172, 172, 172);
    this->mHlBgcolor = juce::Colour::fromRGB(190, 125, 18);

    this->mOnColor = juce::Colour::fromRGB(255, 165, 25);
    this->mOnColorOver = juce::Colour::fromRGB(255, 184, 75);
    this->mOnColorDown = juce::Colour::fromRGB(222, 144, 22);
    this->mOffColor = juce::Colour::fromRGB(56, 56, 56);

    this->mGreenColor = juce::Colour::fromRGB(56, 156, 56);
    this->mRedColor = juce::Colour::fromRGB(220, 48, 35);

    this->setColour(juce::PopupMenu::highlightedBackgroundColourId, this->mOnColor);
    this->setColour(juce::TextEditor::backgroundColourId, this->mEditBgcolor);
    this->setColour(juce::TextEditor::highlightColourId, this->mHlBgcolor);
    this->setColour(juce::TextEditor::shadowColourId, this->mEditBgcolor);

    this->setColour(juce::TextButton::buttonColourId, this->mEditBgcolor);

    this->setColour(juce::ComboBox::backgroundColourId, this->mEditBgcolor);
    this->setColour(juce::ComboBox::outlineColourId, this->mEditBgcolor);

    this->setColour(juce::Slider::thumbColourId, this->mLightColour);
    this->setColour(juce::Slider::rotarySliderFillColourId, this->mOnColor);
    this->setColour(juce::Slider::trackColourId, this->mDarkColour);
    this->setColour(juce::Slider::textBoxBackgroundColourId, this->mEditBgcolor);
    this->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    this->setColour(juce::TooltipWindow::ColourIds::backgroundColourId,
                    this->mBackGroundAndFieldColour.withBrightness(0.8f));
    this->setColour(juce::TooltipWindow::ColourIds::outlineColourId,
                    this->mBackGroundAndFieldColour.withBrightness(0.8f));

    this->setColour(juce::AlertWindow::backgroundColourId, this->mWinBackGroundAndFieldColour);
    this->setColour(juce::AlertWindow::outlineColourId, this->mOnColor);
    this->setColour(juce::AlertWindow::textColourId, this->mLightColour);

#if WIN32
    this->mFontSize = 16.f;
#else
    this->mFontSize = 10.f;
#endif

    this->mFont.setHeight(this->mFontSize);
    this->mSmallFont.setHeight(this->mFontSize - 1);
    this->mSmallerFont.setHeight(this->mFontSize - 3);
    this->mBigFont.setHeight(this->mFontSize + 3);
    this->mBiggerFont.setHeight(this->mFontSize + 6);
}

//==============================================================================
juce::Font GrisLookAndFeel::getLabelFont(juce::Label & label)
{
    if (label.getName() == "AboutBox_title") {
        return this->mBiggerFont;
    }
    if (label.getName() == "AboutBox_version") {
        return this->mBigFont;
    }
    return this->mFont;
}

//==============================================================================
// https://github.com/audioplastic/Juce-look-and-feel-examples/blob/master/JuceLibraryCode/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel.cpp
void GrisLookAndFeel::drawComboBox(juce::Graphics & g,
                                   int /*width*/,
                                   int /*height*/,
                                   bool /*isButtonDown*/,
                                   int const buttonX,
                                   int const buttonY,
                                   int const buttonW,
                                   int const buttonH,
                                   juce::ComboBox & box)
{
    box.setColour(juce::ColourSelector::backgroundColourId, this->mOnColor);

    g.fillAll(this->mEditBgcolor); // box.findColour (juce::ComboBox::backgroundColourId))

    auto const arrowX = 0.3f;
    auto const arrowH = 0.2f;

    auto const buttonXF{ narrow<float>(buttonX) };
    auto const buttonYF{ narrow<float>(buttonY) };
    auto const buttonWF{ narrow<float>(buttonW) };
    auto const buttonHF{ narrow<float>(buttonH) };

    juce::Path p;
    p.addTriangle(buttonXF + buttonWF * 0.5f,
                  buttonYF + buttonHF * (0.45f - arrowH),
                  buttonXF + buttonWF * (1.0f - arrowX),
                  buttonYF + buttonHF * 0.45f,
                  buttonXF + buttonWF * arrowX,
                  buttonYF + buttonHF * 0.45f);

    p.addTriangle(buttonXF + buttonWF * 0.5f,
                  buttonYF + buttonHF * (0.55f + arrowH),
                  buttonXF + buttonWF * (1.0f - arrowX),
                  buttonYF + buttonHF * 0.55f,
                  buttonXF + buttonWF * arrowX,
                  buttonYF + buttonHF * 0.55f);

    g.setColour(this->mDarkColour.withMultipliedAlpha(
        box.isEnabled() ? 1.0f : 0.3f)); // box.findColour (juce::ComboBox::arrowColourId)
    g.fillPath(p);
}

//==============================================================================
void GrisLookAndFeel::drawRoundThumb(juce::Graphics & g,
                                     float const x,
                                     float const y,
                                     float const diameter,
                                     juce::Colour const & colour,
                                     float const outlineThickness) const
{
    auto const halfThickness = outlineThickness * 0.5f;

    juce::Path p{};
    p.addEllipse(x + halfThickness, y + halfThickness, diameter - outlineThickness, diameter - outlineThickness);

    juce::DropShadow const ds(this->mDarkColour, 1, juce::Point<int>(0, 0));
    ds.drawForPath(g, p);

    g.setColour(colour);
    g.fillPath(p);

    g.setColour(colour.brighter());
    g.strokePath(p, juce::PathStrokeType(outlineThickness));
}

//==============================================================================
void GrisLookAndFeel::drawButtonBackground(juce::Graphics & g,
                                           juce::Button & button,
                                           juce::Colour const & /*backgroundColour*/,
                                           bool const isMouseOverButton,
                                           bool const isButtonDown)
{
    auto const width{ narrow<float>(button.getWidth() - 1) };
    auto const height{ narrow<float>(button.getHeight() - 1) };
    auto const cornerSize = juce::jmin(15.0f, juce::jmin(width, height) * 0.45f);
    auto const lineThickness = cornerSize * 0.1f;
    auto const halfThickness = lineThickness * 0.5f;
    juce::Path outline;
    outline.addRectangle(0.5f + halfThickness, 0.5f + halfThickness, width - lineThickness, height - lineThickness);
    g.setColour(button.findColour(juce::TextButton::buttonColourId));
    if (isButtonDown || isMouseOverButton) {
        g.setColour(this->mOnColorOver);
    }
    if (button.getToggleState()) {
        g.setColour(this->mOnColor); // outlineColour
    }
    if (button.isEnabled() && button.isMouseButtonDown()) {
        g.setColour(this->mOnColorDown);
    }
    g.fillPath(outline);
}

//==============================================================================
void GrisLookAndFeel::drawTickBox(juce::Graphics & g,
                                  juce::Component & component,
                                  float const x,
                                  float const y,
                                  float const w,
                                  float const h,
                                  bool const ticked,
                                  bool /*isEnabled*/,
                                  bool /*isMouseOverButton*/,
                                  bool /*isButtonDown*/)
{
    auto const boxSize{ w * 0.8f };
    juce::Rectangle<float> const r{ x, y + (h - boxSize) * 0.5f, boxSize, boxSize };

    if (ticked) {
        auto colour{ this->mOnColor };

        if (component.isMouseOver()) {
            colour = this->mOnColorOver;
        }

        if (!component.isEnabled()) {
            colour = this->mOnColor.withBrightness(0.3f);
        }
        g.setColour(colour);
        g.fillRect(r);

    } else {
        juce::Colour colour = this->mOffColor;
        if (!component.isEnabled()) {
            colour = this->mOffColor.withBrightness(0.3f);
        }
        g.setColour(colour);
        g.fillRect(r);
    }

    if (component.isEnabled() && component.isMouseButtonDown()) {
        g.setColour(this->mOnColorDown);
        g.fillRect(r);
    }
}

//==============================================================================
void GrisLookAndFeel::drawLinearSliderThumb(juce::Graphics & g,
                                            int const x,
                                            int const y,
                                            int const width,
                                            int const height,
                                            float const sliderPos,
                                            float const /*minSliderPos*/,
                                            float const /*maxSliderPos*/,
                                            juce::Slider::SliderStyle const style,
                                            juce::Slider & slider)
{
    auto const sliderRadius = static_cast<float>(getSliderThumbRadius(slider) - 2);
    float kx, ky;

    if (style == juce::Slider::LinearVertical) {
        kx = narrow<float>(x) + narrow<float>(width) * 0.5f;
        ky = sliderPos;
    } else {
        kx = sliderPos;
        ky = narrow<float>(y) + narrow<float>(height) * 0.5f;
    }
    const juce::Rectangle<float> r(kx - (sliderRadius / 2.0f), ky - sliderRadius, 6, narrow<float>(height) * 2.0f);

    if (slider.isEnabled()) {
        auto colour = this->mOnColor;

        if (slider.isMouseOver()) {
            colour = this->mOnColorOver;
        }
        if (slider.isMouseButtonDown()) {
            colour = this->mOnColorDown;
        }
        g.setColour(colour);
        g.fillRect(r);
    } else {
        g.setColour(this->mOffColor);
        g.fillRect(r);
    }
}

//==============================================================================
void GrisLookAndFeel::drawLinearSlider(juce::Graphics & g,
                                       int const x,
                                       int const y,
                                       int const width,
                                       int const height,
                                       float const sliderPos,
                                       float const minSliderPos,
                                       float const maxSliderPos,
                                       juce::Slider::SliderStyle const style,
                                       juce::Slider & slider)
{
    drawLinearSliderBackground(g, x, y, width, height + 2, sliderPos, minSliderPos, maxSliderPos, style, slider);
    drawLinearSliderThumb(g, x, y, width, height + 2, sliderPos, minSliderPos, maxSliderPos, style, slider);
}

//==============================================================================
void GrisLookAndFeel::drawLinearSliderBackground(juce::Graphics & g,
                                                 int const x,
                                                 int const y,
                                                 int const width,
                                                 int const height,
                                                 float const /*sliderPos*/,
                                                 float const /*minSliderPos*/,
                                                 float const /*maxSliderPos*/,
                                                 const juce::Slider::SliderStyle /*style*/,
                                                 juce::Slider & slider)
{
    auto const sliderRadius{ narrow<float>(getSliderThumbRadius(slider) - 5) };
    juce::Path on, off;

    auto const x_f{ narrow<float>(x) };
    auto const y_f{ narrow<float>(y) };
    auto const width_f{ narrow<float>(width) };
    auto const height_f{ narrow<float>(height) };

    if (slider.isHorizontal()) {
        auto const iy = y_f + height_f * 0.5f - sliderRadius * 0.5f;
        juce::Rectangle<float> r(x_f - sliderRadius * 0.5f, iy, width_f + sliderRadius, sliderRadius);
        auto const onW = r.getWidth() * static_cast<float>(slider.valueToProportionOfLength(slider.getValue()));
        on.addRectangle(r.removeFromLeft(onW));
        off.addRectangle(r);
    } else {
        auto const ix = x_f + width_f * 0.5f - sliderRadius * 0.5f;
        juce::Rectangle<float> r(ix, y_f - sliderRadius * 0.5f, sliderRadius, height_f + sliderRadius);
        auto const onH = r.getHeight() * static_cast<float>(slider.valueToProportionOfLength(slider.getValue()));
        on.addRectangle(r.removeFromBottom(onH));
        off.addRectangle(r);
    }

    if (slider.isEnabled()) {
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
        g.fillPath(on);
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillPath(off);
    } else {
        g.setColour(this->mOffColor);
        g.fillPath(on);
        g.fillPath(off);
    }
}

//==============================================================================
void GrisLookAndFeel::fillTextEditorBackground(juce::Graphics & g,
                                               int /*width*/,
                                               int /*height*/,
                                               juce::TextEditor & /*t*/)
{
    g.setColour(this->mEditBgcolor);
    g.fillAll();
}

//==============================================================================
void GrisLookAndFeel::drawTextEditorOutline(juce::Graphics & g, int width, int height, juce::TextEditor & t)
{
    if (t.hasKeyboardFocus(true)) {
        g.setColour(this->mOnColor);
        g.drawRect(0, 0, width, height);
    }
}

//==============================================================================
void GrisLookAndFeel::drawToggleButton(juce::Graphics & g,
                                       juce::ToggleButton & button,
                                       bool const isMouseOverButton,
                                       bool const isButtonDown)
{
    if (button.hasKeyboardFocus(true)) {
        g.setColour(button.findColour(juce::TextEditor::focusedOutlineColourId));
    }

    if (button.getButtonText().length() == 1) {
        drawTickBox(g,
                    button,
                    0,
                    0,
                    static_cast<float>(button.getWidth()),
                    static_cast<float>(button.getHeight()),
                    button.getToggleState(),
                    button.isEnabled(),
                    isMouseOverButton,
                    isButtonDown);
        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(this->mFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(button.getButtonText(),
                         -2,
                         1,
                         button.getWidth(),
                         button.getHeight(),
                         juce::Justification::centred,
                         10);

    } else {
        auto const fontSize = juce::jmin(15.0f, narrow<float>(button.getHeight()) * 0.75f);
        auto const tickWidth = fontSize * 1.1f;

        drawTickBox(g,
                    button,
                    4.0f,
                    (narrow<float>(button.getHeight()) - tickWidth) * 0.5f,
                    tickWidth,
                    tickWidth,
                    button.getToggleState(),
                    button.isEnabled(),
                    isMouseOverButton,
                    isButtonDown);

        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(this->mFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        auto const textX = static_cast<int>(tickWidth) + 5;

        g.drawFittedText(button.getButtonText(),
                         textX,
                         0,
                         button.getWidth() - (textX - 5),
                         button.getHeight(),
                         juce::Justification::centredLeft,
                         10);
    }
}

//==============================================================================
void GrisLookAndFeel::drawTabButton(juce::TabBarButton & button,
                                    juce::Graphics & g,
                                    bool const isMouseOver,
                                    bool const isMouseDown)
{
    auto const activeArea(button.getActiveArea());
    auto const orientation = button.getTabbedButtonBar().getOrientation();
    auto const bkg{ button.getTabBackgroundColour() };

    if (button.getToggleState()) {
        g.setColour(bkg);
    } else {
        g.setColour(bkg.brighter(0.1f));
    }

    g.fillRect(activeArea);

    g.setColour(this->mWinBackGroundAndFieldColour);

    juce::Rectangle<int> r(activeArea);
    if (orientation != juce::TabbedButtonBar::TabsAtTop) {
        g.fillRect(r.removeFromBottom(1));
    }
    if (orientation != juce::TabbedButtonBar::TabsAtRight) {
        g.fillRect(r.removeFromLeft(1));
    }
    if (orientation != juce::TabbedButtonBar::TabsAtLeft) {
        g.fillRect(r.removeFromRight(1));
    }

    auto const alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
    auto const col = (bkg.contrasting().withMultipliedAlpha(alpha));
    auto const area(button.getTextArea().toFloat());

    auto length = area.getWidth();
    auto depth = area.getHeight();

    if (button.getTabbedButtonBar().isVertical()) {
        std::swap(length, depth);
    }

    juce::TextLayout textLayout;
    createTabTextLayout(button, length, depth, col, textLayout);
    textLayout.draw(g, juce::Rectangle<float>(length, depth));
}

//==============================================================================
void GrisLookAndFeel::createTabTextLayout(juce::TabBarButton const & button,
                                          float const length,
                                          float const depth,
                                          juce::Colour const colour,
                                          juce::TextLayout & textLayout) const
{
    auto font{ this->mFont };
#if WIN32
    font.setHeight(depth * 0.60f);
#else

    font.setHeight(depth * 0.35f);
#endif
    font.setUnderline(button.hasKeyboardFocus(false));

    juce::AttributedString s;
    s.setJustification(juce::Justification::centred);
    s.append(button.getButtonText().trim(), font, colour);

    textLayout.createLayout(s, length);
}

//==============================================================================
void GrisLookAndFeel::drawRotarySlider(juce::Graphics & g,
                                       int const x,
                                       int const y,
                                       int const width,
                                       int const height,
                                       float const sliderPos,
                                       float const rotaryStartAngle,
                                       float const rotaryEndAngle,
                                       juce::Slider & slider)
{
    auto const radius = juce::jmin(width / 2, height / 2) - 2.0f;
    auto const centreX = narrow<float>(x) + narrow<float>(width) * 0.5f;
    auto const centreY = narrow<float>(y) + narrow<float>(height) * 0.5f + 6.0f;
    auto const rx = centreX - radius;
    auto const ry = centreY - radius;
    auto const rw = radius * 2.0f;
    auto const angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto const isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();

    if (slider.isEnabled()) {
        if (isMouseOver) {
            g.setColour(this->mOnColorOver);
        } else {
            g.setColour(this->mOnColor);
        }
    } else {
        g.setColour(this->mOffColor);
    }
    juce::Path filledArc;
    filledArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, angle, 0.0);
    g.fillPath(filledArc);
    const float lineThickness = juce::jmin(15.0f, juce::jmin(width, height) * 0.45f) * 0.1f;
    juce::Path outlineArc;
    outlineArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, 0.0);
    g.strokePath(outlineArc, juce::PathStrokeType(lineThickness));
}

void SmallGrisLookAndFeel::drawToggleButton(juce::Graphics & g,
                                            juce::ToggleButton & button,
                                            bool const isMouseOverButton,
                                            bool const isButtonDown)
{
    if (button.hasKeyboardFocus(true)) {
        g.setColour(button.findColour(juce::TextEditor::focusedOutlineColourId));
    }

    if (button.getButtonText().length() == 1) {
        drawTickBox(g,
                    button,
                    0,
                    0,
                    static_cast<float>(button.getWidth()),
                    static_cast<float>(button.getHeight()),
                    button.getToggleState(),
                    button.isEnabled(),
                    isMouseOverButton,
                    isButtonDown);
        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(this->mSmallFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(button.getButtonText(),
                         -2,
                         1,
                         button.getWidth(),
                         button.getHeight(),
                         juce::Justification::centred,
                         10);

    } else {
        auto const fontSize = juce::jmin(15.0f, narrow<float>(button.getHeight()) * 0.75f);
        auto const tickWidth = fontSize * 1.1f;

        drawTickBox(g,
                    button,
                    4.0f,
                    (narrow<float>(button.getHeight()) - tickWidth) * 0.5f,
                    tickWidth,
                    tickWidth,
                    button.getToggleState(),
                    button.isEnabled(),
                    isMouseOverButton,
                    isButtonDown);

        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(this->mSmallFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        auto const textX = static_cast<int>(tickWidth) + 5;

        g.drawFittedText(button.getButtonText(),
                         textX,
                         0,
                         button.getWidth() - (textX - 5),
                         button.getHeight(),
                         juce::Justification::centredLeft,
                         10);
    }
}