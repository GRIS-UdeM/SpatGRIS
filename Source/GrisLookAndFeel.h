/*
 ==============================================================================
 
 This file is part of the JUCE library.
 Copyright (c) 2015 - ROLI Ltd.
 
 Permission is granted to use this software under the terms of either:
 a) the GPL v2 (or any later version)
 b) the Affero GPL v3
 
 Details of these licenses can be found at: www.gnu.org/licenses
 
 JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 
 ------------------------------------------------------------------------------
 
 To release a closed-source product which uses JUCE, commercial licenses are
 available: visit www.juce.com for more information.
 
 ==============================================================================
 */

#ifndef GRISLOOKANDFEEL_H_INCLUDED
#define GRISLOOKANDFEEL_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/** Custom Look And Feel subclasss.
 
 Simply override the methods you need to, anything else will be inherited from the base class.
 It's a good idea not to hard code your colours, use the findColour method along with appropriate
 ColourIds so you can set these on a per-component basis.
 */




class GrisLookAndFeel    : public LookAndFeel_V3 {
private:
    
    float fontSize;

    Font  font = Font(juce::CustomTypeface::createSystemTypefaceFor(BinaryData::SinkinSans400Regular_otf, (size_t) BinaryData::SinkinSans400Regular_otfSize));

    Colour backGroundAndFieldColour, winBackGroundAndFieldColour;
    Colour lightColour, darkColour, greyColour, editBgcolor;
    Colour onColor, onColorOver, onColorDown, offColor, greenColor, redColor;
    
public:
    GrisLookAndFeel(){
        
        this->backGroundAndFieldColour      = Colour::fromRGB(75, 75, 75);  //Colours::darkgrey;
        this->winBackGroundAndFieldColour   = Colour::fromRGB(46, 46, 46);
        
        this->lightColour               = Colour::fromRGB(235, 245, 250);   //Colours::whitesmoke;
        this->darkColour                = Colour::fromRGB(15,  10,  5);       //Colours::black;
        this->greyColour                = Colour::fromRGB(120, 120, 120);   //Colours::grey;
        this->editBgcolor               = Colour::fromRGB(172, 172, 172);
        
        this->onColor                   = Colour::fromRGB(255, 165, 25);
        this->onColorOver               = Colour::fromRGB(255, 184, 75);
        this->onColorDown               = Colour::fromRGB(222, 144, 22);
        this->offColor                  = Colour::fromRGB(56,  56,  56);
       
        this->greenColor                = Colour::fromRGB(56,  156,  56);
        this->redColor                  = Colour::fromRGB(220,  48,  35);
        
        setColour(PopupMenu::highlightedBackgroundColourId, this->onColor);
        setColour(TextEditor::backgroundColourId, this->editBgcolor);
        setColour(TextEditor::highlightColourId, this->onColor);
        setColour(TextEditor::shadowColourId, this->editBgcolor);
        
        setColour(TextButton::buttonColourId, this->editBgcolor);
        
        setColour(ComboBox::backgroundColourId, this->editBgcolor);
        setColour(ComboBox::outlineColourId, this->editBgcolor);
        
        setColour(Slider::thumbColourId, this->lightColour);
        setColour(Slider::rotarySliderFillColourId, this->onColor);
        setColour(Slider::trackColourId, this->darkColour);
        setColour(Slider::textBoxBackgroundColourId, this->editBgcolor);
        setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        
        setColour(TooltipWindow::ColourIds::backgroundColourId,this->backGroundAndFieldColour.withBrightness(0.8));
        setColour(TooltipWindow::ColourIds::outlineColourId, this->backGroundAndFieldColour.withBrightness(0.8));
        
        setColour(AlertWindow::backgroundColourId, this->winBackGroundAndFieldColour);
        setColour(AlertWindow::outlineColourId, this->onColor);
        setColour(AlertWindow::textColourId, this->lightColour);
        
#if WIN32
        this->fontSize = 18.f;
#else
        this->fontSize = 10.f;
#endif
        this->font.setHeight(this->fontSize);
       
    }
    
