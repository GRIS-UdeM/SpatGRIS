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
    this->backGroundAndFieldColour = juce::Colour::fromRGB(75, 75, 75); // Colours::darkgrey;
    this->winBackGroundAndFieldColour = juce::Colour::fromRGB(46, 46, 46);

    this->lightColour = juce::Colour::fromRGB(235, 245, 250); // Colours::whitesmoke;
    this->darkColour = juce::Colour::fromRGB(15, 10, 5);      // Colours::black;
    this->greyColour = juce::Colour::fromRGB(120, 120, 120);  // Colours::grey;
    this->editBgcolor = juce::Colour::fromRGB(172, 172, 172);
    this->hlBgcolor = juce::Colour::fromRGB(190, 125, 18);

    this->onColor = juce::Colour::fromRGB(255, 165, 25);
    this->onColorOver = juce::Colour::fromRGB(255, 184, 75);
    this->onColorDown = juce::Colour::fromRGB(222, 144, 22);
    this->offColor = juce::Colour::fromRGB(56, 56, 56);

    this->greenColor = juce::Colour::fromRGB(56, 156, 56);
    this->redColor = juce::Colour::fromRGB(220, 48, 35);

    this->setColour(PopupMenu::highlightedBackgroundColourId, this->onColor);
    this->setColour(TextEditor::backgroundColourId, this->editBgcolor);
    this->setColour(TextEditor::highlightColourId, this->hlBgcolor);
    this->setColour(TextEditor::shadowColourId, this->editBgcolor);

    this->setColour(TextButton::buttonColourId, this->editBgcolor);

    this->setColour(ComboBox::backgroundColourId, this->editBgcolor);
    this->setColour(ComboBox::outlineColourId, this->editBgcolor);

    this->setColour(Slider::thumbColourId, this->lightColour);
    this->setColour(Slider::rotarySliderFillColourId, this->onColor);
    this->setColour(Slider::trackColourId, this->darkColour);
    this->setColour(Slider::textBoxBackgroundColourId, this->editBgcolor);
    this->setColour(Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    this->setColour(TooltipWindow::ColourIds::backgroundColourId, this->backGroundAndFieldColour.withBrightness(0.8));
    this->setColour(TooltipWindow::ColourIds::outlineColourId, this->backGroundAndFieldColour.withBrightness(0.8));

    this->setColour(AlertWindow::backgroundColourId, this->winBackGroundAndFieldColour);
    this->setColour(AlertWindow::outlineColourId, this->onColor);
    this->setColour(AlertWindow::textColourId, this->lightColour);

#if WIN32
    this->fontSize = 18.f;
#else
    this->fontSize = 10.f;
#endif

    this->font.setHeight(this->fontSize);
    this->smallFont.setHeight(this->fontSize - 1);
    this->smallerFont.setHeight(this->fontSize - 2);
    this->bigFont.setHeight(this->fontSize + 3);
    this->biggerFont.setHeight(this->fontSize + 6);
}

//==============================================================================
juce::Font GrisLookAndFeel::getLabelFont(Label & label)
{
    if (label.getName() == "AboutBox_title")
        return this->biggerFont;
    else if (label.getName() == "AboutBox_version")
        return this->bigFont;
    else
        return this->font;
}

//==============================================================================
// https://github.com/audioplastic/Juce-look-and-feel-examples/blob/master/JuceLibraryCode/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel.cpp
void GrisLookAndFeel::drawComboBox(Graphics & g,
                                   int        width,
                                   int        height,
                                   bool       isButtonDown,
                                   int        buttonX,
                                   int        buttonY,
                                   int        buttonW,
                                   int        buttonH,
                                   ComboBox & box)
{
    box.setColour(ColourSelector::backgroundColourId, this->onColor);

    g.fillAll(this->editBgcolor); // box.findColour (ComboBox::backgroundColourId))

    const float arrowX = 0.3f;
    const float arrowH = 0.2f;

    Path p;
    p.addTriangle(buttonX + buttonW * 0.5f, buttonY + buttonH * (0.45f - arrowH), buttonX + buttonW * (1.0f - arrowX),
                  buttonY + buttonH * 0.45f, buttonX + buttonW * arrowX, buttonY + buttonH * 0.45f);

    p.addTriangle(buttonX + buttonW * 0.5f, buttonY + buttonH * (0.55f + arrowH), buttonX + buttonW * (1.0f - arrowX),
                  buttonY + buttonH * 0.55f, buttonX + buttonW * arrowX, buttonY + buttonH * 0.55f);

    g.setColour(this->darkColour.withMultipliedAlpha(
        box.isEnabled() ? 1.0f : 0.3f)); // box.findColour (ComboBox::arrowColourId)
    g.fillPath(p);
}

