/*
 This file is part of ServerGris.
 
 Developers: Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "WinControl.h"
#include "MainComponent.h"
#include "Input.h"

static const float RadiusMax      = 2.f;
static const float SourceRadius   = 10.f;
static const float SourceDiameter = SourceRadius * 2.f;

typedef Point<float> FPoint;

static float DegreeToRadian(float degree) {
    return ((degree * M_PI ) / 180.0f);
}

static FPoint DegreeToXy(FPoint p, int FieldWidth) {
    float x,y;
    x = -((FieldWidth - SourceDiameter) / 2) * sinf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    y = -((FieldWidth - SourceDiameter) / 2) * cosf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    return FPoint(x, y);
}

static FPoint GetSourceAzimElev(FPoint pXY, bool bUseCosElev = false) {
    // Calculate azim in range [0,1], and negate it because zirkonium wants -1 on right side.
    float fAzim = -atan2f(pXY.x, pXY.y) / M_PI;
    
    // Calculate xy distance from origin, and clamp it to 2 (ie ignore outside of circle).
    float hypo = hypotf(pXY.x, pXY.y);
    if (hypo > RadiusMax) {
        hypo = RadiusMax;
    }
    
    float fElev;
    if (bUseCosElev) {
        fElev = acosf(hypo/RadiusMax);   // fElev is elevation in radian, [0,pi/2)
        fElev /= (M_PI/2.f);             // making range [0,1]
        fElev /= 2.f;                    // making range [0,.5] because that's what the zirkonium wants
    } else {
        fElev = (RadiusMax-hypo)/4.0f;
    }
    
    return FPoint(fAzim, fElev);
}

WinControl::WinControl(const String& name, Colour backgroundColour, int buttonsNeeded,
                       MainContentComponent *parent, GrisLookAndFeel *feel) : DocumentWindow (name, backgroundColour, buttonsNeeded) {
    this->mainParent = parent;
    this->grisFeel = feel;
}

WinControl::~WinControl() {
    this->mainParent->destroyWinControl();
}

void WinControl::setTimerHz(int hz) {
    this->stopTimer();
    this->startTimerHz(hz);
}

void WinControl::timerCallback() {
    this->repaint();
}

void WinControl::paint (Graphics& g) {
    const int fieldWH = getWidth();     // Same as getHeight()
    const int fieldCenter = fieldWH / 2;
    const int realW = (fieldWH - SourceDiameter);
    float w, x;

    g.fillAll(this->grisFeel->getWinBackgroundColour());

    // Draw line and light circle.
    g.setColour(this->grisFeel->getLightColour().withBrightness(0.5));
    w = realW / 1.3f;
    x = (fieldWH - w) / 2.0f;
    g.drawEllipse(x, x, w, w, 1);
    w = realW / 4.0f;
    x = (fieldWH - w) / 2.0f;
    g.drawEllipse(x, x, w, w, 1);
    
    w = realW;
    x = (fieldWH - w) / 2.0f;
    float r = (w / 2) * 0.296f;

    g.drawLine(x+r, x+r, (w+x)-r, (w+x)-r);
    g.drawLine(x+r, (w+x)-r, (w+x)-r , x+r);
    g.drawLine(x, fieldWH/2, w+x , fieldWH/2);
    g.drawLine(fieldWH/2, x, fieldWH/2 , w+x);
        
    // Draw big background circle.
    g.setColour(this->grisFeel->getLightColour());
    g.drawEllipse(x, x, w, w, 1);
    
    // Draw little background circle.
    w = realW / 2.0f;
    x = (fieldWH - w) / 2.0f;
    g.drawEllipse(x, x, w, w, 1);
    
    // Draw fill center cirlce.
    g.setColour(this->grisFeel->getWinBackgroundColour());
    w = realW / 4.0f;
    w -= 2;
    x = (fieldWH - w) / 2.0f;
    g.fillEllipse(x, x, w, w);
    
    g.setFont(this->grisFeel->getFont());
    g.setColour(this->grisFeel->getLightColour());
    g.drawText("0",   fieldCenter, 10,               SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
    g.drawText("90",  realW-10,    (fieldWH-4)/2.0f, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
    g.drawText("180", fieldCenter, realW-6,          SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
    g.drawText("270", 14,          (fieldWH-4)/2.0f, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
    
    // Draw translucid circles (mode).
    const int maxDrawSource = this->mainParent->getListSourceInput().size();

    for(int i = 0; i < maxDrawSource; ++i) {
        drawAzimElevSource(g, this->mainParent->getListSourceInput().at(i), fieldWH, fieldCenter);
    }

    String stringVal;
    w = (fieldWH - SourceDiameter);
    for (int i = 0; i < maxDrawSource; ++i) {
        Input *it = this->mainParent->getListSourceInput().at(i);

        if (it->getGain() == -1.0) { continue; }

        FPoint sourceP = FPoint(it->getCenter().z, it->getCenter().x) / 5.0f;
        sourceP.x = ((w / 2.0f) + ((w / 4.0f) * sourceP.x));
        sourceP.y = ((w / 2.0f) - ((w / 4.0f) * sourceP.y));
        
        g.setColour(it->getColorJWithAlpha());
        g.fillEllipse(sourceP.x , sourceP.y , SourceDiameter, SourceDiameter);
        
        stringVal.clear();
        stringVal << it->getId();
        
        g.setColour(Colours::black.withAlpha(it->getAlpha()));
        int tx = sourceP.x;
        int ty = sourceP.y;
        g.drawText(stringVal, tx+6 , ty+1, SourceDiameter+10, SourceDiameter, Justification(Justification::centredLeft), false);
        g.setColour(Colours::white.withAlpha(it->getAlpha()));
        g.drawText(stringVal, tx+5, ty, SourceDiameter+10, SourceDiameter, Justification(Justification::centredLeft), false);
    }
}

void WinControl::resized() {
    const int fieldWH = min(getWidth(), getHeight());
    this->setSize(fieldWH, fieldWH);
}

void WinControl::closeButtonPressed() {
    this->mainParent->winControlRect.setBounds(getScreenX(), getScreenY(), getWidth(), getHeight());
    delete this;
}

void WinControl::drawAzimElevSource(Graphics &g, Input *it, const int fieldWH, const int fieldCenter) {
    float HRAzimSpan = 180.0f *(it->getAziMuthSpan());  // In zirkosc, this is [0,360]
    float HRElevSpan = 180.0f *(it->getZenithSpan());  // In zirkosc, this is [0,90]

    if ((HRAzimSpan < 0.002f && HRElevSpan < 0.002f) || !this->mainParent->isSpanShown) {
        return;
    }
    
    Colour colorS = it->getColorJ();
    g.setColour(it->getColorJWithAlpha());
    
    FPoint sourceP = FPoint(it->getCenter().z, it->getCenter().x) / 5.0f;
    FPoint azimElev = GetSourceAzimElev(sourceP, true);
    
    float HRAzim = azimElev.x * 180.0f;    // In zirkosc [-180,180]
    float HRElev = azimElev.y * 180.0f;    // In zirkosc [0,89.9999]
    
    
    // Calculate max and min elevation in degrees.
    FPoint maxElev = {HRAzim, HRElev + HRElevSpan / 2.0f};
    FPoint minElev = {HRAzim, HRElev - HRElevSpan / 2.0f};
    
    if (minElev.y < 0) {
        maxElev.y = (maxElev.y - minElev.y);
        minElev.y = 0.0f;
    }
    
    // Convert max min elev to xy.
    FPoint screenMaxElev = DegreeToXy(maxElev, fieldWH);
    FPoint screenMinElev = DegreeToXy(minElev, fieldWH);
    
    // Form minmax elev, calculate minmax radius.
    float maxRadius = sqrtf(screenMaxElev.x * screenMaxElev.x + screenMaxElev.y * screenMaxElev.y);
    float minRadius = sqrtf(screenMinElev.x * screenMinElev.x + screenMinElev.y * screenMinElev.y);
    
    // Drawing the path for spanning.
    Path myPath;
    myPath.startNewSubPath(fieldCenter+screenMaxElev.x, fieldCenter+screenMaxElev.y);
    // Half first arc center.
    myPath.addCentredArc(fieldCenter, fieldCenter, minRadius, minRadius, 0.0, DegreeToRadian(-HRAzim), DegreeToRadian(-HRAzim + HRAzimSpan / 2));

    // If we are over the top of the dome we draw the adjacent angle.
    if (maxElev.getY() > 90.f) {
        myPath.addCentredArc(fieldCenter, fieldCenter, maxRadius, maxRadius, 0.0,
                             M_PI + DegreeToRadian(-HRAzim + HRAzimSpan / 2),
                             M_PI + DegreeToRadian(-HRAzim - HRAzimSpan / 2));
    } else {
        myPath.addCentredArc(fieldCenter, fieldCenter, maxRadius, maxRadius, 0.0,
                             DegreeToRadian(-HRAzim + HRAzimSpan / 2),
                             DegreeToRadian(-HRAzim - HRAzimSpan / 2));
    }
    myPath.addCentredArc(fieldCenter, fieldCenter, minRadius, minRadius, 0.0, 
                         DegreeToRadian(-HRAzim - HRAzimSpan / 2), DegreeToRadian(-HRAzim));
    
    myPath.closeSubPath();
    
    g.setColour(colorS.withAlpha(it->getAlpha() * 0.2f));
    g.fillPath(myPath);
    
    g.setColour(colorS.withAlpha(it->getAlpha() * 0.6f));
    g.strokePath(myPath, PathStrokeType(0.5));
}
