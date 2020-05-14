/*
 This file is part of SpatGRIS2.
 
 Developers: Samuel Béland, Olivier Belanger, Nicolas Masson
 
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
#include "LevelComponent.h"

Input::Input(MainContentComponent * parent, SmallGrisLookAndFeel * feel, int id) {
    this->mainParent = parent;
    this->grisFeel = feel;
    this->idChannel = id;
    this->directOutChannel = 0;
    
    this->resetPosition();

    this->vuMeter.reset(new LevelComponent(this, this->grisFeel));
    this->setColor(Colour::fromHSV(0, 1, 0.75, 1), true);
}

void Input::resetPosition() {
    this->azimuth = M_PI4;
    this->zenith  = M_PI2;
    
    this->azimSpan = 0.0f;
    this->zeniSpan = 0.0f;;
    
    this->gain = -1.0f;
    this->radius = 1.0f;
    
    this->center.x = (14.0f * sinf(this->zenith) * cosf(this->azimuth));
    this->center.z = (14.0f * sinf(this->zenith) * sinf(this->azimuth));
    this->center.y = (14.0f * cosf(this->zenith) + (sizeT / 2.0f));
}

glm::vec3 Input::polToCar(float azimuth, float zenith) const {
    glm::vec3 cart;
    float factor = this->radius * 10.0f;
    cart.x = (factor * sinf(zenith) * cosf(azimuth));
    cart.z = (factor * sinf(zenith) * sinf(azimuth));
    cart.y = ((10.0f * cosf(zenith)) + (sizeT / 2.0f));
    return cart;
}

glm::vec3 Input::polToCar3d(float azimuth, float zenith) const {
    glm::vec3 cart;
    float factor = this->radius * 10.0f;
    cart.x = (factor * cosf(azimuth));
    cart.z = (factor * sinf(azimuth));
    cart.y = ((10.0f * cosf(zenith)) + (sizeT / 2.0f));
    return cart;
}

void Input::setMuted(bool mute) {
    this->mainParent->muteInput(this->idChannel, mute);
    if (mute) {
        this->mainParent->soloInput(this->idChannel, false);
    }
}

void Input::setSolo(bool solo) {
    this->mainParent->soloInput(this->idChannel, solo);
    if (solo) {
        this->mainParent->muteInput(this->idChannel, false);
    }
}

void Input::setColor(Colour color, bool updateLevel) {
    this->colorJ = color;
    this->color.x = this->colorJ.getFloatRed();
    this->color.y = this->colorJ.getFloatGreen();
    this->color.z = this->colorJ.getFloatBlue();
    
    if (updateLevel) {
        this->vuMeter->setColor(this->colorJ);
    }
}

Colour Input::getColorJWithAlpha() const {
    if (this->mainParent->isSourceLevelShown) {
        return this->colorJ.withMultipliedAlpha(this->getAlpha());
    } else {
        return this->colorJ;
    }
}

float Input::getAlpha() const {
    if (this->mainParent->isSourceLevelShown) {
        return this->mainParent->getLevelsAlpha(this->idChannel - 1);
    } else {
        return 1.0f;
    }
}

void Input::draw() {
    float transpa = 0.75;

    // If not initalized, don't draw.
    if (this->gain == -1.0) { return; }

    // If isSourceLevelShown is on and alpha below 0.01, don't draw.
    if (this->mainParent->isSourceLevelShown && this->getAlpha() <= 0.01) { return; }

    // Draw 3D sphere.
    glPushMatrix();
    glTranslatef(this->center.x, this->center.y, this->center.z);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(2);

    if (this->mainParent->isSourceLevelShown) {
        glColor4f(this->color.x, this->color.y, this->color.z, this->getAlpha());
    } else {
        glColor4f(this->color.x, this->color.y, this->color.z, transpa);
    }

    glutSolidSphere(this->sizeT, 8, 8);
    glTranslatef(-this->center.x, -this->center.y, -this->center.z);

    if ((this->azimSpan != 0.0f || this->zeniSpan != 0.0f) && this->mainParent->isSpanShown) {
        if (this->mainParent->getModeSelected() == 1) {
            drawSpanLBAP(this->center.x, this->center.y, this->center.z);
        } else {
            drawSpan();
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

void Input::drawSpan() {
    int num = 8;
    float newazi, newele;
    glm::vec3 cart;

    glPointSize(4);
    glBegin(GL_POINTS);

    for (int i=0; i<num; i++) {
        float azidev = i * this->azimSpan * 0.5f * 0.42f;
        for (int j=0; j<2; j++) {
            if (j)
                newazi = this->azimuth + azidev;
            else
                newazi = this->azimuth - azidev;

            if (newazi > M_PI)
                newazi -= (M_PI * 2.0f);
            else if (newazi < -M_PI)
                newazi += (M_PI * 2.0f);

            if (this->mainParent->getModeSelected() == 1) {
                cart = this->polToCar3d(newazi, this->zenith);
            } else {
                cart = this->polToCar(newazi, this->zenith);
            }
            glVertex3f(cart.x, cart.y, cart.z);
            for (int k=0; k<4; k++) {
                float eledev = (k+1) * this->zeniSpan * 2.0f * 0.38f;
                for (int l=0; l<2; l++) {
                    if (l)
                        newele = this->zenith + eledev;
                    else
                        newele = this->zenith - eledev;
                    if (newele > (M_PI * 0.5f))
                        newele = (M_PI * 0.5f);
                    else if (newele < 0)
                        newele = 0;

                    if (this->mainParent->getModeSelected() == 1) {
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

float Input::getLevel() const {
    return this->mainParent->getLevelsIn(this->idChannel-1);
}

void Input::drawSpanLBAP(float x, float y, float z) {
    int num = 4;
    float ytmp;

    glPointSize(4);
    glBegin(GL_POINTS);

    // For the same elevation as the source position.
    for (int i=1; i<=num; i++) {
        float raddev = i / 4.f * this->azimSpan * 6.f;
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
    for (int j=0; j<num; j++) {
        float eledev = (j+1) / 2.f * this->zeniSpan * 15.f;
        for (int k=0; k<2; k++) {
            if (k) {
                ytmp = y + eledev;
                ytmp = ytmp > 10.f ? 10.f : ytmp;
            }
            else {
                ytmp = y - eledev;
                ytmp = ytmp < -10.f ? -10.f : ytmp;
            }
            for (int i=1; i<=num; i++) {
                float raddev = i / 4.f * this->azimSpan * 6.f;
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

void Input::updateValues(float az, float ze, float azS, float zeS, float radius, float g, int mode) {
    this->azimuth = az;
    this->zenith  = ze;
    this->azimSpan = azS;
    this->zeniSpan = zeS;
    this->radius = radius;
    this->gain = g;

    float factor = radius * 10.0f;

    if (mode == 1) {
        this->center.x = (factor * cosf(this->azimuth));
        this->center.z = (factor * sinf(this->azimuth));
        this->center.y = ((10.0f * cosf(this->zenith)) + (sizeT / 2.0f));
    } else {
        this->center.x = (factor * sinf(this->zenith) * cosf(this->azimuth));
        this->center.z = (factor * sinf(this->zenith) * sinf(this->azimuth));
        this->center.y = ((10.0f * cosf(this->zenith)) + (sizeT / 2.0f));
    }
}

void Input::updateValuesOld(float az, float ze, float azS, float zeS, float g) {
    if (az < 0) {
        this->azimuth = fabsf(az) * M_PI;
    } else {
        this->azimuth = (1.0f - az) * M_PI + M_PI;
    }
    this->zenith  = (M_PI2) - (M_PI * ze);
    
    this->azimSpan = azS;
    this->zeniSpan = zeS;
    this->gain = g;
    
    this->center.x = (10.0f * sinf(this->zenith) * cosf(this->azimuth));
    this->center.z = (10.0f * sinf(this->zenith) * sinf(this->azimuth));
    this->center.y = (10.0f * cosf(this->zenith)) + (this->sizeT / 2.0f);
}

void Input::sendDirectOutToClient(int id, int chn) {
    this->mainParent->setDirectOut(id, chn);
}

void Input::changeDirectOutChannel(int chn) {
    this->directOutChannel = chn;
}

void Input::setDirectOutChannel(int chn) {
    this->directOutChannel = chn;
    if (chn == 0) {
        this->vuMeter->directOut->setButtonText("-");
        return;
    } else {
        this->vuMeter->directOut->setButtonText(String(chn));
    }
}
