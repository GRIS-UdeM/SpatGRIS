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

#include "InputModel.h"

#include "GlSphere.h"
#include "MainComponent.h"
#include "SpeakerViewComponent.h"

//==============================================================================
InputModel::InputModel(MainContentComponent & mainContentComponent,
                       SmallGrisLookAndFeel & lookAndFeel,
                       source_index_t const index)
    : VuMeterModel(index.get())
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
    , mIndex(index)
    , mVuMeter(*this, lookAndFeel)
{
    resetPosition();
    setColor(juce::Colour::fromHSV(0, 1, 0.75, 1), true);
}

//==============================================================================
void InputModel::resetPosition()
{
    mVector.azimuth = HALF_PI / 2.0f;
    mVector.elevation = HALF_PI;

    mAzimuthSpan = 0.0f;
    mZenithSpan = 0.0f;

    mVector.length = 1.0f;

    mCenter.x = 14.0f * std::sin(mVector.elevation.get()) * std::cos(mVector.azimuth.get());
    mCenter.z = 14.0f * std::sin(mVector.elevation.get()) * std::sin(mVector.azimuth.get());
    mCenter.y = 14.0f * std::cos(mVector.elevation.get()) + SPHERE_RADIUS / 2.0f;
}

//==============================================================================
glm::vec3 InputModel::polToCar(radians_t const azimuth, radians_t const zenith) const
{
    glm::vec3 cart;
    auto const factor{ mVector.length * SpeakerViewComponent::MAX_RADIUS };
    cart.x = factor * std::sin(zenith.get()) * std::cos(azimuth.get());
    cart.z = factor * std::sin(zenith.get()) * std::sin(azimuth.get());
    cart.y = SpeakerViewComponent::MAX_RADIUS * std::cos(zenith.get()) + SPHERE_RADIUS / 2.0f;
    return cart;
}

//==============================================================================
glm::vec3 InputModel::polToCar3d(radians_t const azimuth, radians_t const zenith) const
{
    glm::vec3 cart;
    auto const factor{ mVector.length * SpeakerViewComponent::MAX_RADIUS };
    cart.x = factor * std::cos(azimuth.get());
    cart.z = factor * std::sin(azimuth.get());
    cart.y = SpeakerViewComponent::MAX_RADIUS * std::cos(zenith.get()) + SPHERE_RADIUS / 2.0f;
    return cart;
}

void InputModel::setState(PortState const state)
{
    mMainContentComponent.setSourceState(mIndex, state);
}

//==============================================================================
void InputModel::setColor(juce::Colour const color, bool const updateLevel)
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
glm::vec3 InputModel::getNumberColor() const
{
    return glm::vec3{ mColor.x * 0.5f, mColor.y * 0.5f, mColor.z * 0.5f };
}

//==============================================================================
juce::Colour InputModel::getColorJWithAlpha() const
{
    if (mMainContentComponent.isSourceLevelShown()) {
        return mColorJ.withMultipliedAlpha(getAlpha());
    }
    return mColorJ;
}

//==============================================================================
float InputModel::getAlpha() const
{
    if (mMainContentComponent.isSourceLevelShown()) {
        return mMainContentComponent.getSourceAlpha(mIndex);
    }
    return 1.0f;
}