//==============================================================================
void GrisLookAndFeel::drawRoundThumb(Graphics &           g,
                                     float const          x,
                                     float const          y,
                                     float const          diameter,
                                     juce::Colour const & colour,
                                     float const          outlineThickness) const
{
    juce::Rectangle<float> const a(x, y, diameter, diameter);
    float const                  halfThickness = outlineThickness * 0.5f;

    Path p{};
    p.addEllipse(x + halfThickness, y + halfThickness, diameter - outlineThickness, diameter - outlineThickness);

    DropShadow const ds(this->darkColour, 1, Point<int>(0, 0));
    ds.drawForPath(g, p);

    g.setColour(colour);
    g.fillPath(p);

    g.setColour(colour.brighter());
    g.strokePath(p, PathStrokeType(outlineThickness));
}

//==============================================================================
void GrisLookAndFeel::drawButtonBackground(Graphics &           g,
                                           Button &             button,
                                           const juce::Colour & backgroundColour,
                                           bool                 isMouseOverButton,
                                           bool                 isButtonDown)
{
    const float width = button.getWidth() - 1.0f;
    const float height = button.getHeight() - 1.0f;
    const float cornerSize = jmin(15.0f, jmin(width, height) * 0.45f);
    const float lineThickness = cornerSize * 0.1f;
    const float halfThickness = lineThickness * 0.5f;
    Path        outline;
    outline.addRectangle(0.5f + halfThickness, 0.5f + halfThickness, width - lineThickness, height - lineThickness);
    g.setColour(button.findColour(TextButton::buttonColourId));
    if (isButtonDown || isMouseOverButton) {
        g.setColour(this->onColorOver);
    }
    if (button.getToggleState()) {
        g.setColour(this->onColor); // outlineColour
    }
    if (button.isEnabled() && button.isMouseButtonDown()) {
        g.setColour(this->onColorDown);
    }
    g.fillPath(outline);
}

//==============================================================================
void GrisLookAndFeel::drawTickBox(Graphics &  g,
                                  Component & component,
                                  float       x,
                                  float       y,
                                  float       w,
                                  float       h,
                                  bool        ticked,
                                  bool        isEnabled,
                                  bool        isMouseOverButton,
                                  bool        isButtonDown)
{
    const float                  boxSize = w * 0.8f;
    const juce::Rectangle<float> r(x, y + (h - boxSize) * 0.5f, boxSize, boxSize);

    if (ticked) {
        juce::Colour colour = this->onColor;

        if (component.isMouseOver()) {
            colour = this->onColorOver;
        }

        if (!component.isEnabled()) {
            colour = this->onColor.withBrightness(0.3f);
        }
        g.setColour(colour);
        g.fillRect(r);

    } else {
        juce::Colour colour = this->offColor;
        if (!component.isEnabled()) {
            colour = this->offColor.withBrightness(0.3f);
        }
        g.setColour(colour);
        g.fillRect(r);
    }

    if (component.isEnabled() && component.isMouseButtonDown()) {
        g.setColour(this->onColorDown);
        g.fillRect(r);
    }
}

