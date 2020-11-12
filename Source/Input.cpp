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

#include "MainComponent.h"

//==============================================================================
Input::Input(MainContentComponent & mainContentComponent, SmallGrisLookAndFeel & lookAndFeel, int const id)
    : mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
    , mIdChannel(id)
    , mDirectOutChannel(0)
    , mVuMeter(*this, lookAndFeel)
{
    this->resetPosition();
    this->setColor(juce::Colour::fromHSV(0, 1, 0.75, 1), true);
}

//==============================================================================
void Input::resetPosition()
{
    this->mAzimuth = M_PI4;
    this->mZenith = M_PI2;

    this->mAzimuthSpan = 0.0f;
    this->mZenithSpan = 0.0f;
    ;

    this->mGain = -1.0f;
    this->mRadius = 1.0f;

    this->mCenter.x = (14.0f * sinf(this->mZenith) * cosf(this->mAzimuth));
    this->mCenter.z = (14.0f * sinf(this->mZenith) * sinf(this->mAzimuth));
    this->mCenter.y = (14.0f * cosf(this->mZenith) + (mSizeT / 2.0f));
}

//==============================================================================
glm::vec3 Input::polToCar(float azimuth, float zenith) const
{
    glm::vec3 cart;
    float factor = this->mRadius * 10.0f;
    cart.x = (factor * sinf(zenith) * cosf(azimuth));
    cart.z = (factor * sinf(zenith) * sinf(azimuth));
    cart.y = ((10.0f * cosf(zenith)) + (mSizeT / 2.0f));
    return cart;
}

//==============================================================================
glm::vec3 Input::polToCar3d(float azimuth, float zenith) const
{
    glm::vec3 cart;
    float factor = this->mRadius * 10.0f;
    cart.x = (factor * cosf(azimuth));
    cart.z = (factor * sinf(azimuth));
    cart.y = ((10.0f * cosf(zenith)) + (mSizeT / 2.0f));
    return cart;
}

//==============================================================================
void Input::setMuted(bool mute)
{
    this->mMainContentComponent.muteInput(this->mIdChannel, mute);
    if (mute) {
        this->mMainContentComponent.soloInput(this->mIdChannel, false);
    }
}

//==============================================================================
void Input::setSolo(bool solo)
{
    this->mMainContentComponent.soloInput(this->mIdChannel, solo);
    if (solo) {
        this->mMainContentComponent.muteInput(this->mIdChannel, false);
    }
}

//==============================================================================
void Input::setColor(juce::Colour color, bool updateLevel)
{
    this->mColorJ = color;
    this->mColor.x = this->mColorJ.getFloatRed();
    this->mColor.y = this->mColorJ.getFloatGreen();
    this->mColor.z = this->mColorJ.getFloatBlue();

    if (updateLevel) {
        this->mVuMeter.setColor(this->mColorJ);
    }
}

//==============================================================================
glm::vec3 Input::getNumberColor() const
{
    return glm::vec3(this->mColor.x * 0.5, this->mColor.y * 0.5, this->mColor.z * 0.5);
}

//==============================================================================
juce::Colour Input::getColorJWithAlpha() const
{
    if (this->mMainContentComponent.isSourceLevelShown) {
        return this->mColorJ.withMultipliedAlpha(this->getAlpha());
    } else {
        return this->mColorJ;
    }
}

//==============================================================================
float Input::getAlpha() const
{
    if (this->mMainContentComponent.isSourceLevelShown) {
        return this->mMainContentComponent.getLevelsAlpha(this->mIdChannel - 1);
    } else {
        return 1.0f;
    }
}

