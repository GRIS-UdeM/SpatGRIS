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
FlatViewWindow::FlatViewWindow(MainContentComponent & parent, GrisLookAndFeel & feel)
    : DocumentWindow("2D View", feel.getBackgroundColour(), juce::DocumentWindow::allButtons)
    , mMainContentComponent(parent)
    , mLookAndFeel(feel)
{
    startTimerHz(24);
}

//==============================================================================
void FlatViewWindow::drawFieldBackground(juce::Graphics & g) const
{
    g.fillAll(mLookAndFeel.getWinBackgroundColour());

    auto const fieldSize{ narrow<float>(getWidth()) };
    auto const realSize{ fieldSize - SOURCE_DIAMETER };

    auto const getCenteredSquare = [fieldSize](float const size) -> juce::Rectangle<float> {
        auto const offset{ (fieldSize - size) / 2.0f };
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
        auto x{ (fieldSize - w) / 2.0f };
        g.drawEllipse(x, x, w, w, 1);
        w = realSize / 4.0f;
        x = (fieldSize - w) / 2.0f;
        g.drawEllipse(x, x, w, w, 1);

        w = realSize;
        x = (fieldSize - w) / 2.0f;
        auto const r{ w / 2.0f * 0.296f };

        g.drawLine(x + r, x + r, w + x - r, w + x - r);
        g.drawLine(x + r, w + x - r, w + x - r, x + r);
        g.drawLine(x, fieldSize / 2, w + x, fieldSize / 2);
        g.drawLine(fieldSize / 2, x, fieldSize / 2, w + x);

        // Draw big background circle.
        g.setColour(mLookAndFeel.getLightColour());
        g.drawEllipse(x, x, w, w, 1);

        // Draw little background circle.
        w = realSize / 2.0f;
        x = (fieldSize - w) / 2.0f;
        g.drawEllipse(x, x, w, w, 1);

        // Draw fill center circle.
        g.setColour(mLookAndFeel.getWinBackgroundColour());
        w = realSize / 4.0f;
        w -= 2.0f;
        x = (fieldSize - w) / 2.0f;
        g.fillEllipse(x, x, w, w);
    }
}

//==============================================================================
void FlatViewWindow::paint(juce::Graphics & g)
{
    static auto constexpr SOURCE_DIAMETER_INT{ narrow<int>(SOURCE_DIAMETER) };

    drawFieldBackground(g);

    auto const fieldSize = getWidth(); // Same as getHeight()
    auto const fieldCenter = fieldSize / 2;
    auto const realSize = fieldSize - SOURCE_DIAMETER_INT;

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

    juce::ScopedReadLock const lock{ mMainContentComponent.getLock() };

    // Draw sources.
    for (auto const source : mMainContentComponent.getData().project.sources) {
        drawSource(g, source, mMainContentComponent.getData().appData.spatMode);
    }
}

//==============================================================================
void FlatViewWindow::drawSource(juce::Graphics & g,
                                SourcesData::ConstNode const & source,
                                SpatMode const spatMode) const
{
    static constexpr auto SOURCE_DIAMETER_INT{ narrow<int>(SOURCE_DIAMETER) };

    if (!source.value->position) {
        return;
    }

    auto const fieldSize{ narrow<float>(getWidth()) };
    auto const realSize = fieldSize - SOURCE_DIAMETER;

    auto const getSourcePosition = [&]() {
        switch (spatMode) {
        case SpatMode::hrtfVbap:
        case SpatMode::vbap:
        case SpatMode::stereo: {
            jassert(source.value->vector);
            auto const & vector{ *source.value->vector };
            auto const radius{ 1.0f - vector.elevation / HALF_PI };
            return juce::Point<float>{ std::cos(vector.azimuth.get()), -std::sin(vector.azimuth.get()) } * radius;
        }
        case SpatMode::lbap: {
            jassert(source.value->position);
            auto const & position{ *source.value->position };
            return juce::Point<float>{ position.x, -position.y } / LBAP_EXTENDED_RADIUS;
        }
        }
        jassertfalse;
        return juce::Point<float>{};
    };

    auto const sourcePositionRelative{ getSourcePosition().translated(1.0f, 1.0f) * 0.5f };
    auto const sourcePositionAbsolute{ (sourcePositionRelative * realSize).translated(SOURCE_RADIUS, SOURCE_RADIUS) };

    auto const color{ source.value->colour };

    g.setColour(color);
    g.fillEllipse(sourcePositionAbsolute.x - SOURCE_RADIUS,
                  sourcePositionAbsolute.y - SOURCE_RADIUS,
                  SOURCE_DIAMETER,
                  SOURCE_DIAMETER);

    g.setColour(juce::Colours::black.withAlpha(color.getAlpha())); // TODO : should read alpha from peaks?
    auto const tx{ static_cast<int>(sourcePositionAbsolute.x - SOURCE_RADIUS) };
    auto const ty{ static_cast<int>(sourcePositionAbsolute.y - SOURCE_RADIUS) };
    juce::String const id{ source.key.get() };
    g.drawText(id,
               tx,
               ty,
               SOURCE_DIAMETER_INT,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centred),
               false);
    g.setColour(
        juce::Colours::white.withAlpha(source.value->colour.getAlpha())); // TODO : should read alpha from peaks?
    g.drawText(id,
               tx,
               ty,
               SOURCE_DIAMETER_INT,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centred),
               false);

    // draw spans
    switch (spatMode) {
    case SpatMode::hrtfVbap:
    case SpatMode::vbap:
        drawSourceVbapSpan(g, *source.value);
        break;
    case SpatMode::lbap:
        drawSourceLbapSpan(g, sourcePositionAbsolute, *source.value);
        break;
    case SpatMode::stereo:
        break;
    default:
        jassertfalse;
    }
}