//==============================================================================
void GrisLookAndFeel::drawLinearSliderThumb(Graphics &                g,
                                            int                       x,
                                            int                       y,
                                            int                       width,
                                            int                       height,
                                            float                     sliderPos,
                                            float                     minSliderPos,
                                            float                     maxSliderPos,
                                            const Slider::SliderStyle style,
                                            Slider &                  slider)
{
    const float sliderRadius = (float)(getSliderThumbRadius(slider) - 2);
    float       kx, ky;

    if (style == Slider::LinearVertical) {
        kx = x + width * 0.5f;
        ky = sliderPos;
    } else {
        kx = sliderPos;
        ky = y + height * 0.5f;
    }
    const juce::Rectangle<float> r(kx - (sliderRadius / 2.0f), ky - sliderRadius, 6, height * 2.0f);

    if (slider.isEnabled()) {
        juce::Colour colour = this->onColor;

        if (slider.isMouseOver()) {
            colour = this->onColorOver;
        }
        if (slider.isMouseButtonDown()) {
            colour = this->onColorDown;
        }
        g.setColour(colour);
        g.fillRect(r);
    } else {
        g.setColour(this->offColor);
        g.fillRect(r);
    }
}

//==============================================================================
void GrisLookAndFeel::drawLinearSlider(Graphics &                g,
                                       int                       x,
                                       int                       y,
                                       int                       width,
                                       int                       height,
                                       float                     sliderPos,
                                       float                     minSliderPos,
                                       float                     maxSliderPos,
                                       const Slider::SliderStyle style,
                                       Slider &                  slider)
{
    drawLinearSliderBackground(g, x, y, width, height + 2, sliderPos, minSliderPos, maxSliderPos, style, slider);
    drawLinearSliderThumb(g, x, y, width, height + 2, sliderPos, minSliderPos, maxSliderPos, style, slider);
}

//==============================================================================
void GrisLookAndFeel::drawLinearSliderBackground(Graphics & g,
                                                 int        x,
                                                 int        y,
                                                 int        width,
                                                 int        height,
                                                 float /*sliderPos*/,
                                                 float /*minSliderPos*/,
                                                 float /*maxSliderPos*/,
                                                 const Slider::SliderStyle /*style*/,
                                                 Slider & slider)
{
    const float sliderRadius = getSliderThumbRadius(slider) - 5.0f;
    Path        on, off;

    if (slider.isHorizontal()) {
        const float            iy = y + height * 0.5f - sliderRadius * 0.5f;
        juce::Rectangle<float> r(x - sliderRadius * 0.5f, iy, width + sliderRadius, sliderRadius);
        const float            onW = r.getWidth() * ((float)slider.valueToProportionOfLength(slider.getValue()));
        on.addRectangle(r.removeFromLeft(onW));
        off.addRectangle(r);
    } else {
        const float            ix = x + width * 0.5f - sliderRadius * 0.5f;
        juce::Rectangle<float> r(ix, y - sliderRadius * 0.5f, sliderRadius, height + sliderRadius);
        const float            onH = r.getHeight() * ((float)slider.valueToProportionOfLength(slider.getValue()));
        on.addRectangle(r.removeFromBottom(onH));
        off.addRectangle(r);
    }

    if (slider.isEnabled()) {
        g.setColour(slider.findColour(Slider::rotarySliderFillColourId));
        g.fillPath(on);
        g.setColour(slider.findColour(Slider::trackColourId));
        g.fillPath(off);
    } else {
        g.setColour(this->offColor);
        g.fillPath(on);
        g.fillPath(off);
    }
}

//==============================================================================
void GrisLookAndFeel::fillTextEditorBackground(Graphics & g, int width, int height, TextEditor & t)
{
    g.setColour(this->editBgcolor);
    g.fillAll();
}

//==============================================================================
void GrisLookAndFeel::drawTextEditorOutline(Graphics & g, int width, int height, TextEditor & t)
{
    if (t.hasKeyboardFocus(true)) {
        g.setColour(this->onColor);
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
        drawTickBox(g, button, 0, 0, button.getWidth(), button.getHeight(), button.getToggleState(), button.isEnabled(),
                    isMouseOverButton, isButtonDown);
        g.setColour(button.findColour(ToggleButton::textColourId));
        g.setFont(this->font);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(button.getButtonText(), -2, 1, button.getWidth(), button.getHeight(), Justification::centred,
                         10);

    } else {
        float       fontSize = jmin(15.0f, button.getHeight() * 0.75f);
        const float tickWidth = fontSize * 1.1f;

        drawTickBox(g, button, 4.0f, (button.getHeight() - tickWidth) * 0.5f, tickWidth, tickWidth,
                    button.getToggleState(), button.isEnabled(), isMouseOverButton, isButtonDown);

        g.setColour(button.findColour(ToggleButton::textColourId));
        g.setFont(this->font);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        const int textX = (int)tickWidth + 5;

        g.drawFittedText(button.getButtonText(), textX, 0, button.getWidth() - (textX - 5), button.getHeight(),
                         Justification::centredLeft, 10);
    }
}

