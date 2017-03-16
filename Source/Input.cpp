//
//  Input.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-16.
//
//

#include "Input.h"


Input::Input(int id){
    this->idChannel = id;

}
Input::~Input(){

}


void Input::draw(){

    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1);
    glRotatef(90, 1, 0, 0);
    glColor3f(0.8, 0.2, 0.1);
    glutSolidSphere(0.5 ,5, 5);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glTranslatef(-1*position.x, -1*position.y, -1*position.z);
    glPopMatrix();
}


void Input::updateValues(float az, float ze, float azS, float zeS, float g){
    this->azimuth=(az/M_PI)*-10.0f;
    this->zenith=((ze-0.5f)/M_PI)*10.0f;
    this->azimSpan=azS;
    this->zeniSpan=zeS;
    this->gain = g;
    glm::vec3 nCenter;
    
    //aziZenRad.x = abs( (this->azimuth * M_PI ) / 180.0f) ;
    //aziZenRad.y = abs( ((-90.0f+this->zenith) * M_PI ) / 180.0f);
    
    nCenter.x = (10.0f  * sinf(this->zenith)*cosf(this->azimuth));
    nCenter.z = (10.0f * sinf(this->zenith)*sinf(this->azimuth));
    nCenter.y = (10.0f * cosf(this->zenith));
    this->position =nCenter;
}