    Font getFont(){
        return this->font;
    }
    Font getLabelFont (Label & label) override{
        return this->font;
    }
    Font getComboBoxFont (ComboBox & comboBox) override{
        return this->font;
    }
    Font getTextButtonFont (TextButton &, int buttonHeight) override{
        return this->font;
    }
    Font getMenuBarFont	(MenuBarComponent &, int itemIndex, const String & itemText) override{
        return this->font;
    }
    
    Colour getWinBackgroundColour(){
        return this->winBackGroundAndFieldColour;
    }

    Colour getBackgroundColour(){
        return this->backGroundAndFieldColour;
    }
    
    Colour getFieldColour(){
        return this->backGroundAndFieldColour;
    }
    
    Colour getFontColour(){
        return this->lightColour;
    }
    
    Colour getScrollBarColour(){
        return this->greyColour;
    }
    
    Colour getDarkColour(){
        return this->darkColour;
    }
    
    Colour getLightColour(){
        return this->lightColour;
    }
    
    Colour getOnColour(){
        return this->onColor;
    }
    Colour getOffColour(){
        return this->offColor;
    }
    Colour getGreenColour(){
        return this->greenColor;
    }
    Colour getRedColour(){
        return this->redColor;
    }
    
    //https://github.com/audioplastic/Juce-look-and-feel-examples/blob/master/JuceLibraryCode/modules/juce_gui_basics/lookandfeel/juce_LookAndFeel.cpp
    
    void drawComboBox(Graphics& g,int width, int height,bool isButtonDown,int buttonX,int buttonY,int buttonW,int buttonH,ComboBox & box) override
    {
        box.setColour(ColourSelector::backgroundColourId, this->onColor);
    
        g.fillAll (this->editBgcolor);//box.findColour (ComboBox::backgroundColourId))
        
        const Colour buttonColour (this->darkColour);//box.findColour (ComboBox::buttonColourId)
        const float arrowX = 0.3f;
        const float arrowH = 0.2f;
        
        Path p;
        p.addTriangle (buttonX + buttonW * 0.5f,            buttonY + buttonH * (0.45f - arrowH),
                       buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.45f,
                       buttonX + buttonW * arrowX,          buttonY + buttonH * 0.45f);
        
        p.addTriangle (buttonX + buttonW * 0.5f,            buttonY + buttonH * (0.55f + arrowH),
                       buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.55f,
                       buttonX + buttonW * arrowX,          buttonY + buttonH * 0.55f);
        
        g.setColour (this->darkColour.withMultipliedAlpha (box.isEnabled() ? 1.0f : 0.3f));//box.findColour (ComboBox::arrowColourId)
        g.fillPath (p);
    }
    
