/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "FlatViewWindow.h"

#include "Input.h"
#include "MainComponent.h"

static const float RadiusMax = 2.f;
static const float SourceRadius = 10.f;
static const float SourceDiameter = SourceRadius * 2.f;

typedef juce::Point<float> FPoint;

//==============================================================================
static float DegreeToRadian(float degree)
{
    return ((degree * M_PI) / 180.0f);
}

//==============================================================================
static FPoint DegreeToXy(FPoint p, int FieldWidth)
{
    float x, y;
    x = -((FieldWidth - SourceDiameter) / 2) * sinf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    y = -((FieldWidth - SourceDiameter) / 2) * cosf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    return FPoint(x, y);
}

//==============================================================================
static FPoint GetSourceAzimElev(FPoint pXY, bool bUseCosElev = false)
{
    // Calculate azim in range [0,1], and negate it because zirkonium wants -1 on right side.
    float fAzim = -atan2f(pXY.x, pXY.y) / M_PI;

    // Calculate xy distance from origin, and clamp it to 2 (ie ignore outside of circle).
    float hypo = hypotf(pXY.x, pXY.y);
    if (hypo > RadiusMax) {
        hypo = RadiusMax;
    }

    float fElev;
    if (bUseCosElev) {
        fElev = acosf(hypo / RadiusMax); // fElev is elevation in radian, [0,pi/2)
        fElev /= (M_PI / 2.f);           // making range [0,1]
        fElev /= 2.f;                    // making range [0,.5] because that's what the zirkonium wants
    } else {
        fElev = (RadiusMax - hypo) / 4.0f;
    }

    return FPoint(fAzim, fElev);
}

//==============================================================================
FlatViewWindow::FlatViewWindow(MainContentComponent & parent, GrisLookAndFeel & feel)
    : DocumentWindow("2D View", feel.getBackgroundColour(), juce::DocumentWindow::allButtons)
    , mMainContentComponent(parent)
    , mLookAndFeel(feel)
{
    this->startTimerHz(24);
}

//==============================================================================
FlatViewWindow::~FlatViewWindow()
{
    this->mMainContentComponent.closeFlatViewWindow();
}

//==============================================================================
void FlatViewWindow::drawFieldBackground(juce::Graphics & g, const int fieldWH) const
{
    const int realW = (fieldWH - SourceDiameter);

    if (this->mMainContentComponent.getModeSelected() == LBAP) {
        float offset, size;
        // Draw light background squares.
        g.setColour(this->mLookAndFeel.getLightColour());
        offset = fieldWH * 0.4;
        size = fieldWH * 0.2;
        g.drawRect(offset, offset, size, size);
        offset = fieldWH * 0.2;
        size = fieldWH * 0.6;
        g.drawRect(offset, offset, size, size);
        g.drawRect(10, 10, fieldWH - 20, fieldWH - 20);
        // Draw shaded background squares.
        g.setColour(this->mLookAndFeel.getLightColour().withBrightness(0.5));
        offset = fieldWH * 0.3;
        size = fieldWH * 0.4;
        g.drawRect(offset, offset, size, size);
        offset = fieldWH * 0.1;
        size = fieldWH * 0.8;
        g.drawRect(offset, offset, size, size);
        // Draw lines.
        float center = (fieldWH - realW) / 2.0f;
        g.drawLine(fieldWH * 0.2, fieldWH * 0.2, fieldWH - fieldWH * 0.2, fieldWH - fieldWH * 0.2);
        g.drawLine(fieldWH * 0.2, fieldWH - fieldWH * 0.2, fieldWH - fieldWH * 0.2, fieldWH * 0.2);
        g.drawLine(center, fieldWH / 2, realW + center, fieldWH / 2);
        g.drawLine(fieldWH / 2, center, fieldWH / 2, realW + center);
    } else {
        float w, x;
        // Draw line and light circle.
        g.setColour(this->mLookAndFeel.getLightColour().withBrightness(0.5));
        w = realW / 1.3f;
        x = (fieldWH - w) / 2.0f;
        g.drawEllipse(x, x, w, w, 1);
        w = realW / 4.0f;
        x = (fieldWH - w) / 2.0f;
        g.drawEllipse(x, x, w, w, 1);

        w = realW;
        x = (fieldWH - w) / 2.0f;
        float r = (w / 2) * 0.296f;

        g.drawLine(x + r, x + r, (w + x) - r, (w + x) - r);
        g.drawLine(x + r, (w + x) - r, (w + x) - r, x + r);
        g.drawLine(x, fieldWH / 2, w + x, fieldWH / 2);
        g.drawLine(fieldWH / 2, x, fieldWH / 2, w + x);

        // Draw big background circle.
        g.setColour(this->mLookAndFeel.getLightColour());
        g.drawEllipse(x, x, w, w, 1);

        // Draw little background circle.
        w = realW / 2.0f;
        x = (fieldWH - w) / 2.0f;
        g.drawEllipse(x, x, w, w, 1);

        // Draw fill center cirlce.
        g.setColour(this->mLookAndFeel.getWinBackgroundColour());
        w = realW / 4.0f;
        w -= 2;
        x = (fieldWH - w) / 2.0f;
        g.fillEllipse(x, x, w, w);
    }
}

