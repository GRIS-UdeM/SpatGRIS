//
//  Input.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-16.
//
//

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

void Input::draw(){
    // Draw 3D sphere.
    glPushMatrix();
    glTranslatef(this->center.x, this->center.y, this->center.z);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2);
    glRotatef(90, 1, 0, 0);
    glColor3f(this->color.x, this->color.y, this->color.z);
    glutSolidSphere(sizeT, 6, 6);
    glTranslatef(-1*this->center.x, -1*this->center.y, -1*this->center.z);
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
