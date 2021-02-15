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

#include "Input.h"

#include "GlSphere.h"
#include "MainComponent.h"

//==============================================================================
Input::Input(MainContentComponent & mainContentComponent, SmallGrisLookAndFeel & lookAndFeel, int const id)
    : mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
    , mIdChannel(id)
    , mVuMeter(*this, lookAndFeel)
{
    resetPosition();
    setColor(juce::Colour::fromHSV(0, 1, 0.75, 1), true);
}

//==============================================================================
void Input::resetPosition()
{
    mAzimuth = juce::MathConstants<float>::halfPi / 2.0f;
    mZenith = juce::MathConstants<float>::halfPi;

    mAzimuthSpan = 0.0f;
    mZenithSpan = 0.0f;

    mGain = -1.0f;
    mRadius = 1.0f;

    mCenter.x = 14.0f * std::sin(mZenith) * std::cos(mAzimuth);
    mCenter.z = 14.0f * std::sin(mZenith) * std::sin(mAzimuth);
    mCenter.y = 14.0f * std::cos(mZenith) + mSizeT / 2.0f;
}

//==============================================================================
glm::vec3 Input::polToCar(float const azimuth, float const zenith) const
{
    glm::vec3 cart;
    auto const factor{ mRadius * 10.0f };
    cart.x = factor * std::sin(zenith) * std::cos(azimuth);
    cart.z = factor * std::sin(zenith) * std::sin(azimuth);
    cart.y = 10.0f * std::cos(zenith) + mSizeT / 2.0f;
    return cart;
}

//==============================================================================
glm::vec3 Input::polToCar3d(float const azimuth, float const zenith) const
{
    glm::vec3 cart;
    auto const factor{ mRadius * 10.0f };
    cart.x = factor * std::cos(azimuth);
    cart.z = factor * std::sin(azimuth);
    cart.y = 10.0f * std::cos(zenith) + mSizeT / 2.0f;
    return cart;
}

//==============================================================================
void Input::setMuted(bool const mute)
{
    mMainContentComponent.muteInput(mIdChannel, mute);
    if (mute) {
        mMainContentComponent.soloInput(mIdChannel, false);
    }
}

//==============================================================================
void Input::setSolo(bool const solo)
{
    mMainContentComponent.soloInput(mIdChannel, solo);
    if (solo) {
        mMainContentComponent.muteInput(mIdChannel, false);
    }
}

//==============================================================================
void Input::setColor(juce::Colour const color, bool const updateLevel)
{
    mColorJ = color;
    mColor.x = mColorJ.getFloatRed();
    mColor.y = mColorJ.getFloatGreen();
    mColor.z = mColorJ.getFloatBlue();

    if (updateLevel) {
        mVuMeter.setColor(mColorJ);
    }
}

//==============================================================================
glm::vec3 Input::getNumberColor() const
{
    return glm::vec3{ mColor.x * 0.5f, mColor.y * 0.5f, mColor.z * 0.5f };
}

//==============================================================================
juce::Colour Input::getColorJWithAlpha() const
{
    if (mMainContentComponent.isSourceLevelShown()) {
        return mColorJ.withMultipliedAlpha(getAlpha());
    }
    return mColorJ;
}

//==============================================================================
float Input::getAlpha() const
{
    if (mMainContentComponent.isSourceLevelShown()) {
        return mMainContentComponent.getLevelsAlpha(mIdChannel - 1);
    }
    return 1.0f;
}

