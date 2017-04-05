/*
 This file is part of spatServerGRIS.
 
 spatServerGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 spatServerGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with spatServerGRIS.  If not, see <http://www.gnu.org/licenses/>.
 */



#include "Input.h"
#include "MainComponent.h"
#include "LevelComponent.h"

Input::Input(MainContentComponent * parent, GrisLookAndFeel * feel,int id){
    this->mainParent = parent;
    this->grisFeel = feel;
    this->idChannel = id;
    
    this->vuMeter = new LevelComponent(this, this->grisFeel);
}
Input::~Input(){
    delete this->vuMeter;
}

glm::vec3 Input::getCenter(){
    return this->center;
}

float Input::getLevel(){
    return this->mainParent->getLevelsIn(this->idChannel-1);
}
void Input::setMuted(bool mute){
    this->mainParent->muteInput(this->idChannel, mute);
}
void Input::setSolo(bool solo){
    this->mainParent->soloInput(this->idChannel, solo);
}
void Input::setColor(Colour color, bool updateLevel){
    this->color.x = color.getFloatRed();
    this->color.y = color.getFloatGreen();
    this->color.z = color.getFloatBlue();
    if(updateLevel){
        this->vuMeter->setColor(color);
    }
}
glm::vec3 Input::getColor(){
    return this->color;
}

void Input::draw(){
    // Draw 3D sphere.
    glPushMatrix();
    glTranslatef(this->center.x, this->center.y, this->center.z);
    glRotatef(90, 1, 0, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2);
    glColor3f(this->color.x, this->color.y, this->color.z);
    glutSolidSphere(sizeT, 6, 6);
    glTranslatef(-this->center.x, -this->center.y, -this->center.z);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    glPopMatrix();
}


void Input::updateValues(float az, float ze, float azS, float zeS, float g){

    this->azimuth=((az/M_PI)-M_PI)*-10.0f;
    this->zenith=((ze-0.5f)/M_PI)*-10.0f;
    
    this->azimSpan=(azS*M_PI);
    this->zeniSpan=zeS;
    
    //cout << this->azimSpan << " <> "<< zeS<< endl;
    this->gain = g;
    
    this->center.x = (10.0f * sinf(this->zenith)*cosf(this->azimuth));
    this->center.z = (10.0f * sinf(this->zenith)*sinf(this->azimuth));
    this->center.y = (10.0f * cosf(this->zenith)) + (sizeT/2.0f );

}
