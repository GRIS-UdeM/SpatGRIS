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

#include "sg_VuMeterComponent.hpp"

#include "sg_GrisLookAndFeel.hpp"
#include "sg_Narrow.hpp"
#include "sg_constants.hpp"

namespace
{
constexpr auto VU_METER_MIN_HEIGHT = 140;
}

//==============================================================================
void VuMeterComponent::resized()
{
    auto const width{ getWidth() };
    auto const height{ getHeight() };

    mColorGrad = juce::ColourGradient{ juce::Colour::fromRGB(255, 94, 69),
                                       0.f,
                                       0.f,
                                       juce::Colour::fromRGB(17, 255, 159),
                                       0.f,
                                       narrow<float>(height),
                                       false };
    mColorGrad.addColour(0.1, juce::Colours::yellow);

    // Create vu-meter foreground image.
    mVuMeterBit = juce::Image{ juce::Image::RGB, width, height, true }; // used to be width - 1
    juce::Graphics gf{ mVuMeterBit };
    gf.setGradientFill(mColorGrad);
    gf.fillRect(0, 0, getWidth(), getHeight());
    gf.setColour(mLookAndFeel.getDarkColour());
    gf.setFont(10.0f);

    // Create vu-meter background image.
    mVuMeterBackBit = juce::Image{ juce::Image::RGB, width, height, true }; // used to be width - 1
    juce::Graphics gb{ mVuMeterBackBit };
    gb.setColour(mLookAndFeel.getDarkColour());
    gb.fillRect(0, 0, getWidth(), getHeight());
    gb.setColour(mLookAndFeel.getScrollBarColour());
    gb.setFont(10.0f);

    // Create vu-meter muted image.
    mVuMeterMutedBit = juce::Image(juce::Image::RGB, width, height, true); // used to be width - 1
    juce::Graphics gm{ mVuMeterMutedBit };
    gm.setColour(mLookAndFeel.getWinBackgroundColour());
    gm.fillRect(0, 0, getWidth(), getHeight());
    gm.setColour(mLookAndFeel.getScrollBarColour());
    gm.setFont(10.0f);

    // Draw ticks on images.
    auto const start = getWidth() - 3;
    static constexpr auto NUM_TICKS = 10;
    for (auto i{ 1 }; i < NUM_TICKS; ++i) {
        auto const y = i * height / NUM_TICKS;
        auto const y_f{ narrow<float>(y) };
        auto const start_f{ narrow<float>(start) };
        auto const with_f{ narrow<float>(getWidth()) };

        gf.drawLine(start_f, y_f, with_f, y_f, 1.0f);
        gb.drawLine(start_f, y_f, with_f, y_f, 1.0f);
        gm.drawLine(start_f, y_f, with_f, y_f, 1.0f);
        if (i % 2 == 1) {
            gf.drawText(juce::String(i * -6), start - 15, y - 5, 15, 10, juce::Justification::centred, false);
            gb.drawText(juce::String(i * -6), start - 15, y - 5, 15, 10, juce::Justification::centred, false);
            gm.drawText(juce::String(i * -6), start - 15, y - 5, 15, 10, juce::Justification::centred, false);
        }
    }
}

//==============================================================================
void VuMeterComponent::paint(juce::Graphics & g)
{
    auto const width{ getWidth() };
    auto const height{ getHeight() };

    if (mIsMuted) {
        g.drawImage(mVuMeterMutedBit, 0, 0, width, height, 0, 0, width, height);
        return;
    }

    if (mLevel <= MIN_LEVEL_COMP && !mIsClipping) {
        g.drawImage(mVuMeterBackBit, 0, 0, width, height, 0, 0, width, height);
        return;
    }

    auto const magnitude{ 1.0f - std::clamp(mLevel, MIN_LEVEL_COMP, MAX_LEVEL_COMP) / MIN_LEVEL_COMP };
    auto const rel = narrow<int>(std::round(magnitude * narrow<float>(height)));
    auto const h = height - rel;
    g.drawImage(mVuMeterBit, 0, h, width, rel, 0, h, width, rel);
    g.drawImage(mVuMeterBackBit, 0, 0, width, h, 0, 0, width, h);
    if (mIsClipping) {
        g.setColour(juce::Colour::fromHSV(0.0, 1, 0.75, 1));
        juce::Rectangle<float> const clipRect{ 0.5, 0.5, narrow<float>(height - 1), 5 };
        g.fillRect(clipRect);
    }
}

//==============================================================================
void VuMeterComponent::mouseDown(juce::MouseEvent const & /*e*/)
{
    resetClipping();
}

//==============================================================================
int VuMeterComponent::getMinWidth() const noexcept
{
    return SLICES_WIDTH;
}

//==============================================================================
int VuMeterComponent::getMinHeight() const noexcept
{
    return VU_METER_MIN_HEIGHT;
}

//==============================================================================
void VuMeterComponent::resetClipping()
{
    mIsClipping = false;
    repaint();
}

//==============================================================================
void VuMeterComponent::setLevel(dbfs_t const level)
{
    auto const & clippedLevel{ std::clamp(level, MIN_LEVEL_COMP, MAX_LEVEL_COMP) };

    if (clippedLevel == mLevel) {
        return;
    }

    if (level > MAX_LEVEL_COMP) {
        mIsClipping = true;
    }
    mLevel = clippedLevel;

    repaint();
}

//==============================================================================
void VuMeterComponent::setMuted(bool const muted)
{
    if (muted == mIsMuted) {
        return;
    }
    mIsMuted = muted;
    repaint();
}