//==============================================================================
void Input::draw() const
{
    static auto constexpr ALPHA{ 0.75f };

    // If not initialized, don't draw.
    if (mGain == -1.0f) {
        return;
    }

    // If isSourceLevelShown is on and alpha below 0.01, don't draw.
    if (mMainContentComponent.isSourceLevelShown() && getAlpha() <= 0.01f) {
        return;
    }

    // Draw 3D sphere.
    glPushMatrix();
    glTranslatef(mCenter.x, mCenter.y, mCenter.z);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(2.0f);

    if (mMainContentComponent.isSourceLevelShown()) {
        glColor4f(mColor.x, mColor.y, mColor.z, getAlpha());
    } else {
        glColor4f(mColor.x, mColor.y, mColor.z, ALPHA);
    }

#if defined(__APPLE__)
    glutSolidSphere(static_cast<double>(mSizeT), 8, 8);
#else
    drawSphere(mSizeT);
#endif

    glTranslatef(-mCenter.x, -mCenter.y, -mCenter.z);

    if ((mAzimuthSpan != 0.0f || mZenithSpan != 0.0f) && mMainContentComponent.isSpanShown()) {
        if (mMainContentComponent.getModeSelected() == SpatModes::lbap) {
            drawSpanLbap(mCenter.x, mCenter.y, mCenter.z);
        } else {
            drawSpan();
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

//==============================================================================
void Input::drawSpan() const
{
    static auto constexpr NUM{ 8 };

    glm::vec3 cart;

    glPointSize(4);
    glBegin(GL_POINTS);

    for (int i{}; i < NUM; ++i) {
        auto const aziDev{ static_cast<float>(i) * mAzimuthSpan * 0.5f * 0.42f };
        for (int j{}; j < 2; ++j) {
            auto newAzimuth{ j ? mAzimuth + aziDev : mAzimuth - aziDev };

            if (newAzimuth > juce::MathConstants<float>::pi) {
                newAzimuth -= juce::MathConstants<float>::twoPi;
            } else if (newAzimuth < -juce::MathConstants<float>::pi) {
                newAzimuth += juce::MathConstants<float>::twoPi;
            }

            if (mMainContentComponent.getModeSelected() == SpatModes::lbap) {
                cart = polToCar3d(newAzimuth, mZenith);
            } else {
                cart = polToCar(newAzimuth, mZenith);
            }
            glVertex3f(cart.x, cart.y, cart.z);
            for (int k{}; k < 4; ++k) {
                auto const eleDev{ (static_cast<float>(k) + 1.0f) * mZenithSpan * 2.0f * 0.38f };
                for (int l{}; l < 2; ++l) {
                    auto newElevation{ l ? mZenith + eleDev : mZenith - eleDev };
                    newElevation = std::clamp(newElevation, 0.0f, juce::MathConstants<float>::halfPi);

                    if (mMainContentComponent.getModeSelected() == SpatModes::lbap) {
                        cart = polToCar3d(newAzimuth, newElevation);
                    } else {
                        cart = polToCar(newAzimuth, newElevation);
                    }
                    glVertex3f(cart.x, cart.y, cart.z);
                }
            }
        }
    }

    glEnd();
    glPointSize(1);
}

//==============================================================================
float Input::getLevel() const
{
    return mMainContentComponent.getLevelsIn(mIdChannel - 1);
}

//==============================================================================
void Input::drawSpanLbap(float const x, float const y, float const z) const
{
    static auto constexpr NUM = 4;

    glPointSize(4);
    glBegin(GL_POINTS);

    // For the same elevation as the source position.
    for (auto i{ 1 }; i <= NUM; ++i) {
        auto const raddev{ static_cast<float>(i) / 4.0f * mAzimuthSpan * 6.0f };
        if (i < NUM) {
            glVertex3f(x + raddev, y, z + raddev);
            glVertex3f(x + raddev, y, z - raddev);
            glVertex3f(x - raddev, y, z + raddev);
            glVertex3f(x - raddev, y, z - raddev);
        }
        glVertex3f(x + raddev, y, z);
        glVertex3f(x - raddev, y, z);
        glVertex3f(x, y, z + raddev);
        glVertex3f(x, y, z - raddev);
    }
    // For all other elevation levels.
    for (int j{}; j < NUM; ++j) {
        auto const eledev{ static_cast<float>(j + 1) / 2.0f * mZenithSpan * 15.0f };
        for (int k{}; k < 2; ++k) {
            float yTmp;
            if (k) {
                yTmp = y + eledev;
                yTmp = yTmp > 10.0f ? 10.0f : yTmp;
            } else {
                yTmp = y - eledev;
                yTmp = yTmp < -10.0f ? -10.0f : yTmp;
            }
            for (auto i{ 1 }; i <= NUM; ++i) {
                auto const raddev{ static_cast<float>(i) / 4.0f * mAzimuthSpan * 6.0f };
                if (i < NUM) {
                    glVertex3f(x + raddev, yTmp, z + raddev);
                    glVertex3f(x + raddev, yTmp, z - raddev);
                    glVertex3f(x - raddev, yTmp, z + raddev);
                    glVertex3f(x - raddev, yTmp, z - raddev);
                }
                glVertex3f(x + raddev, yTmp, z);
                glVertex3f(x - raddev, yTmp, z);
                glVertex3f(x, yTmp, z + raddev);
                glVertex3f(x, yTmp, z - raddev);
            }
        }
    }

    glEnd();
    glPointSize(1);
}

//==============================================================================
void Input::updateValues(float const azimuth,
                         float const zenith,
                         float const azimuthSpan,
                         float const zenithSpan,
                         float const radius,
                         float const gain,
                         SpatModes const mode)
{
    mAzimuth = azimuth;
    mZenith = zenith;
    mAzimuthSpan = azimuthSpan;
    mZenithSpan = zenithSpan;
    mRadius = radius;
    mGain = gain;

    auto const factor{ radius * 10.0f };

    if (mode == SpatModes::lbap) {
        mCenter.x = factor * std::cos(mAzimuth);
        mCenter.z = factor * std::sin(mAzimuth);
        mCenter.y = 10.0f * std::cos(mZenith) + mSizeT / 2.0f;
    } else {
        mCenter.x = factor * std::sin(mZenith) * std::cos(mAzimuth);
        mCenter.z = factor * std::sin(mZenith) * std::sin(mAzimuth);
        mCenter.y = 10.0f * std::cos(mZenith) + mSizeT / 2.0f;
    }
}

//==============================================================================
void Input::updateValuesOld(float const azimuth,
                            float const zenith,
                            float const azimuthSpan,
                            float const zenithSpan,
                            float const g)
{
    if (azimuth < 0.0f) {
        mAzimuth = std::fabs(azimuth) * juce::MathConstants<float>::pi;
    } else {
        mAzimuth = (1.0f - azimuth) * juce::MathConstants<float>::pi + juce::MathConstants<float>::pi;
    }
    mZenith = juce::MathConstants<float>::halfPi - (juce::MathConstants<float>::pi * zenith);

    mAzimuthSpan = azimuthSpan;
    mZenithSpan = zenithSpan;
    mGain = g;

    mCenter.x = 10.0f * std::sin(mZenith) * std::cos(mAzimuth);
    mCenter.z = 10.0f * std::sin(mZenith) * std::sin(mAzimuth);
    mCenter.y = 10.0f * std::cos(mZenith) + (mSizeT / 2.0f);
}

//==============================================================================
void Input::sendDirectOutToClient(int const id, int const chn)
{
    mMainContentComponent.setDirectOut(id, chn);
}

//==============================================================================
void Input::setDirectOutChannel(int const chn)
{
    mDirectOutChannel = chn;
    if (chn == 0) {
        mVuMeter.getDirectOutButton().setButtonText("-");
    } else {
        mVuMeter.getDirectOutButton().setButtonText(juce::String(chn));
    }
}