//==============================================================================
void FlatViewWindow::paint(juce::Graphics & g)
{
    const int fieldWH = getWidth(); // Same as getHeight()
    const int fieldCenter = fieldWH / 2;
    const int realW = (fieldWH - SourceDiameter);

    g.fillAll(this->mLookAndFeel.getWinBackgroundColour());

    drawFieldBackground(g, fieldWH);

    g.setFont(this->mLookAndFeel.getFont());
    g.setColour(this->mLookAndFeel.getLightColour());
    g.drawText("0",
               fieldCenter,
               10,
               SourceDiameter,
               SourceDiameter,
               juce::Justification(juce::Justification::centred),
               false);
    g.drawText("90",
               realW - 10,
               (fieldWH - 4) / 2.0f,
               SourceDiameter,
               SourceDiameter,
               juce::Justification(juce::Justification::centred),
               false);
    g.drawText("180",
               fieldCenter,
               realW - 6,
               SourceDiameter,
               SourceDiameter,
               juce::Justification(juce::Justification::centred),
               false);
    g.drawText("270",
               14,
               (fieldWH - 4) / 2.0f,
               SourceDiameter,
               SourceDiameter,
               juce::Justification(juce::Justification::centred),
               false);

    // Draw sources.
    int maxDrawSource = (int)this->mMainContentComponent.getListSourceInput().size();
    for (int i = 0; i < maxDrawSource; ++i) {
        Input * it = this->mMainContentComponent.getListSourceInput().getUnchecked(i);
        if (it->getGain() == -1.0) {
            continue;
        }
        drawSource(g, this->mMainContentComponent.getListSourceInput().getUnchecked(i), fieldWH);
        drawSourceSpan(g, this->mMainContentComponent.getListSourceInput().getUnchecked(i), fieldWH, fieldCenter);
    }
}

//==============================================================================
void FlatViewWindow::drawSource(juce::Graphics & g, Input * it, const int fieldWH) const
{
    const int realW = (fieldWH - SourceDiameter);
    juce::String stringVal;
    FPoint sourceP;
    if (this->mMainContentComponent.getModeSelected() == LBAP) {
        sourceP = FPoint(it->getCenter().z * 0.6f, it->getCenter().x * 0.6f) / 5.0f;
    } else {
        sourceP = FPoint(it->getCenter().z, it->getCenter().x) / 5.0f;
    }
    sourceP.x = ((realW / 2.0f) + ((realW / 4.0f) * sourceP.x));
    sourceP.y = ((realW / 2.0f) - ((realW / 4.0f) * sourceP.y));
    sourceP.x = sourceP.x < 0 ? 0 : sourceP.x > fieldWH - SourceDiameter ? fieldWH - SourceDiameter : sourceP.x;
    sourceP.y = sourceP.y < 0 ? 0 : sourceP.y > fieldWH - SourceDiameter ? fieldWH - SourceDiameter : sourceP.y;

    g.setColour(it->getColorJWithAlpha());
    g.fillEllipse(sourceP.x, sourceP.y, SourceDiameter, SourceDiameter);

    stringVal.clear();
    stringVal << it->getId();

    g.setColour(juce::Colours::black.withAlpha(it->getAlpha()));
    int tx = sourceP.x;
    int ty = sourceP.y;
    g.drawText(stringVal,
               tx + 6,
               ty + 1,
               SourceDiameter + 10,
               SourceDiameter,
               juce::Justification(juce::Justification::centredLeft),
               false);
    g.setColour(juce::Colours::white.withAlpha(it->getAlpha()));
    g.drawText(stringVal,
               tx + 5,
               ty,
               SourceDiameter + 10,
               SourceDiameter,
               juce::Justification(juce::Justification::centredLeft),
               false);
}

