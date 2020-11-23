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

static float constexpr RADIUS_MAX = 2.0f;
static float constexpr SOURCE_RADIUS = 10.0f;
static float constexpr SOURCE_DIAMETER = SOURCE_RADIUS * 2.f;

//==============================================================================
static float degreeToRadian(float const degree)
{
    return ((degree * juce::MathConstants<float>::pi) / 180.0f);
}

//==============================================================================
static juce::Point<float> degreeToXy(juce::Point<float> const & p, int const fieldWidth)
{
    auto const fieldWidth_f{ static_cast<float>(fieldWidth) };
    auto const x{ -((fieldWidth_f - SOURCE_DIAMETER) / 2.0f) * std::sin(degreeToRadian(p.x))
                  * std::cos(degreeToRadian(p.y)) };
    auto const y{ -((fieldWidth_f - SOURCE_DIAMETER) / 2.0f) * std::cos(degreeToRadian(p.x))
                  * std::cos(degreeToRadian(p.y)) };
    return juce::Point<float>{ x, y };
}

//==============================================================================
static juce::Point<float> getSourceAzimuthElevation(juce::Point<float> const pXY, bool const bUseCosElev = false)
{
    // Calculate azim in range [0,1], and negate it because zirkonium wants -1 on right side.
    auto const fAzimuth{ -std::atan2(pXY.x, pXY.y) / juce::MathConstants<float>::pi };

    // Calculate xy distance from origin, and clamp it to 2 (ie ignore outside of circle).
    auto const hypo{ std::min(std::hypot(pXY.x, pXY.y), RADIUS_MAX) };

    float fElev;
    if (bUseCosElev) {
        fElev = std::acos(hypo / RADIUS_MAX);            // fElev is elevation in radian, [0,pi/2)
        fElev /= (juce::MathConstants<float>::pi / 2.f); // making range [0,1]
        fElev /= 2.0f;                                   // making range [0,.5] because that's what the zirkonium wants
    } else {
        fElev = (RADIUS_MAX - hypo) / 4.0f;
    }

    return juce::Point<float>(fAzimuth, fElev);
}

//==============================================================================
FlatViewWindow::FlatViewWindow(MainContentComponent & parent, GrisLookAndFeel & feel)
    : DocumentWindow("2D View", feel.getBackgroundColour(), juce::DocumentWindow::allButtons)
    , mMainContentComponent(parent)
    , mLookAndFeel(feel)
{
    startTimerHz(24);
}

//==============================================================================
FlatViewWindow::~FlatViewWindow()
{
    mMainContentComponent.closeFlatViewWindow();
}