    void drawRoundThumb (Graphics& g, const float x, const float y, const float diameter, const Colour& colour, float outlineThickness) {
        const juce::Rectangle<float> a (x, y, diameter, diameter);
        const float halfThickness = outlineThickness * 0.5f;
        
        Path p;
        p.addEllipse (x + halfThickness, y + halfThickness, diameter - outlineThickness, diameter - outlineThickness);
        
        const DropShadow ds (this->darkColour, 1, Point<int> (0, 0));
        ds.drawForPath (g, p);
        
        g.setColour (colour);
        g.fillPath (p);
        
        g.setColour (colour.brighter());
        g.strokePath (p, PathStrokeType (outlineThickness));
    }
    
    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override {
        
        const float width  = button.getWidth() - 1.0f;
        const float height = button.getHeight() - 1.0f;
        const float cornerSize = jmin (15.0f, jmin (width, height) * 0.45f);
        const float lineThickness = cornerSize * 0.1f;
        const float halfThickness = lineThickness * 0.5f;
        Path outline;
        outline.addRectangle(0.5f + halfThickness, 0.5f + halfThickness, width - lineThickness, height - lineThickness);
        g.setColour (button.findColour(TextButton::buttonColourId));
        if (isButtonDown || isMouseOverButton){
            g.setColour (this->onColorOver);
        }
        if ( button.getToggleState()) {
            g.setColour (this->onColor);//outlineColour
        }
        if(button.isEnabled() && button.isMouseButtonDown()){
            g.setColour (this->onColorDown);
        }
        g.fillPath (outline);
    }
    
    
    void drawTickBox (Graphics& g, Component& component, float x, float y, float w, float h, bool ticked, bool isEnabled, bool isMouseOverButton, bool isButtonDown) override {
        const float boxSize = w * 0.8f;
        const Rectangle<float> r (x, y + (h - boxSize) * 0.5f, boxSize, boxSize);

        if (ticked) {
            Colour colour = this->onColor;
            
            if(component.isMouseOver()){
                colour = this->onColorOver;
            }
            
            if(!component.isEnabled()){
                colour = this->onColor.withBrightness(0.3f);
            }
            g.setColour (colour);
            g.fillRect (r);

        }else{
            Colour colour = this->offColor;
            if(!component.isEnabled()){
                colour = this->offColor.withBrightness(0.3f);
            }
            g.setColour (colour);
            g.fillRect (r);
        }
        
        if(component.isEnabled() && component.isMouseButtonDown()){
            g.setColour (this->onColorDown);
            g.fillRect (r);
        }
    }
    
    
    void drawLinearSliderThumb (Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override {
        const float sliderRadius = (float) (getSliderThumbRadius (slider) - 2);
        float kx, ky;
        
        if (style == Slider::LinearVertical) {
            kx = x + width * 0.5f;
            ky = sliderPos;
        } else {
            kx = sliderPos;
            ky = y + height * 0.5f;
        }
        const Rectangle<float> r (kx - (sliderRadius/2.0f), ky- sliderRadius , 6, height*2.0f);

        if(slider.isEnabled()){
            Colour colour = this->onColor;
            
            if(slider.isMouseOver()){
                colour = this->onColorOver;
            }
            if(slider.isMouseButtonDown()){
                colour = this->onColorDown;
            }
            g.setColour (colour);
            g.fillRect (r);
        }else{
            g.setColour (this->offColor);
            g.fillRect (r);
        }
        
    }
    
    void drawLinearSlider (Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override {
        drawLinearSliderBackground (g, x, y, width, height+2, sliderPos, minSliderPos, maxSliderPos, style, slider);
        drawLinearSliderThumb (g, x, y, width, height+2, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
    
    void drawLinearSliderBackground (Graphics& g, int x, int y, int width, int height, float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/, const Slider::SliderStyle /*style*/, Slider& slider) override {
        const float sliderRadius = getSliderThumbRadius (slider) - 5.0f;
        Path on, off;

        if (slider.isHorizontal()) {
            const float iy = y + height * 0.5f - sliderRadius * 0.5f;
            juce::Rectangle<float> r (x - sliderRadius * 0.5f, iy, width + sliderRadius, sliderRadius);
            const float onW = r.getWidth() * ((float) slider.valueToProportionOfLength (slider.getValue()));
            on.addRectangle (r.removeFromLeft (onW));
            off.addRectangle (r);
        } else {
            const float ix = x + width * 0.5f - sliderRadius * 0.5f;
            juce::Rectangle<float> r (ix, y - sliderRadius * 0.5f, sliderRadius, height + sliderRadius);
            const float onH = r.getHeight() * ((float) slider.valueToProportionOfLength (slider.getValue()));
            on.addRectangle (r.removeFromBottom (onH));
            off.addRectangle (r);
        }
        
        if (slider.isEnabled()){
            g.setColour (slider.findColour (Slider::rotarySliderFillColourId));
            g.fillPath (on);
            g.setColour (slider.findColour (Slider::trackColourId));
            g.fillPath (off);
        }else{
            g.setColour (this->offColor);
            g.fillPath (on);
            g.fillPath (off);
        }
       
    }
    
    void fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& t) override {
        g.setColour(this->editBgcolor);
        g.fillAll();
    }
    
    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& t) override {
        if(t.hasKeyboardFocus(true))
        {
            g.setColour(this->onColor);
            g.drawRect (0, 0, width, height);
        }
        
       
    }
    
    void drawToggleButton (Graphics& g, ToggleButton& button, bool isMouseOverButton, bool isButtonDown) override {
        if (button.hasKeyboardFocus (true))
        {
            g.setColour (button.findColour (TextEditor::focusedOutlineColourId));
            //g.drawRect (0, 0, button.getWidth(), button.getHeight());
        }

        if(button.getButtonText().length()==1){
            drawTickBox (g, button, 0, 0, button.getWidth(), button.getHeight(),
                         button.getToggleState(), button.isEnabled(),isMouseOverButton,isButtonDown);
            g.setColour(button.findColour (ToggleButton::textColourId));
            g.setFont(this->font);
            
            if (! button.isEnabled())
                g.setOpacity (0.5f);
                
            
            g.drawFittedText (button.getButtonText(),-2, 1,button.getWidth() , button.getHeight(),
                              Justification::centred, 10);
            
            
        }else{
            float fontSize = jmin (15.0f, button.getHeight() * 0.75f);
            const float tickWidth = fontSize * 1.1f;
            
            drawTickBox (g, button, 4.0f, (button.getHeight() - tickWidth) * 0.5f,
                         tickWidth, tickWidth,
                         button.getToggleState(),
                         button.isEnabled(),
                         isMouseOverButton,
                         isButtonDown);
            
            g.setColour(button.findColour (ToggleButton::textColourId));
            g.setFont(this->font);
            
            if (! button.isEnabled())
                g.setOpacity (0.5f);
                
                const int textX = (int) tickWidth + 5;
            
            g.drawFittedText (button.getButtonText(),
                              textX, 0,
                              button.getWidth() - (textX-5) , button.getHeight(),
                              Justification::centredLeft, 10);
        }
    }
    
    void drawTabButton (TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown) override{
        const Rectangle<int> activeArea (button.getActiveArea());
        activeArea.withHeight(18);
        const TabbedButtonBar::Orientation o = button.getTabbedButtonBar().getOrientation();
        const Colour bkg (button.getTabBackgroundColour());
        
        if (button.getToggleState())
        {
            g.setColour (bkg);
        }
        else
        {
            g.setColour (bkg.brighter (0.1f));
        }
        
        g.fillRect (activeArea);
        
        g.setColour (this->winBackGroundAndFieldColour);
        
        Rectangle<int> r (activeArea);
        if (o != TabbedButtonBar::TabsAtTop)      g.fillRect (r.removeFromBottom (1));
            if (o != TabbedButtonBar::TabsAtRight)    g.fillRect (r.removeFromLeft (1));
                if (o != TabbedButtonBar::TabsAtLeft)     g.fillRect (r.removeFromRight (1));
        
        const float alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
        Colour col = (bkg.contrasting().withMultipliedAlpha (alpha));
        const Rectangle<float> area (button.getTextArea().toFloat());
        
        float length = area.getWidth();
        float depth  = area.getHeight();
        
        if (button.getTabbedButtonBar().isVertical())
            std::swap (length, depth);
            
        TextLayout textLayout;
        createTabTextLayout (button, length, depth, col, textLayout);
        textLayout.draw (g, Rectangle<float> (length, depth));
        /*
        Rectangle<int> activeArea (button.getActiveArea());

        //const TabbedButtonBar::Orientation o = button.getTabbedButtonBar().getOrientation();
        const Colour bkg (button.getTabBackgroundColour());
        
        if (button.getToggleState())
        {
            g.setColour (bkg);
        }
        else
        {
            g.setColour (bkg.brighter (0.1f));
        }
        
        g.fillRect (activeArea);
        g.setColour (this->winBackGroundAndFieldColour);
        
        Rectangle<int> r (activeArea);
        const float alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
        Colour col = (bkg.contrasting().withMultipliedAlpha (alpha));
        
       
        const Rectangle<float> area (button.getTextArea().toFloat());
        
        float length = area.getWidth();
        float depth  = area.getHeight();
        
        if (button.getTabbedButtonBar().isVertical())
            std::swap (length, depth);
        
        TextLayout textLayout;
        createTabTextLayout (button, length, depth, col, textLayout);
        
        AffineTransform t;
        g.addTransform (t);
        textLayout.draw (g, Rectangle<float> (length, depth));*/
    }
    
    void createTabTextLayout (const TabBarButton& button, float length, float depth, Colour colour, TextLayout& textLayout)
    {
        Font font (this->font);
#if WIN32
        font.setHeight(depth * 0.60f);
#else

        font.setHeight(depth * 0.35f);
#endif
        font.setUnderline (button.hasKeyboardFocus (false));
        
        AttributedString s;
        s.setJustification (Justification::centred);
        s.append (button.getButtonText().trim(), font, colour);
        
        textLayout.createLayout (s, length);
    }
    

    
//    void drawTabButtonText (TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown) override
//    {
//        const Rectangle<float> area (button.getTextArea().toFloat());
//        
//        float length = area.getWidth();
//        float depth  = area.getHeight();
//        
//        if (button.getTabbedButtonBar().isVertical())
//            std::swap (length, depth);
//        
//        Font font (this->font);
//        font.setHeight(depth * 0.35f);
//        font.setUnderline (button.hasKeyboardFocus (false));
//        
//        AffineTransform t;
//        
//        switch (button.getTabbedButtonBar().getOrientation())
//        {
//            case TabbedButtonBar::TabsAtLeft:   t = t.rotated (float_Pi * -0.5f).translated (area.getX(), area.getBottom()); break;
//            case TabbedButtonBar::TabsAtRight:  t = t.rotated (float_Pi *  0.5f).translated (area.getRight(), area.getY()); break;
//            case TabbedButtonBar::TabsAtTop:
//            case TabbedButtonBar::TabsAtBottom: t = t.translated (area.getX(), area.getY()); break;
//            default:                            jassertfalse; break;
//        }
//        
//        Colour col;
//        
//        if (button.isFrontTab() && (button.isColourSpecified (TabbedButtonBar::frontTextColourId)
//                                    || isColourSpecified (TabbedButtonBar::frontTextColourId)))
//            col = findColour (TabbedButtonBar::frontTextColourId);
//        else if (button.isColourSpecified (TabbedButtonBar::tabTextColourId)
//                 || isColourSpecified (TabbedButtonBar::tabTextColourId))
//            col = findColour (TabbedButtonBar::tabTextColourId);
//        else
//            col = button.getTabBackgroundColour().contrasting();
//        
//        const float alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
//        
//        g.setColour (col.withMultipliedAlpha (alpha));
//        g.setFont (font);
//        g.addTransform (t);
//        
//        g.drawFittedText (button.getButtonText().trim(),
//                          0, 0, (int) length, (int) depth,
//                          Justification::centred,
//                          jmax (1, ((int) depth) / 12));
//    }
    

        
        //    //we don't use those, so far
    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, Slider& slider) override
    {
        const float radius = jmin (width / 2, height / 2) - 2.0f;
        const float centreX = x + width * 0.5f;
        const float centreY = (y + height * 0.5f)+6.0f;
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius * 2.0f;
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const bool isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();
        
        if (slider.isEnabled()){
            //slider.findColour (Slider::rotarySliderFillColourId).withAlpha (isMouseOver ? 0.7f : 1.0f)
            if(isMouseOver){
                g.setColour (this->onColorOver);
            }
            else{
                g.setColour (this->onColor);
            }
        }
        else{
                g.setColour (this->offColor);
        }
        Path filledArc;
        filledArc.addPieSegment (rx, ry, rw, rw, rotaryStartAngle, angle, 0.0);
        g.fillPath (filledArc);
        const float lineThickness = jmin (15.0f, jmin (width, height) * 0.45f) * 0.1f;
        Path outlineArc;
        outlineArc.addPieSegment (rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, 0.0);
        g.strokePath (outlineArc, PathStrokeType (lineThickness));
    }
};

#endif