//==============================================================================
void FlatViewWindow::drawSourceLbapSpan(juce::Graphics & g,
                                        juce::Point<float> const & sourcePositionAbsolute,
                                        SourceData const & source) const
{
    auto const fieldSize{ narrow<float>(getWidth()) };
    auto const realSize{ fieldSize - SOURCE_DIAMETER };

    auto const maxSpan{ realSize / LBAP_EXTENDED_RADIUS * juce::MathConstants<float>::sqrt2 };
    auto const spanRange{ maxSpan - SOURCE_RADIUS };

    auto const span{ source.azimuthSpan * spanRange + SOURCE_RADIUS };
    auto const halfSpan{ span / 2.0f };

    auto const color{ source.colour };

    // TODO : should alpha be yielded from source peaks?
    static constexpr auto ALPHA{ 1.0f };

    g.setColour(color.withAlpha(ALPHA * 0.6f));
    g.drawEllipse(sourcePositionAbsolute.x - halfSpan, sourcePositionAbsolute.y - halfSpan, span, span, 1.5f);
    g.setColour(color.withAlpha(ALPHA * 0.2f));
    g.fillEllipse(sourcePositionAbsolute.x - halfSpan, sourcePositionAbsolute.y - halfSpan, span, span);
}

//==============================================================================
void FlatViewWindow::drawSourceVbapSpan(juce::Graphics & g, SourceData const & source) const
{
    auto const fieldSize{ narrow<float>(getWidth()) };
    auto const halfFieldSize{ fieldSize / 2.0f };
    auto const realSize{ fieldSize - SOURCE_DIAMETER };
    auto const halfRealSize{ realSize / 2.0f };

    // Radius
    jassert(source.vector);
    auto const relativeElevation{ source.vector->elevation / HALF_PI };
    auto const halfElevationSpan{ source.zenithSpan };
    auto const innerRadius{ 1.0f - std::min(relativeElevation + halfElevationSpan, 0.999f) };
    auto const outerRadius{ 1.0f - std::max(relativeElevation - halfElevationSpan, 0.0f) };

    // Angle
    auto const & sourceAzimuth{ HALF_PI - source.vector->azimuth };
    auto const azimuthSpan{ PI * source.azimuthSpan };
    auto const startAngle{ sourceAzimuth - azimuthSpan };
    auto const endAngle{ sourceAzimuth + azimuthSpan };

    juce::Path path{};
    path.addCentredArc(0.0f, 0.0f, outerRadius, outerRadius, 0.0f, endAngle.get(), startAngle.get(), true);
    if (innerRadius > 0.0f) {
        path.addCentredArc(0.0f, 0.0f, innerRadius, innerRadius, 0.0f, startAngle.get(), endAngle.get(), false);
    } else {
        path.lineTo(halfFieldSize, halfFieldSize);
    }
    path.closeSubPath();
    path.applyTransform(juce::AffineTransform::translation(1.0f, 1.0f)
                            .scaled(halfRealSize, halfRealSize)
                            .translated(SOURCE_RADIUS, SOURCE_RADIUS));

    auto const & color{ source.colour };

    // TODO : should be fetched from source peaks?
    static constexpr auto ALPHA{ 1.0f };

    g.setColour(color.withAlpha(ALPHA * 0.2f));
    g.fillPath(path);

    g.setColour(color.withAlpha(ALPHA * 0.6f));
    g.strokePath(path, juce::PathStrokeType(0.5));
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