//==============================================================================
void Input::draw()
{
    float transpa = 0.75;

    // If not initalized, don't draw.
    if (this->mGain == -1.0) {
        return;
    }

    // If isSourceLevelShown is on and alpha below 0.01, don't draw.
    if (this->mMainContentComponent.isSourceLevelShown && this->getAlpha() <= 0.01) {
        return;
    }

    // Draw 3D sphere.
    glPushMatrix();
    glTranslatef(this->mCenter.x, this->mCenter.y, this->mCenter.z);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(2);

    if (this->mMainContentComponent.isSourceLevelShown) {
        glColor4f(this->mColor.x, this->mColor.y, this->mColor.z, this->getAlpha());
    } else {
        glColor4f(this->mColor.x, this->mColor.y, this->mColor.z, transpa);
    }

    glutSolidSphere(this->mSizeT, 8, 8);
    glTranslatef(-this->mCenter.x, -this->mCenter.y, -this->mCenter.z);

    if ((this->mAzimuthSpan != 0.0f || this->mZenithSpan != 0.0f) && this->mMainContentComponent.isSpanShown) {
        if (this->mMainContentComponent.getModeSelected() == 1) {
            drawSpanLbap(this->mCenter.x, this->mCenter.y, this->mCenter.z);
        } else {
            drawSpan();
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

//==============================================================================
void Input::drawSpan()
{
    int num = 8;
    float newazi, newele;
    glm::vec3 cart;

    glPointSize(4);
    glBegin(GL_POINTS);

    for (int i = 0; i < num; i++) {
        float azidev = i * this->mAzimuthSpan * 0.5f * 0.42f;
        for (int j = 0; j < 2; j++) {
            if (j)
                newazi = this->mAzimuth + azidev;
            else
                newazi = this->mAzimuth - azidev;

            if (newazi > M_PI)
                newazi -= (M_PI * 2.0f);
            else if (newazi < -M_PI)
                newazi += (M_PI * 2.0f);

            if (this->mMainContentComponent.getModeSelected() == 1) {
                cart = this->polToCar3d(newazi, this->mZenith);
            } else {
                cart = this->polToCar(newazi, this->mZenith);
            }
            glVertex3f(cart.x, cart.y, cart.z);
            for (int k = 0; k < 4; k++) {
                float eledev = (k + 1) * this->mZenithSpan * 2.0f * 0.38f;
                for (int l = 0; l < 2; l++) {
                    if (l)
                        newele = this->mZenith + eledev;
                    else
                        newele = this->mZenith - eledev;
                    if (newele > (M_PI * 0.5f))
                        newele = (M_PI * 0.5f);
                    else if (newele < 0)
                        newele = 0;

                    if (this->mMainContentComponent.getModeSelected() == 1) {
                        cart = this->polToCar3d(newazi, newele);
                    } else {
                        cart = this->polToCar(newazi, newele);
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
    return this->mMainContentComponent.getLevelsIn(this->mIdChannel - 1);
}

//==============================================================================
void Input::drawSpanLbap(float x, float y, float z)
{
    int num = 4;
    float ytmp;

    glPointSize(4);
    glBegin(GL_POINTS);

    // For the same elevation as the source position.
    for (int i = 1; i <= num; i++) {
        float raddev = i / 4.f * this->mAzimuthSpan * 6.f;
        if (i < num) {
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
    for (int j = 0; j < num; j++) {
        float eledev = (j + 1) / 2.f * this->mZenithSpan * 15.f;
        for (int k = 0; k < 2; k++) {
            if (k) {
                ytmp = y + eledev;
                ytmp = ytmp > 10.f ? 10.f : ytmp;
            } else {
                ytmp = y - eledev;
                ytmp = ytmp < -10.f ? -10.f : ytmp;
            }
            for (int i = 1; i <= num; i++) {
                float raddev = i / 4.f * this->mAzimuthSpan * 6.f;
                if (i < num) {
                    glVertex3f(x + raddev, ytmp, z + raddev);
                    glVertex3f(x + raddev, ytmp, z - raddev);
                    glVertex3f(x - raddev, ytmp, z + raddev);
                    glVertex3f(x - raddev, ytmp, z - raddev);
                }
                glVertex3f(x + raddev, ytmp, z);
                glVertex3f(x - raddev, ytmp, z);
                glVertex3f(x, ytmp, z + raddev);
                glVertex3f(x, ytmp, z - raddev);
            }
        }
    }

    glEnd();
    glPointSize(1);
}

//==============================================================================
void Input::updateValues(float az, float ze, float azS, float zeS, float radius, float g, int mode)
{
    this->mAzimuth = az;
    this->mZenith = ze;
    this->mAzimuthSpan = azS;
    this->mZenithSpan = zeS;
    this->mRadius = radius;
    this->mGain = g;

    float factor = radius * 10.0f;

    if (mode == 1) {
        this->mCenter.x = (factor * cosf(this->mAzimuth));
        this->mCenter.z = (factor * sinf(this->mAzimuth));
        this->mCenter.y = ((10.0f * cosf(this->mZenith)) + (mSizeT / 2.0f));
    } else {
        this->mCenter.x = (factor * sinf(this->mZenith) * cosf(this->mAzimuth));
        this->mCenter.z = (factor * sinf(this->mZenith) * sinf(this->mAzimuth));
        this->mCenter.y = ((10.0f * cosf(this->mZenith)) + (mSizeT / 2.0f));
    }
}

//==============================================================================
void Input::updateValuesOld(float az, float ze, float azS, float zeS, float g)
{
    if (az < 0) {
        this->mAzimuth = fabsf(az) * M_PI;
    } else {
        this->mAzimuth = (1.0f - az) * M_PI + M_PI;
    }
    this->mZenith = (M_PI2) - (M_PI * ze);

    this->mAzimuthSpan = azS;
    this->mZenithSpan = zeS;
    this->mGain = g;

    this->mCenter.x = (10.0f * sinf(this->mZenith) * cosf(this->mAzimuth));
    this->mCenter.z = (10.0f * sinf(this->mZenith) * sinf(this->mAzimuth));
    this->mCenter.y = (10.0f * cosf(this->mZenith)) + (this->mSizeT / 2.0f);
}

//==============================================================================
void Input::sendDirectOutToClient(int id, int chn)
{
    this->mMainContentComponent.setDirectOut(id, chn);
}

//==============================================================================
void Input::setDirectOutChannel(int chn)
{
    this->mDirectOutChannel = chn;
    if (chn == 0) {
        this->mVuMeter.directOut.setButtonText("-");
        return;
    } else {
        this->mVuMeter.directOut.setButtonText(juce::String(chn));
    }
}