//==============================================================================
void FlatViewWindow::drawSourceSpan(juce::Graphics & g, Input * it, const int fieldWH, const int fieldCenter) const
{
    juce::Colour colorS = it->getColorJ();

    FPoint sourceP;
    if (this->mMainContentComponent.getModeSelected() == LBAP) {
        sourceP = FPoint(it->getCenter().z * 0.6f, it->getCenter().x * 0.6f) / 5.0f;
    } else {
        sourceP = FPoint(it->getCenter().z, it->getCenter().x) / 5.0f;
    }

    if (this->mMainContentComponent.getModeSelected() == LBAP) {
        int realW = (fieldWH - SourceDiameter);
        float azimuthSpan = fieldWH * (it->getAzimuthSpan() * 0.5f);
        float halfAzimuthSpan = azimuthSpan / 2.0f - SourceRadius;

        sourceP.x = ((realW / 2.0f) + ((realW / 4.0f) * sourceP.x));
        sourceP.y = ((realW / 2.0f) - ((realW / 4.0f) * sourceP.y));
        sourceP.x = sourceP.x < 0 ? 0 : sourceP.x > fieldWH - SourceDiameter ? fieldWH - SourceDiameter : sourceP.x;
        sourceP.y = sourceP.y < 0 ? 0 : sourceP.y > fieldWH - SourceDiameter ? fieldWH - SourceDiameter : sourceP.y;

        g.setColour(colorS.withAlpha(it->getAlpha() * 0.6f));
        g.drawEllipse(sourceP.x - halfAzimuthSpan, sourceP.y - halfAzimuthSpan, azimuthSpan, azimuthSpan, 1.5f);
        g.setColour(colorS.withAlpha(it->getAlpha() * 0.2f));
        g.fillEllipse(sourceP.x - halfAzimuthSpan, sourceP.y - halfAzimuthSpan, azimuthSpan, azimuthSpan);
    } else {
        float HRAzimSpan = 180.0f * (it->getAzimuthSpan()); // In zirkosc, this is [0,360]
        float HRElevSpan = 180.0f * (it->getZenithSpan());  // In zirkosc, this is [0,90]

        if ((HRAzimSpan < 0.002f && HRElevSpan < 0.002f) || !this->mMainContentComponent.isSpanShown()) {
            return;
        }

        FPoint azimElev = GetSourceAzimElev(sourceP, true);

        float HRAzim = azimElev.x * 180.0f; // In zirkosc [-180,180]
        float HRElev = azimElev.y * 180.0f; // In zirkosc [0,89.9999]

        // Calculate max and min elevation in degrees.
        FPoint maxElev = { HRAzim, HRElev + HRElevSpan / 2.0f };
        FPoint minElev = { HRAzim, HRElev - HRElevSpan / 2.0f };

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
        juce::Path myPath;
        myPath.startNewSubPath(fieldCenter + screenMaxElev.x, fieldCenter + screenMaxElev.y);
        // Half first arc center.
        myPath.addCentredArc(fieldCenter,
                             fieldCenter,
                             minRadius,
                             minRadius,
                             0.0,
                             DegreeToRadian(-HRAzim),
                             DegreeToRadian(-HRAzim + HRAzimSpan / 2));

        // If we are over the top of the dome we draw the adjacent angle.
        if (maxElev.getY() > 90.f) {
            myPath.addCentredArc(fieldCenter,
                                 fieldCenter,
                                 maxRadius,
                                 maxRadius,
                                 0.0,
                                 M_PI + DegreeToRadian(-HRAzim + HRAzimSpan / 2),
                                 M_PI + DegreeToRadian(-HRAzim - HRAzimSpan / 2));
        } else {
            myPath.addCentredArc(fieldCenter,
                                 fieldCenter,
                                 maxRadius,
                                 maxRadius,
                                 0.0,
                                 DegreeToRadian(-HRAzim + HRAzimSpan / 2),
                                 DegreeToRadian(-HRAzim - HRAzimSpan / 2));
        }
        myPath.addCentredArc(fieldCenter,
                             fieldCenter,
                             minRadius,
                             minRadius,
                             0.0,
                             DegreeToRadian(-HRAzim - HRAzimSpan / 2),
                             DegreeToRadian(-HRAzim));

        myPath.closeSubPath();

        g.setColour(colorS.withAlpha(it->getAlpha() * 0.2f));
        g.fillPath(myPath);

        g.setColour(colorS.withAlpha(it->getAlpha() * 0.6f));
        g.strokePath(myPath, juce::PathStrokeType(0.5));
    }
}

//==============================================================================
void FlatViewWindow::resized()
{
    const int fieldWH = std::min(getWidth(), getHeight());
    this->setSize(fieldWH, fieldWH);
}

//==============================================================================
void FlatViewWindow::closeButtonPressed()
{
    this->mMainContentComponent.closeFlatViewWindow();
}
