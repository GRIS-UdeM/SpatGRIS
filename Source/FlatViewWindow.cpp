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

#include "FlatViewWindow.hpp"

#include "Constants.hpp"
#include "MainComponent.hpp"
#include "Narrow.hpp"

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

    if (mMainContentComponent.getData().speakerSetup.spatMode == SpatMode::lbap) {
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
    drawFieldBackground(g);

    g.setFont(mLookAndFeel.getFont().withHeight(15.0f));

    juce::ScopedReadLock const lock{ mMainContentComponent.getLock() };

    auto const spatMode{ mMainContentComponent.getData().speakerSetup.spatMode };

    // Draw sources.
    for (auto const & source : mMainContentComponent.getData().project.sources) {
        auto *& ticket{ mLastSourceData[source.key] };

        auto & exchanger{ mSourceDataQueues[source.key] };
        exchanger.getMostRecent(ticket);

        if (ticket == nullptr) {
            continue;
        }

        auto const & sourceData{ ticket->get() };

        if (!sourceData) {
            continue;
        }

        drawSource(g, source.key, *sourceData, spatMode);
    }
}

//==============================================================================
void FlatViewWindow::drawSource(juce::Graphics & g,
                                source_index_t const sourceIndex,
                                ViewportSourceData const & sourceData,
                                SpatMode const spatMode) const
{
    static constexpr auto SOURCE_DIAMETER_INT{ narrow<int>(SOURCE_DIAMETER) };

    if (sourceData.colour.getAlpha() == 0) {
        return;
    }

    auto const fieldSize{ narrow<float>(getWidth()) };
    auto const realSize = fieldSize - SOURCE_DIAMETER;

    auto const getSourcePosition = [&]() {
        switch (spatMode) {
        case SpatMode::vbap: {
            auto const & vector{ PolarVector::fromCartesian(sourceData.position) };
            auto const radius{ 1.0f - vector.elevation / HALF_PI };
            return juce::Point<float>{ std::cos(vector.azimuth.get()), -std::sin(vector.azimuth.get()) } * radius;
        }
        case SpatMode::lbap: {
            auto const & position{ sourceData.position };
            return juce::Point<float>{ position.x, -position.y } / LBAP_EXTENDED_RADIUS;
        }
        }
        jassertfalse;
        return juce::Point<float>{};
    };

    auto const sourcePositionRelative{ getSourcePosition().translated(1.0f, 1.0f) * 0.5f };
    auto const sourcePositionAbsolute{ (sourcePositionRelative * realSize).translated(SOURCE_RADIUS, SOURCE_RADIUS) };

    // draw spans
    switch (spatMode) {
    case SpatMode::vbap:
        drawSourceVbapSpan(g, sourceData);
        break;
    case SpatMode::lbap:
        drawSourceLbapSpan(g, sourceData, sourcePositionAbsolute);
        break;
    default:
        jassertfalse;
    }

    auto const colour{ sourceData.colour };

    g.setColour(colour);
    g.fillEllipse(sourcePositionAbsolute.x - SOURCE_RADIUS,
                  sourcePositionAbsolute.y - SOURCE_RADIUS,
                  SOURCE_DIAMETER,
                  SOURCE_DIAMETER);

    auto const tx{ static_cast<int>(sourcePositionAbsolute.x - SOURCE_RADIUS) };
    auto const ty{ static_cast<int>(sourcePositionAbsolute.y - SOURCE_RADIUS) };
    juce::String const id{ sourceIndex.get() };
    g.setColour(juce::Colours::black.withAlpha(colour.getFloatAlpha()));
    g.drawText(id,
               tx,
               ty,
               SOURCE_DIAMETER_INT,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centred),
               false);
    g.setColour(juce::Colours::white.withAlpha(colour.getFloatAlpha()));
    g.drawText(id,
               tx,
               ty,
               SOURCE_DIAMETER_INT,
               SOURCE_DIAMETER_INT,
               juce::Justification(juce::Justification::centred),
               false);
}

//==============================================================================
void FlatViewWindow::drawSourceLbapSpan(juce::Graphics & g,
                                        ViewportSourceData const & sourceData,
                                        juce::Point<float> const & sourcePositionAbsolute) const
{
    auto const fieldSize{ narrow<float>(getWidth()) };
    auto const realSize{ fieldSize - SOURCE_DIAMETER };

    auto const maxSpan{ realSize / LBAP_EXTENDED_RADIUS * juce::MathConstants<float>::sqrt2 };
    auto const spanRange{ maxSpan - SOURCE_DIAMETER };

    auto const span{ sourceData.azimuthSpan * spanRange + SOURCE_DIAMETER };
    auto const halfSpan{ span / 2.0f };

    auto const color{ sourceData.colour };

    g.setColour(color);
    g.drawEllipse(sourcePositionAbsolute.x - halfSpan, sourcePositionAbsolute.y - halfSpan, span, span, 1.5f);
    g.setColour(color.withAlpha(color.getFloatAlpha() * 0.5f));
    g.fillEllipse(sourcePositionAbsolute.x - halfSpan, sourcePositionAbsolute.y - halfSpan, span, span);
}

//==============================================================================
void FlatViewWindow::drawSourceVbapSpan(juce::Graphics & g, ViewportSourceData const & sourceData) const
{
    auto const fieldSize{ narrow<float>(getWidth()) };
    auto const halfFieldSize{ fieldSize / 2.0f };
    auto const realSize{ fieldSize - SOURCE_DIAMETER };
    auto const halfRealSize{ realSize / 2.0f };

    // Radius
    auto const vector{ PolarVector::fromCartesian(sourceData.position) };
    auto const relativeElevation{ vector.elevation / HALF_PI };
    auto const halfElevationSpan{ sourceData.zenithSpan };
    auto const innerRadius{ 1.0f - std::min(relativeElevation + halfElevationSpan, 0.999f) };
    auto const outerRadius{ 1.0f - std::max(relativeElevation - halfElevationSpan, 0.0f) };

    // Angle
    auto const & sourceAzimuth{ HALF_PI - vector.azimuth };
    auto const azimuthSpan{ PI * sourceData.azimuthSpan };
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

    auto const & color{ sourceData.colour };

    g.setColour(color.withAlpha(color.getFloatAlpha() * 0.5f));
    g.fillPath(path);

    g.setColour(color);
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