//==============================================================================
void FlatViewWindow::drawFieldBackground(juce::Graphics & g, const int fieldWH) const
{
    auto const realW{ fieldWH - static_cast<int>(SOURCE_DIAMETER) };
    auto const realW_f{ static_cast<float>(realW) };

    auto const fieldWH_f{ static_cast<float>(fieldWH) };
    if (mMainContentComponent.getModeSelected() == LBAP) {
        // Draw light background squares.
        g.setColour(mLookAndFeel.getLightColour());
        auto offset{ fieldWH_f * 0.4f };
        auto size{ fieldWH_f * 0.2f };
        g.drawRect(offset, offset, size, size);
        offset = fieldWH_f * 0.2f;
        size = fieldWH_f * 0.6f;
        g.drawRect(offset, offset, size, size);
        g.drawRect(10, 10, fieldWH - 20, fieldWH - 20);
        // Draw shaded background squares.
        g.setColour(mLookAndFeel.getLightColour().withBrightness(0.5));
        offset = fieldWH_f * 0.3f;
        size = fieldWH_f * 0.4f;
        g.drawRect(offset, offset, size, size);
        offset = fieldWH_f * 0.1f;
        size = fieldWH_f * 0.8f;
        g.drawRect(offset, offset, size, size);
        // Draw lines.
        auto const center{ (fieldWH - realW_f) / 2.0f };
        g.drawLine(fieldWH_f * 0.2f, fieldWH_f * 0.2f, fieldWH_f - fieldWH_f * 0.2f, fieldWH_f - fieldWH_f * 0.2f);
        g.drawLine(fieldWH_f * 0.2f, fieldWH_f - fieldWH_f * 0.2f, fieldWH_f - fieldWH_f * 0.2f, fieldWH_f * 0.2f);
        g.drawLine(center, fieldWH_f / 2.0f, realW_f + center, fieldWH_f / 2.0f);
        g.drawLine(fieldWH_f / 2.0f, center, fieldWH_f / 2.0f, realW_f + center);
    } else {
        // Draw line and light circle.
        g.setColour(mLookAndFeel.getLightColour().withBrightness(0.5));
        auto w{ realW_f / 1.3f };
        auto x{ (fieldWH_f - w) / 2.0f };
        g.drawEllipse(x, x, w, w, 1);
        w = realW_f / 4.0f;
        x = (fieldWH_f - w) / 2.0f;
        g.drawEllipse(x, x, w, w, 1);

        w = realW_f;
        x = (fieldWH_f - w) / 2.0f;
        auto const r{ w / 2.0f * 0.296f };

        g.drawLine(x + r, x + r, (w + x) - r, (w + x) - r);
        g.drawLine(x + r, (w + x) - r, (w + x) - r, x + r);
        g.drawLine(x, fieldWH_f / 2, w + x, fieldWH_f / 2);
        g.drawLine(fieldWH_f / 2, x, fieldWH_f / 2, w + x);

        // Draw big background circle.
        g.setColour(mLookAndFeel.getLightColour());
        g.drawEllipse(x, x, w, w, 1);

        // Draw little background circle.
        w = realW_f / 2.0f;
        x = (fieldWH_f - w) / 2.0f;
        g.drawEllipse(x, x, w, w, 1);

        // Draw fill center cirlce.
        g.setColour(mLookAndFeel.getWinBackgroundColour());
        w = realW_f / 4.0f;
        w -= 2.0f;
        x = (fieldWH_f - w) / 2.0f;
        g.fillEllipse(x, x, w, w);
    }
}

//==============================================================================
void FlatViewWindow::paint(juce::Graphics & g)
{
    auto const fieldWh = getWidth(); // Same as getHeight()
    auto const fieldCenter = fieldWh / 2;
    auto const SourceDiameter_i{ static_cast<int>(SOURCE_DIAMETER) };
    auto const realW = fieldWh - SourceDiameter_i;

    g.fillAll(mLookAndFeel.getWinBackgroundColour());

    drawFieldBackground(g, fieldWh);

    g.setFont(mLookAndFeel.getFont());
    g.setColour(mLookAndFeel.getLightColour());
    g.drawText("0",
               fieldCenter,
               10,
               SourceDiameter_i,
               SourceDiameter_i,
               juce::Justification(juce::Justification::centred),
               false);
    g.drawText("90",
               realW - 10,
               (fieldWh - 4) / 2,
               SourceDiameter_i,
               SourceDiameter_i,
               juce::Justification(juce::Justification::centred),
               false);
    g.drawText("180",
               fieldCenter,
               realW - 6,
               SourceDiameter_i,
               SourceDiameter_i,
               juce::Justification(juce::Justification::centred),
               false);
    g.drawText("270",
               14,
               (fieldWh - 4) / 2,
               SourceDiameter_i,
               SourceDiameter_i,
               juce::Justification(juce::Justification::centred),
               false);

    // Draw sources.
    auto const maxDrawSource{ mMainContentComponent.getSourceInputs().size() };
    for (int i{}; i < maxDrawSource; ++i) {
        auto * it{ mMainContentComponent.getSourceInputs().getUnchecked(i) };
        if (it->getGain() == -1.0f) {
            continue;
        }
        drawSource(g, mMainContentComponent.getSourceInputs().getUnchecked(i), fieldWh);
        drawSourceSpan(g, mMainContentComponent.getSourceInputs().getUnchecked(i), fieldWh, fieldCenter);
    }
}

