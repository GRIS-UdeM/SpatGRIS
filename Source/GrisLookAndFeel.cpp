/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Belanger, Nicolas Masson

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

#include "GrisLookAndFeel.h"

//==============================================================================
GrisLookAndFeel::GrisLookAndFeel()
{
    this->mBackGroundAndFieldColour = juce::Colour::fromRGB(75, 75, 75); // Colours::darkgrey;
    this->mWinBackGroundAndFieldColour = juce::Colour::fromRGB(46, 46, 46);

    this->mLightColour = juce::Colour::fromRGB(235, 245, 250); // Colours::whitesmoke;
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

    this->setColour(PopupMenu::highlightedBackgroundColourId, this->mOnColor);
    this->setColour(TextEditor::backgroundColourId, this->mEditBgcolor);
    this->setColour(TextEditor::highlightColourId, this->mHlBgcolor);
    this->setColour(TextEditor::shadowColourId, this->mEditBgcolor);

    this->setColour(TextButton::buttonColourId, this->mEditBgcolor);

    this->setColour(ComboBox::backgroundColourId, this->mEditBgcolor);
    this->setColour(ComboBox::outlineColourId, this->mEditBgcolor);

    this->setColour(Slider::thumbColourId, this->mLightColour);
    this->setColour(Slider::rotarySliderFillColourId, this->mOnColor);
    this->setColour(Slider::trackColourId, this->mDarkColour);
    this->setColour(Slider::textBoxBackgroundColourId, this->mEditBgcolor);
    this->setColour(Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    this->setColour(TooltipWindow::ColourIds::backgroundColourId, this->mBackGroundAndFieldColour.withBrightness(0.8f));
    this->setColour(TooltipWindow::ColourIds::outlineColourId, this->mBackGroundAndFieldColour.withBrightness(0.8f));

    this->setColour(AlertWindow::backgroundColourId, this->mWinBackGroundAndFieldColour);
    this->setColour(AlertWindow::outlineColourId, this->mOnColor);
    this->setColour(AlertWindow::textColourId, this->mLightColour);

#if WIN32
    this->mFontSize = 18.f;
#else
    this->mFontSize = 10.f;
#endif

    this->mFont.setHeight(this->mFontSize);
    this->mSmallFont.setHeight(this->mFontSize - 1);
    this->mSmallerFont.setHeight(this->mFontSize - 2);
    this->mBigFont.setHeight(this->mFontSize + 3);
    this->mBiggerFont.setHeight(this->mFontSize + 6);
}

//==============================================================================
juce::Font GrisLookAndFeel::getLabelFont(Label & label)
{
    if (label.getName() == "AboutBox_title")
        return this->mBiggerFont;
    else if (label.getName() == "AboutBox_version")
        return this->mBigFont;
    else
        return this->mFont;
}

//==============================================================================
// https://github.com/audioplastic/Juce-look-and-feel-examples/blob/master/JuceLibraryCode/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel.cpp
void GrisLookAndFeel::drawComboBox(Graphics & g,
                                   int /*width*/,
                                   int /*height*/,
                                   bool /*isButtonDown*/,
                                   int buttonX,
                                   int buttonY,
                                   int buttonW,
                                   int buttonH,
                                   ComboBox & box)
{
    box.setColour(ColourSelector::backgroundColourId, this->mOnColor);

    g.fillAll(this->mEditBgcolor); // box.findColour (ComboBox::backgroundColourId))

    const float arrowX = 0.3f;
    const float arrowH = 0.2f;

    Path p;
    p.addTriangle(buttonX + buttonW * 0.5f,
                  buttonY + buttonH * (0.45f - arrowH),
                  buttonX + buttonW * (1.0f - arrowX),
                  buttonY + buttonH * 0.45f,
                  buttonX + buttonW * arrowX,
                  buttonY + buttonH * 0.45f);

    p.addTriangle(buttonX + buttonW * 0.5f,
                  buttonY + buttonH * (0.55f + arrowH),
                  buttonX + buttonW * (1.0f - arrowX),
                  buttonY + buttonH * 0.55f,
                  buttonX + buttonW * arrowX,
                  buttonY + buttonH * 0.55f);

    g.setColour(this->mDarkColour.withMultipliedAlpha(
        box.isEnabled() ? 1.0f : 0.3f)); // box.findColour (ComboBox::arrowColourId)
    g.fillPath(p);
}

//==============================================================================
void GrisLookAndFeel::drawRoundThumb(Graphics & g,
                                     float const x,
                                     float const y,
                                     float const diameter,
                                     juce::Colour const & colour,
                                     float const outlineThickness) const
{
    juce::Rectangle<float> const a(x, y, diameter, diameter);
    float const halfThickness = outlineThickness * 0.5f;

    Path p{};
    p.addEllipse(x + halfThickness, y + halfThickness, diameter - outlineThickness, diameter - outlineThickness);

    DropShadow const ds(this->mDarkColour, 1, Point<int>(0, 0));
    ds.drawForPath(g, p);

    g.setColour(colour);
    g.fillPath(p);

    g.setColour(colour.brighter());
    g.strokePath(p, PathStrokeType(outlineThickness));
}

//==============================================================================
void GrisLookAndFeel::drawButtonBackground(Graphics & g,
                                           Button & button,
                                           const juce::Colour & /*backgroundColour*/,
                                           bool isMouseOverButton,
                                           bool isButtonDown)
{
    const float width = button.getWidth() - 1.0f;
    const float height = button.getHeight() - 1.0f;
    const float cornerSize = jmin(15.0f, jmin(width, height) * 0.45f);
    const float lineThickness = cornerSize * 0.1f;
    const float halfThickness = lineThickness * 0.5f;
    Path outline;
    outline.addRectangle(0.5f + halfThickness, 0.5f + halfThickness, width - lineThickness, height - lineThickness);
    g.setColour(button.findColour(TextButton::buttonColourId));
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
void GrisLookAndFeel::drawTickBox(Graphics & g,
                                  Component & component,
                                  float x,
                                  float y,
                                  float w,
                                  float h,
                                  bool ticked,
                                  bool /*isEnabled*/,
                                  bool /*isMouseOverButton*/,
                                  bool /*isButtonDown*/)
{
    const float boxSize = w * 0.8f;
    const juce::Rectangle<float> r(x, y + (h - boxSize) * 0.5f, boxSize, boxSize);

    if (ticked) {
        juce::Colour colour = this->mOnColor;

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
void GrisLookAndFeel::drawLinearSliderThumb(Graphics & g,
                                            int x,
                                            int y,
                                            int width,
                                            int height,
                                            float sliderPos,
                                            float /*minSliderPos*/,
                                            float /*maxSliderPos*/,
                                            const Slider::SliderStyle style,
                                            Slider & slider)
{
    const float sliderRadius = (float)(getSliderThumbRadius(slider) - 2);
    float kx, ky;

    if (style == Slider::LinearVertical) {
        kx = x + width * 0.5f;
        ky = sliderPos;
    } else {
        kx = sliderPos;
        ky = y + height * 0.5f;
    }
    const juce::Rectangle<float> r(kx - (sliderRadius / 2.0f), ky - sliderRadius, 6, height * 2.0f);

    if (slider.isEnabled()) {
        juce::Colour colour = this->mOnColor;

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
void GrisLookAndFeel::drawLinearSlider(Graphics & g,
                                       int x,
                                       int y,
                                       int width,
                                       int height,
                                       float sliderPos,
                                       float minSliderPos,
                                       float maxSliderPos,
                                       const Slider::SliderStyle style,
                                       Slider & slider)
{
    drawLinearSliderBackground(g, x, y, width, height + 2, sliderPos, minSliderPos, maxSliderPos, style, slider);
    drawLinearSliderThumb(g, x, y, width, height + 2, sliderPos, minSliderPos, maxSliderPos, style, slider);
}

//==============================================================================
void GrisLookAndFeel::drawLinearSliderBackground(Graphics & g,
                                                 int x,
                                                 int y,
                                                 int width,
                                                 int height,
                                                 float /*sliderPos*/,
                                                 float /*minSliderPos*/,
                                                 float /*maxSliderPos*/,
                                                 const Slider::SliderStyle /*style*/,
                                                 Slider & slider)
{
    const float sliderRadius = getSliderThumbRadius(slider) - 5.0f;
    Path on, off;

    if (slider.isHorizontal()) {
        const float iy = y + height * 0.5f - sliderRadius * 0.5f;
        juce::Rectangle<float> r(x - sliderRadius * 0.5f, iy, width + sliderRadius, sliderRadius);
        const float onW = r.getWidth() * ((float)slider.valueToProportionOfLength(slider.getValue()));
        on.addRectangle(r.removeFromLeft(onW));
        off.addRectangle(r);
    } else {
        const float ix = x + width * 0.5f - sliderRadius * 0.5f;
        juce::Rectangle<float> r(ix, y - sliderRadius * 0.5f, sliderRadius, height + sliderRadius);
        const float onH = r.getHeight() * ((float)slider.valueToProportionOfLength(slider.getValue()));
        on.addRectangle(r.removeFromBottom(onH));
        off.addRectangle(r);
    }

    if (slider.isEnabled()) {
        g.setColour(slider.findColour(Slider::rotarySliderFillColourId));
        g.fillPath(on);
        g.setColour(slider.findColour(Slider::trackColourId));
        g.fillPath(off);
    } else {
        g.setColour(this->mOffColor);
        g.fillPath(on);
        g.fillPath(off);
    }
}

//==============================================================================
void GrisLookAndFeel::fillTextEditorBackground(Graphics & g, int /*width*/, int /*height*/, TextEditor & /*t*/)
{
    g.setColour(this->mEditBgcolor);
    g.fillAll();
}

//==============================================================================
void GrisLookAndFeel::drawTextEditorOutline(Graphics & g, int width, int height, TextEditor & t)
{
    if (t.hasKeyboardFocus(true)) {
        g.setColour(this->mOnColor);
        g.drawRect(0, 0, width, height);
    }
}

//==============================================================================
void GrisLookAndFeel::drawToggleButton(Graphics & g, ToggleButton & button, bool isMouseOverButton, bool isButtonDown)
{
    if (button.hasKeyboardFocus(true)) {
        g.setColour(button.findColour(TextEditor::focusedOutlineColourId));
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
        g.setColour(button.findColour(ToggleButton::textColourId));
        g.setFont(this->mFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(button.getButtonText(),
                         -2,
                         1,
                         button.getWidth(),
                         button.getHeight(),
                         Justification::centred,
                         10);

    } else {
        float fontSize = jmin(15.0f, button.getHeight() * 0.75f);
        const float tickWidth = fontSize * 1.1f;

        drawTickBox(g,
                    button,
                    4.0f,
                    (button.getHeight() - tickWidth) * 0.5f,
                    tickWidth,
                    tickWidth,
                    button.getToggleState(),
                    button.isEnabled(),
                    isMouseOverButton,
                    isButtonDown);

        g.setColour(button.findColour(ToggleButton::textColourId));
        g.setFont(this->mFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        const int textX = (int)tickWidth + 5;

        g.drawFittedText(button.getButtonText(),
                         textX,
                         0,
                         button.getWidth() - (textX - 5),
                         button.getHeight(),
                         Justification::centredLeft,
                         10);
    }
}

//==============================================================================
void GrisLookAndFeel::drawTabButton(TabBarButton & button, Graphics & g, bool isMouseOver, bool isMouseDown)
{
    const juce::Rectangle<int> activeArea(button.getActiveArea());
    activeArea.withHeight(18);
    const TabbedButtonBar::Orientation o = button.getTabbedButtonBar().getOrientation();
    const juce::Colour bkg(button.getTabBackgroundColour());

    if (button.getToggleState()) {
        g.setColour(bkg);
    } else {
        g.setColour(bkg.brighter(0.1f));
    }

    g.fillRect(activeArea);

    g.setColour(this->mWinBackGroundAndFieldColour);

    juce::Rectangle<int> r(activeArea);
    if (o != TabbedButtonBar::TabsAtTop)
        g.fillRect(r.removeFromBottom(1));
    if (o != TabbedButtonBar::TabsAtRight)
        g.fillRect(r.removeFromLeft(1));
    if (o != TabbedButtonBar::TabsAtLeft)
        g.fillRect(r.removeFromRight(1));

    const float alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
    juce::Colour col = (bkg.contrasting().withMultipliedAlpha(alpha));
    const juce::Rectangle<float> area(button.getTextArea().toFloat());

    float length = area.getWidth();
    float depth = area.getHeight();

    if (button.getTabbedButtonBar().isVertical())
        std::swap(length, depth);

    TextLayout textLayout;
    createTabTextLayout(button, length, depth, col, textLayout);
    textLayout.draw(g, juce::Rectangle<float>(length, depth));
}

//==============================================================================
void GrisLookAndFeel::createTabTextLayout(const TabBarButton & button,
                                          float length,
                                          float depth,
                                          juce::Colour const colour,
                                          TextLayout & textLayout) const
{
    Font font(this->mFont);
#if WIN32
    font.setHeight(depth * 0.60f);
#else

    font.setHeight(depth * 0.35f);
#endif
    font.setUnderline(button.hasKeyboardFocus(false));

    AttributedString s;
    s.setJustification(Justification::centred);
    s.append(button.getButtonText().trim(), font, colour);

    textLayout.createLayout(s, length);
}

//==============================================================================
void GrisLookAndFeel::drawRotarySlider(Graphics & g,
                                       int x,
                                       int y,
                                       int width,
                                       int height,
                                       float sliderPos,
                                       float rotaryStartAngle,
                                       float rotaryEndAngle,
                                       Slider & slider)
{
    const float radius = jmin(width / 2, height / 2) - 2.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = (y + height * 0.5f) + 6.0f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const bool isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();

    if (slider.isEnabled()) {
        // slider.findColour (Slider::rotarySliderFillColourId).withAlpha (isMouseOver ? 0.7f : 1.0f)
        if (isMouseOver) {
            g.setColour(this->mOnColorOver);
        } else {
            g.setColour(this->mOnColor);
        }
    } else {
        g.setColour(this->mOffColor);
    }
    Path filledArc;
    filledArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, angle, 0.0);
    g.fillPath(filledArc);
    const float lineThickness = jmin(15.0f, jmin(width, height) * 0.45f) * 0.1f;
    Path outlineArc;
    outlineArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, 0.0);
    g.strokePath(outlineArc, PathStrokeType(lineThickness));
}

void SmallGrisLookAndFeel::drawToggleButton(Graphics & g,
                                            ToggleButton & button,
                                            bool isMouseOverButton,
                                            bool isButtonDown)
{
    if (button.hasKeyboardFocus(true)) {
        g.setColour(button.findColour(TextEditor::focusedOutlineColourId));
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
        g.setColour(button.findColour(ToggleButton::textColourId));
        g.setFont(this->mSmallFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(button.getButtonText(),
                         -2,
                         1,
                         button.getWidth(),
                         button.getHeight(),
                         Justification::centred,
                         10);

    } else {
        float fontSize = jmin(15.0f, button.getHeight() * 0.75f);
        const float tickWidth = fontSize * 1.1f;

        drawTickBox(g,
                    button,
                    4.0f,
                    (button.getHeight() - tickWidth) * 0.5f,
                    tickWidth,
                    tickWidth,
                    button.getToggleState(),
                    button.isEnabled(),
                    isMouseOverButton,
                    isButtonDown);

        g.setColour(button.findColour(ToggleButton::textColourId));
        g.setFont(this->mSmallFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        const int textX = (int)tickWidth + 5;

        g.drawFittedText(button.getButtonText(),
                         textX,
                         0,
                         button.getWidth() - (textX - 5),
                         button.getHeight(),
                         Justification::centredLeft,
                         10);
    }
}