//==============================================================================
void InputModel::draw() const
{
    static auto constexpr ALPHA{ 0.75f };

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
    glutSolidSphere(static_cast<double>(SPHERE_RADIUS), 8, 8);
#else
    drawSphere(SPHERE_RADIUS);
#endif

    glTranslatef(-mCenter.x, -mCenter.y, -mCenter.z);

    if ((mAzimuthSpan != 0.0f || mZenithSpan != 0.0f) && mMainContentComponent.isSpanShown()) {
        if (mMainContentComponent.getModeSelected() == SpatMode::lbap) {
            drawSpanLbap(mCenter.x, mCenter.y, mCenter.z);
        } else {
            drawSpan();
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

//==============================================================================
void InputModel::drawSpan() const
{
    static auto constexpr NUM{ 8 };

    glm::vec3 cart;

    glPointSize(4);
    glBegin(GL_POINTS);

    auto const & azimuth{ mVector.azimuth };
    auto const & elevation{ mVector.elevation };

    for (int i{}; i < NUM; ++i) {
        auto const aziDev{ radians_t{ narrow<radians_t::type>(i) } * mAzimuthSpan * 0.5f * 0.42f };
        for (int j{}; j < 2; ++j) {
            auto newAzimuth{ j ? azimuth + aziDev : azimuth - aziDev };

            newAzimuth = newAzimuth.centered();

            if (mMainContentComponent.getModeSelected() == SpatMode::lbap) {
                cart = polToCar3d(newAzimuth, elevation);
            } else {
                cart = polToCar(newAzimuth, elevation);
            }
            glVertex3f(cart.x, cart.y, cart.z);
            for (int k{}; k < 4; ++k) {
                radians_t const eleDev{ (static_cast<float>(k) + 1.0f) * mZenithSpan * 2.0f * 0.38f };
                for (int l{}; l < 2; ++l) {
                    auto newElevation{ l ? elevation + eleDev : elevation - eleDev };
                    newElevation = std::clamp(newElevation, radians_t{}, HALF_PI);

                    if (mMainContentComponent.getModeSelected() == SpatMode::lbap) {
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
dbfs_t InputModel::getLevel() const
{
    return mMainContentComponent.getSourcePeak(mIndex);
}

//==============================================================================
void InputModel::drawSpanLbap(float const x, float const y, float const z) const
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
                yTmp = yTmp > SpeakerViewComponent::MAX_RADIUS ? SpeakerViewComponent::MAX_RADIUS : yTmp;
            } else {
                yTmp = y - eledev;
                yTmp = yTmp < -SpeakerViewComponent::MAX_RADIUS ? -SpeakerViewComponent::MAX_RADIUS : yTmp;
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
void InputModel::updateValues(PolarVector const & vector,
                              float azimuthSpan,
                              float zenithSpan,
                              dbfs_t gain,
                              SpatMode mode)
{
    mVector = vector;
    mAzimuthSpan = azimuthSpan;
    mZenithSpan = zenithSpan;
    mGain = gain;

    auto const radiusInComponent{ radius * SpeakerViewComponent::MAX_RADIUS };
    if (mode == SpatMode::lbap) {
        mCenter.x = radiusInComponent * std::cos(mAzimuth.get());
        mCenter.y = (HALF_PI - zenith) / HALF_PI * SpeakerViewComponent::MAX_RADIUS;
        mCenter.z = radiusInComponent * std::sin(mAzimuth.get());
    } else {
        mCenter.x = radiusInComponent * std::sin(mZenith.get()) * std::cos(mAzimuth.get());
        mCenter.y = SpeakerViewComponent::MAX_RADIUS * std::cos(mZenith.get());
        mCenter.z = radiusInComponent * std::sin(mZenith.get()) * std::sin(mAzimuth.get());
    }
}

//==============================================================================
void InputModel::updateValuesOld(float const azimuth,
                                 float const zenith,
                                 float const azimuthSpan,
                                 float const zenithSpan,
                                 float const g)
{
    if (azimuth < 0.0f) {
        mAzimuth = PI * std::abs(azimuth);
    } else {
        mAzimuth = PI * (1.0f - azimuth) + PI;
    }
    mZenith = HALF_PI - PI * zenith;

    mAzimuthSpan = azimuthSpan;
    mZenithSpan = zenithSpan;
    mGain = g;

    mCenter.x = SpeakerViewComponent::MAX_RADIUS * std::sin(mZenith.get()) * std::cos(mAzimuth.get());
    mCenter.y = SpeakerViewComponent::MAX_RADIUS * std::cos(mZenith.get()) + (SPHERE_RADIUS / 2.0f);
    mCenter.z = SpeakerViewComponent::MAX_RADIUS * std::sin(mZenith.get()) * std::sin(mAzimuth.get());
}

//==============================================================================
void InputModel::setDirectOut(tl::optional<output_patch_t> const directOut)
{
    mDirectOut = directOut;

    auto const buttonText{ directOut.map_or([](output_patch_t const & patch) { return juce::String{ patch.get() }; },
                                            juce::String{ "-" }) };
    mVuMeter.getDirectOutButton().setButtonText(buttonText);
    mMainContentComponent.setDirectOut()
}

//==============================================================================
void InputModel::sendDirectOutToClient(int const id, output_patch_t const chn)
{
    mMainContentComponent.setSourceDirectOut(id, chn);
}

//==============================================================================
void InputModel::setDirectOutChannel(output_patch_t const chn)
{
    mDirectOutChannel = chn;
    if (chn == output_patch_t{}) {
        mVuMeter.getDirectOutButton().setButtonText("-");
    } else {
        mVuMeter.getDirectOutButton().setButtonText(juce::String(chn.get()));
    }
}
