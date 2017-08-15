/*
 This file is part of ServerGris.
 
 Developers: Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Input.h"
#include "MainComponent.h"
#include "LevelComponent.h"

Input::Input(MainContentComponent * parent, GrisLookAndFeel * feel,int id)
{
    this->mainParent = parent;
    this->grisFeel = feel;
    this->idChannel = id;
    this->directOutChannel = 0;
    
    this->resetPosition();

    this->vuMeter = new LevelComponent(this, this->grisFeel);
    this->setColor(Colour::fromHSV(0, 1, 0.75, 1), true);
}

Input::~Input()
{
    delete this->vuMeter;
}

void Input::resetPosition()
{
    this->azimuth = M_PI4;
    this->zenith  = M_PI2;
    
    this->azimSpan = 0.0f;
    this->zeniSpan = 0.0f;;
    
    this->gain = 1.0f;
    
    this->center.x = (14.0f * sinf(this->zenith) * cosf(this->azimuth));
    this->center.z = (14.0f * sinf(this->zenith) * sinf(this->azimuth));
    this->center.y = (14.0f * cosf(this->zenith) + (sizeT/2.0f));
    this->radius = sqrt((this->center.x*this->center.x)+(this->center.y*this->center.y)+(this->center.z*this->center.z));
}

glm::vec3 Input::getCenter()
{
    return this->center;
}

float Input::getLevel()
{
    return this->mainParent->getLevelsIn(this->idChannel-1);
}

glm::vec3 Input::polToCar(float azimuth, float zenith)
{
    glm::vec3 cart;
    cart.x = (10.0f * sinf(zenith) * cosf(azimuth));
    cart.z = (10.0f * sinf(zenith) * sinf(azimuth));
    cart.y = ((10.0f * cosf(zenith)) + (sizeT/2.0f)) * 0.75; //heS;
    return cart;
}

void Input::setMuted(bool mute)
{
    this->mainParent->muteInput(this->idChannel, mute);
}
void Input::setSolo(bool solo)
{
    this->mainParent->soloInput(this->idChannel, solo);
}

void Input::setColor(Colour color, bool updateLevel)
{
    this->colorJ = color;

    this->color.x = this->colorJ.getFloatRed();
    this->color.y = this->colorJ.getFloatGreen();
    this->color.z = this->colorJ.getFloatBlue();
    
    if(updateLevel){
        this->vuMeter->setColor(this->colorJ);
    }
}

glm::vec3 Input::getColor()
{
    return this->color;
}

glm::vec3 Input::getNumberColor()
{
    return glm::vec3(this->color.x * 0.5, this->color.y * 0.5, this->color.z * 0.5);
}

Colour Input::getColorJ()
{
    return this->colorJ;
}

Colour Input::getColorJWithAlpha()
{
    if (this->mainParent->useAlpha) {
        return this->colorJ.withMultipliedAlpha(this->getAlpha());
    } else {
        return this->colorJ;
    }
}

float Input::getAlpha() {
    if (this->mainParent->useAlpha) {
        return this->mainParent->getLevelsAlpha(this->idChannel-1);
    } else {
        return 1.0f;
    }
}

void Input::draw()
{
    // Draw 3D sphere.
    glPushMatrix();
    glTranslatef(this->center.x, this->center.y, this->center.z);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  //GL_LINE
    glLineWidth(2);
    if (this->mainParent->useAlpha) {
        glColor4f(this->color.x, this->color.y, this->color.z, this->getAlpha());
    } else {
        glColor3f(this->color.x, this->color.y, this->color.z);
    }
    glutSolidSphere(this->sizeT, 8, 8);
    glTranslatef(-this->center.x, -this->center.y, -this->center.z);

    if (this->azimSpan != 0.0f || this->zeniSpan != 0.0f) {
        drawSpan();
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
            if (newazi > M_PI) { newazi -= (M_PI * 2.0f); }
            else if (newazi < -M_PI) { newazi += (M_PI * 2.0f); }

            cart = this->polToCar(newazi, this->zenith);
            glVertex3f(cart.x, cart.y, cart.z);
            //glTranslatef(cart.x, cart.y, cart.z);
            //glutSolidSphere(this->sizeT / 4, 8, 8);
            //glTranslatef(-cart.x, -cart.y, -cart.z);

            for (int k=0; k<3; k++) {
                float eledev = (k+1) * this->zeniSpan * 2.0f * 0.25f;
                for (int l=0; l<2; l++) {
                    if (l)
                        newele = this->zenith + eledev;
                    else
                        newele = this->zenith - eledev;
                    if (newele > (M_PI * 0.5f)) { newele = (M_PI * 0.5f); }
                    else if (newele < 0) { newele = 0; }

                    cart = this->polToCar(newazi, newele);
                    glVertex3f(cart.x, cart.y, cart.z);
                    //glTranslatef(cart.x, cart.y, cart.z);
                    //glutSolidSphere(this->sizeT / 4, 8, 8);
                    //glTranslatef(-cart.x, -cart.y, -cart.z);
                }
            }
        }
    }
    glEnd();
    glPointSize(1);
}

void Input::updateValues(float az, float ze, float azS, float zeS, float heS, float g)
{
    this->azimuth = az; //fmod(((az/M_PI)-M_PI)*-10.0f,(M2_PI));
    this->zenith  = ze; //(M_PI2) - (M_PI * ze);     //((ze-0.5f)/M_PI)*-10.0f;
    
    this->azimSpan = azS;
    this->zeniSpan = zeS;
    
    this->gain = g;
    
    this->center.x = (10.0f * sinf(this->zenith)*cosf(this->azimuth));
    this->center.z = (10.0f * sinf(this->zenith)*sinf(this->azimuth));
    this->center.y = ((10.0f * cosf(this->zenith)) + (sizeT/2.0f )) * 0.75; //heS;
    
    this->radius = sqrt((this->center.x*this->center.x)+(this->center.y*this->center.y)+(this->center.z*this->center.z));

    //re - compute
    //this->azimuth = ( (atan2(this->center.x, this->center.z) * 180.0f) / M_PI) +90.0f;
    //this->zenith = ( atan2(this->center.y, sqrt(this->center.x*this->center.x + this->center.z*this->center.z)) * 180.0f) / M_PI;
}

void Input::updateValuesOld(float az, float ze, float azS, float zeS, float g)
{
    this->azimuth = fmod(((az/M_PI)-M_PI)*-10.0f,(M2_PI));  //0 - 2_PI
    this->zenith  = (M_PI2) - (M_PI * ze); //0 - (PI/2)    //((ze-0.5f)/M_PI)*-10.0f;
    
    this->azimSpan = azS;
    this->zeniSpan = zeS;
    
    this->gain = g;
    
    this->center.x = (10.0f * sinf(this->zenith)*cosf(this->azimuth));
    this->center.z = (10.0f * sinf(this->zenith)*sinf(this->azimuth));
    this->center.y = (10.0f * cosf(this->zenith)) + (this->sizeT/2.0f);
    
    this->radius = sqrt((this->center.x*this->center.x)+(this->center.y*this->center.y)+(this->center.z*this->center.z));
}

void Input::sendDirectOutToClient(int id, int chn) {
    this->mainParent->setDirectOut(id, chn);
}

void Input::changeDirectOutChannel(int chn) {
    this->directOutChannel = chn;
}

void Input::setDirectOutChannel(int chn) {
    this->directOutChannel = chn;
    int nitems = this->vuMeter->directOut->getNumItems();
    if (chn == 0) {
        this->vuMeter->directOut->setSelectedItemIndex(0);
        return;
    }
    for (int i = 1; i < nitems; i++) {
        if (this->vuMeter->directOut->getItemText(i).compare(String(chn)) == 0) {
            this->vuMeter->directOut->setSelectedItemIndex(i);
        }
    }
}
