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

#include "FlatViewWindow.h"

#include "MainComponent.h"
#include "constants.hpp"
#include "narrow.hpp"

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
void FlatViewWindow::drawFieldBackground(juce::Graphics & g, int const fieldSize) const
{
    auto const fieldSizeFloat{ narrow<float>(fieldSize) };
    auto const realSize{ fieldSizeFloat - SOURCE_DIAMETER };

    auto const getCenteredSquare = [fieldSizeFloat](float const size) -> juce::Rectangle<float> {
        auto const offset{ (fieldSizeFloat - size) / 2.0f };
        juce::Rectangle<float> const result{ offset, offset, size, size };
        return result;
    };

    if (mMainContentComponent.getData().appData.spatMode == SpatMode::lbap) {
        // Draw shaded background squares.
        g.setColour(mLookAndFeel.getLightColour().withBrightness(0.5));
        auto const smallRect{ getCenteredSquare(realSize / LBAP_EXTENDED_RADIUS / 2.0f) };
        auto const noAttenuationRect{ getCenteredSquare(realSize / LBAP_EXTENDED_RADIUS) };
        auto const maxAttenuationRect{ getCenteredSquare(realSize) };
        g.drawRect(smallRect);
        g.drawEllipse(noAttenuationRect, 1.0f);
        g.drawEllipse(maxAttenuationRect, 1.0f);
        // Draw lines.
        static constexpr auto LINE_START{ SOURCE_DIAMETER / 2.0f };
        auto const lineEnd{ realSize + SOURCE_DIAMETER / 2.0f };
        g.drawLine(LINE_START, LINE_START, lineEnd, lineEnd);
        g.drawLine(LINE_START, lineEnd, lineEnd, LINE_START);
        // Draw light background squares.
        g.setColour(mLookAndFeel.getLightColour());
        g.drawRect(noAttenuationRect);
        g.drawRect(maxAttenuationRect);
    } else {
        // Draw line and light circle.
        g.setColour(mLookAndFeel.getLightColour().withBrightness(0.5));
        auto w{ realSize / 1.3f };
        auto x{ (fieldSizeFloat - w) / 2.0f };
        g.drawEllipse(x, x, w, w, 1);
        w = realSize / 4.0f;
        x = (fieldSizeFloat - w) / 2.0f;
        g.drawEllipse(x, x, w, w, 1);

        w = realSize;
        x = (fieldSizeFloat - w) / 2.0f;
        auto const r{ w / 2.0f * 0.296f };

        g.drawLine(x + r, x + r, (w + x) - r, (w + x) - r);
        g.drawLine(x + r, (w + x) - r, (w + x) - r, x + r);
        g.drawLine(x, fieldSizeFloat / 2, w + x, fieldSizeFloat / 2);
        g.drawLine(fieldSizeFloat / 2, x, fieldSizeFloat / 2, w + x);

        // Draw big background circle.
        g.setColour(mLookAndFeel.getLightColour());
        g.drawEllipse(x, x, w, w, 1);

        // Draw little background circle.
        w = realSize / 2.0f;
        x = (fieldSizeFloat - w) / 2.0f;
        g.drawEllipse(x, x, w, w, 1);

        // Draw fill center circle.
        g.setColour(mLookAndFeel.getWinBackgroundColour());
        w = realSize / 4.0f;
        w -= 2.0f;
        x = (fieldSizeFloat - w) / 2.0f;
        g.fillEllipse(x, x, w, w);
    }
}

//==============================================================================
void FlatViewWindow::paint(juce::Graphics & g)
{
    static auto constexpr SOURCE_DIAMETER_INT{ narrow<int>(SOURCE_DIAMETER) };

    auto const fieldSize = getWidth(); // Same as getHeight()
    auto const fieldCenter = fieldSize / 2;
    auto const realSize = fieldSize - SOURCE_DIAMETER_INT;

    g.fillAll(mLookAndFeel.getWinBackgroundColour());

    drawFieldBackground(g, fieldSize);

    g.setFont(mLookAndFeel.getFont().withHeight(15.0f));
    g.setColour(mLookAndFeel.getLightColour());
    g.drawText("0",
               fieldCenter,
               10,
               SOURCE_DIAMETER_INT,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centred),
               false);
    g.drawText("90",
               realSize - 10,
               (fieldSize - 4) / 2,
               SOURCE_DIAMETER_INT,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centred),
               false);
    g.drawText("180",
               fieldCenter,
               realSize - 6,
               SOURCE_DIAMETER_INT,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centred),
               false);
    g.drawText("270",
               14,
               (fieldSize - 4) / 2,
               SOURCE_DIAMETER_INT,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centred),
               false);

    // Draw sources.
    for (auto const source : mMainContentComponent.getData().project.sources) {
        drawSource(g, source, fieldSize);
        drawSourceSpan(g, *source.value, fieldSize, fieldCenter, mMainContentComponent.getData().appData.spatMode);
    }
}