//==============================================================================
void FlatViewWindow::drawSource(juce::Graphics & g, Input * it, const int fieldWh) const
{
    auto const realW = static_cast<float>(fieldWh) - SOURCE_DIAMETER;
    juce::String stringVal;
    juce::Point<float> sourceP;
    if (mMainContentComponent.getModeSelected() == LBAP) {
        sourceP = juce::Point<float>(it->getCenter().z * 0.6f, it->getCenter().x * 0.6f) / 5.0f;
    } else {
        sourceP = juce::Point<float>(it->getCenter().z, it->getCenter().x) / 5.0f;
    }
    sourceP.x = realW / 2.0f + realW / 4.0f * sourceP.x;
    sourceP.y = realW / 2.0f - realW / 4.0f * sourceP.y;
    sourceP.x = sourceP.x < 0 ? 0 : sourceP.x > fieldWh - SOURCE_DIAMETER ? fieldWh - SOURCE_DIAMETER : sourceP.x;
    sourceP.y = sourceP.y < 0 ? 0 : sourceP.y > fieldWh - SOURCE_DIAMETER ? fieldWh - SOURCE_DIAMETER : sourceP.y;

    g.setColour(it->getColorJWithAlpha());
    g.fillEllipse(sourceP.x, sourceP.y, SOURCE_DIAMETER, SOURCE_DIAMETER);

    stringVal.clear();
    stringVal << it->getId();

    g.setColour(juce::Colours::black.withAlpha(it->getAlpha()));
    auto const tx{ static_cast<int>(sourceP.x) };
    auto const ty{ static_cast<int>(sourceP.y) };
    auto const SourceDiameter_i{ static_cast<int>(SOURCE_DIAMETER) };
    g.drawText(stringVal,
               tx + 6,
               ty + 1,
               SourceDiameter_i + 10,
               SourceDiameter_i,
               juce::Justification(juce::Justification::centredLeft),
               false);
    g.setColour(juce::Colours::white.withAlpha(it->getAlpha()));
    g.drawText(stringVal,
               tx + 5,
               ty,
               SourceDiameter_i + 10,
               SourceDiameter_i,
               juce::Justification(juce::Justification::centredLeft),
               false);
}