//==============================================================================
void GrisLookAndFeel::drawTabButton(TabBarButton & button, Graphics & g, bool isMouseOver, bool isMouseDown)
{
    const juce::Rectangle<int> activeArea(button.getActiveArea());
    activeArea.withHeight(18);
    const TabbedButtonBar::Orientation o = button.getTabbedButtonBar().getOrientation();
    const juce::Colour                 bkg(button.getTabBackgroundColour());

    if (button.getToggleState()) {
        g.setColour(bkg);
    } else {
        g.setColour(bkg.brighter(0.1f));
    }

    g.fillRect(activeArea);

    g.setColour(this->winBackGroundAndFieldColour);

    juce::Rectangle<int> r(activeArea);
    if (o != TabbedButtonBar::TabsAtTop)
        g.fillRect(r.removeFromBottom(1));
    if (o != TabbedButtonBar::TabsAtRight)
        g.fillRect(r.removeFromLeft(1));
    if (o != TabbedButtonBar::TabsAtLeft)
        g.fillRect(r.removeFromRight(1));

    const float                  alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
    juce::Colour                 col = (bkg.contrasting().withMultipliedAlpha(alpha));
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
                                          float                length,
                                          float                depth,
                                          juce::Colour const   colour,
                                          TextLayout &         textLayout) const
{
    Font font(this->font);
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
                                       int        x,
                                       int        y,
                                       int        width,
                                       int        height,
                                       float      sliderPos,
                                       float      rotaryStartAngle,
                                       float      rotaryEndAngle,
                                       Slider &   slider)
{
    const float radius = jmin(width / 2, height / 2) - 2.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = (y + height * 0.5f) + 6.0f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const bool  isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();

    if (slider.isEnabled()) {
        // slider.findColour (Slider::rotarySliderFillColourId).withAlpha (isMouseOver ? 0.7f : 1.0f)
        if (isMouseOver) {
            g.setColour(this->onColorOver);
        } else {
            g.setColour(this->onColor);
        }
    } else {
        g.setColour(this->offColor);
    }
    Path filledArc;
    filledArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, angle, 0.0);
    g.fillPath(filledArc);
    const float lineThickness = jmin(15.0f, jmin(width, height) * 0.45f) * 0.1f;
    Path        outlineArc;
    outlineArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, 0.0);
    g.strokePath(outlineArc, PathStrokeType(lineThickness));
}

void SmallGrisLookAndFeel::drawToggleButton(Graphics &     g,
                                            ToggleButton & button,
                                            bool           isMouseOverButton,
                                            bool           isButtonDown)
{
    if (button.hasKeyboardFocus(true)) {
        g.setColour(button.findColour(TextEditor::focusedOutlineColourId));
    }

    if (button.getButtonText().length() == 1) {
        drawTickBox(g, button, 0, 0, button.getWidth(), button.getHeight(), button.getToggleState(), button.isEnabled(),
                    isMouseOverButton, isButtonDown);
        g.setColour(button.findColour(ToggleButton::textColourId));
        g.setFont(this->smallFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(button.getButtonText(), -2, 1, button.getWidth(), button.getHeight(), Justification::centred,
                         10);

    } else {
        float       fontSize = jmin(15.0f, button.getHeight() * 0.75f);
        const float tickWidth = fontSize * 1.1f;

        drawTickBox(g, button, 4.0f, (button.getHeight() - tickWidth) * 0.5f, tickWidth, tickWidth,
                    button.getToggleState(), button.isEnabled(), isMouseOverButton, isButtonDown);

        g.setColour(button.findColour(ToggleButton::textColourId));
        g.setFont(this->smallFont);

        if (!button.isEnabled())
            g.setOpacity(0.5f);

        const int textX = (int)tickWidth + 5;

        g.drawFittedText(button.getButtonText(), textX, 0, button.getWidth() - (textX - 5), button.getHeight(),
                         Justification::centredLeft, 10);
    }
}