//==============================================================================
void FlatViewWindow::drawSource(juce::Graphics & g, SourcesData::ConstNode const & source, const int fieldSize) const
{
    static constexpr auto SOURCE_DIAMETER_INT{ narrow<int>(SOURCE_DIAMETER) };

    if (!source.value->position) {
        return;
    }

    auto const fieldSizeFloat{ narrow<float>(fieldSize) };
    auto const realSize = fieldSizeFloat - SOURCE_DIAMETER;
    juce::Point<float> sourcePosition{ source.value->position->x, source.value->position->y };
    sourcePosition.x = realSize / 2.0f + realSize / 4.0f * sourcePosition.x;
    sourcePosition.y = realSize / 2.0f - realSize / 4.0f * sourcePosition.y;
    sourcePosition.x = sourcePosition.x < 0.0f
                           ? 0.0f
                           : sourcePosition.x > fieldSizeFloat - SOURCE_DIAMETER ? fieldSizeFloat - SOURCE_DIAMETER
                                                                                 : sourcePosition.x;
    sourcePosition.y = sourcePosition.y < 0.0f
                           ? 0.0f
                           : sourcePosition.y > fieldSizeFloat - SOURCE_DIAMETER ? fieldSizeFloat - SOURCE_DIAMETER
                                                                                 : sourcePosition.y;

    g.setColour(source.value->colour);
    g.fillEllipse(sourcePosition.x, sourcePosition.y, SOURCE_DIAMETER, SOURCE_DIAMETER);

    g.setColour(
        juce::Colours::black.withAlpha(source.value->colour.getAlpha())); // TODO : should read alpha from peaks?
    auto const tx{ static_cast<int>(sourcePosition.x) };
    auto const ty{ static_cast<int>(sourcePosition.y) };
    juce::String const stringVal{ source.key.get() };
    g.drawText(stringVal,
               tx + 6,
               ty + 1,
               SOURCE_DIAMETER_INT + 10,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centredLeft),
               false);
    g.setColour(
        juce::Colours::white.withAlpha(source.value->colour.getAlpha())); // TODO : should read alpha from peaks?
    g.drawText(stringVal,
               tx + 5,
               ty,
               SOURCE_DIAMETER_INT + 10,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centredLeft),
               false);
}

//==============================================================================
void FlatViewWindow::drawSourceSpan(juce::Graphics & g,
                                    SourceData const & source,
                                    const int fieldWh,
                                    const int fieldCenter,
                                    SpatMode const spatMode) const
{
    auto const colorS{ source.colour };

    juce::Point<float> sourceP{};

    auto const alpha{ source.colour.getAlpha() }; // TODO : should use alpha from peaks?
    if (spatMode == SpatMode::lbap) {
        auto const realW{ fieldWh - narrow<int>(SOURCE_DIAMETER) };
        auto const azimuthSpan{ static_cast<float>(fieldWh) * (source.azimuthSpan * 0.5f) };
        auto const halfAzimuthSpan{ azimuthSpan / 2.0f - SOURCE_RADIUS };

        sourceP.x = realW / 2.0f + realW / 4.0f * sourceP.x;
        sourceP.y = realW / 2.0f - realW / 4.0f * sourceP.y;
        sourceP.x = sourceP.x < 0 ? 0 : sourceP.x > fieldWh - SOURCE_DIAMETER ? fieldWh - SOURCE_DIAMETER : sourceP.x;
        sourceP.y = sourceP.y < 0 ? 0 : sourceP.y > fieldWh - SOURCE_DIAMETER ? fieldWh - SOURCE_DIAMETER : sourceP.y;

        g.setColour(colorS.withAlpha(alpha * 0.6f));
        g.drawEllipse(sourceP.x - halfAzimuthSpan, sourceP.y - halfAzimuthSpan, azimuthSpan, azimuthSpan, 1.5f);
        g.setColour(colorS.withAlpha(alpha * 0.2f));
        g.fillEllipse(sourceP.x - halfAzimuthSpan, sourceP.y - halfAzimuthSpan, azimuthSpan, azimuthSpan);
    } else {
        auto const HRAzimSpan{ 180.0f * source.azimuthSpan };
        auto const HRElevSpan{ 180.0f * source.zenithSpan };

        if ((HRAzimSpan < 0.002f && HRElevSpan < 0.002f) || !mMainContentComponent.isSpanShown()) {
            return;
        }

        auto const azimuthElevation{ getSourceAzimuthElevation(sourceP, true) };

        auto const hrAzimuth{ azimuthElevation.x * 180.0f };
        auto const hrElevation{ azimuthElevation.y * 180.0f };

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

        g.setColour(colorS.withAlpha(alpha * 0.2f));
        g.fillPath(myPath);

        g.setColour(colorS.withAlpha(alpha * 0.6f));
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