//==============================================================================
void FlatViewWindow::drawSourceSpan(juce::Graphics & g, Input * it, const int fieldWh, const int fieldCenter) const
{
    auto const colorS{ it->getColorJ() };

    juce::Point<float> sourceP;
    if (mMainContentComponent.getModeSelected() == LBAP) {
        sourceP = juce::Point<float>(it->getCenter().z * 0.6f, it->getCenter().x * 0.6f) / 5.0f;
    } else {
        sourceP = juce::Point<float>(it->getCenter().z, it->getCenter().x) / 5.0f;
    }

    if (mMainContentComponent.getModeSelected() == LBAP) {
        auto const realW{ fieldWh - static_cast<int>(SOURCE_DIAMETER) };
        auto const azimuthSpan{ fieldWh * (it->getAzimuthSpan() * 0.5f) };
        auto const halfAzimuthSpan{ azimuthSpan / 2.0f - SOURCE_RADIUS };

        sourceP.x = realW / 2.0f + realW / 4.0f * sourceP.x;
        sourceP.y = realW / 2.0f - realW / 4.0f * sourceP.y;
        sourceP.x = sourceP.x < 0 ? 0 : sourceP.x > fieldWh - SOURCE_DIAMETER ? fieldWh - SOURCE_DIAMETER : sourceP.x;
        sourceP.y = sourceP.y < 0 ? 0 : sourceP.y > fieldWh - SOURCE_DIAMETER ? fieldWh - SOURCE_DIAMETER : sourceP.y;

        g.setColour(colorS.withAlpha(it->getAlpha() * 0.6f));
        g.drawEllipse(sourceP.x - halfAzimuthSpan, sourceP.y - halfAzimuthSpan, azimuthSpan, azimuthSpan, 1.5f);
        g.setColour(colorS.withAlpha(it->getAlpha() * 0.2f));
        g.fillEllipse(sourceP.x - halfAzimuthSpan, sourceP.y - halfAzimuthSpan, azimuthSpan, azimuthSpan);
    } else {
        auto const HRAzimSpan{ 180.0f * it->getAzimuthSpan() }; // In zirkosc, this is [0,360]
        auto const HRElevSpan{ 180.0f * it->getZenithSpan() };  // In zirkosc, this is [0,90]

        if ((HRAzimSpan < 0.002f && HRElevSpan < 0.002f) || !mMainContentComponent.isSpanShown()) {
            return;
        }

        auto const azimuthElevation{ getSourceAzimuthElevation(sourceP, true) };

        auto const hrAzimuth{ azimuthElevation.x * 180.0f };   // In zirkosc [-180,180]
        auto const hrElevation{ azimuthElevation.y * 180.0f }; // In zirkosc [0,89.9999]

        // Calculate max and min elevation in degrees.
        juce::Point<float> maxElev = { hrAzimuth, hrElevation + HRElevSpan / 2.0f };
        juce::Point<float> minElev = { hrAzimuth, hrElevation - HRElevSpan / 2.0f };

        if (minElev.y < 0) {
            maxElev.y = (maxElev.y - minElev.y);
            minElev.y = 0.0f;
        }

        // Convert max min elev to xy.
        auto const screenMaxElev = degreeToXy(maxElev, fieldWh);
        auto const screenMinElev = degreeToXy(minElev, fieldWh);

        // Form minmax elev, calculate minmax radius.
        auto const maxRadius = std::sqrt(screenMaxElev.x * screenMaxElev.x + screenMaxElev.y * screenMaxElev.y);
        auto const minRadius = std::sqrt(screenMinElev.x * screenMinElev.x + screenMinElev.y * screenMinElev.y);

        // Drawing the path for spanning.
        juce::Path myPath;
        auto const fieldCenter_f{ static_cast<float>(fieldCenter) };
        myPath.startNewSubPath(fieldCenter_f + screenMaxElev.x, fieldCenter_f + screenMaxElev.y);
        // Half first arc center.
        myPath.addCentredArc(fieldCenter_f,
                             fieldCenter_f,
                             minRadius,
                             minRadius,
                             0.0,
                             degreeToRadian(-hrAzimuth),
                             degreeToRadian(-hrAzimuth + HRAzimSpan / 2));

        // If we are over the top of the dome we draw the adjacent angle.
        if (maxElev.getY() > 90.f) {
            myPath.addCentredArc(fieldCenter_f,
                                 fieldCenter_f,
                                 maxRadius,
                                 maxRadius,
                                 0.0,
                                 juce::MathConstants<float>::pi + degreeToRadian(-hrAzimuth + HRAzimSpan / 2),
                                 juce::MathConstants<float>::pi + degreeToRadian(-hrAzimuth - HRAzimSpan / 2));
        } else {
            myPath.addCentredArc(fieldCenter_f,
                                 fieldCenter_f,
                                 maxRadius,
                                 maxRadius,
                                 0.0,
                                 degreeToRadian(-hrAzimuth + HRAzimSpan / 2),
                                 degreeToRadian(-hrAzimuth - HRAzimSpan / 2));
        }
        myPath.addCentredArc(fieldCenter_f,
                             fieldCenter_f,
                             minRadius,
                             minRadius,
                             0.0,
                             degreeToRadian(-hrAzimuth - HRAzimSpan / 2),
                             degreeToRadian(-hrAzimuth));

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
    auto const fieldWh = std::min(getWidth(), getHeight());
    setSize(fieldWh, fieldWh);
}

//==============================================================================
void FlatViewWindow::closeButtonPressed()
{
    mMainContentComponent.closeFlatViewWindow();
}
