/*
 This file is part of spatServerGRIS.
 
 Developers: Nicolas Masson
 
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
    
    this->azimuth = M_PI4;
    this->zenith  = M_PI2;
    
    this->azimSpan = 0.0f;
    this->zeniSpan = 0.0f;;
    
    this->gain = 1.0f;
    
    this->center.x = (14.0f * sinf(this->zenith) * cosf(this->azimuth));
    this->center.z = (14.0f * sinf(this->zenith) * sinf(this->azimuth));
    this->center.y = (14.0f * cosf(this->zenith) + (sizeT/2.0f));
    this->radius = sqrt((this->center.x*this->center.x)+(this->center.y*this->center.y)+(this->center.z*this->center.z));
    
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
    this->colorJ = color;
    this->color.x = this->colorJ.getFloatRed();
    this->color.y = this->colorJ.getFloatGreen();
    this->color.z = this->colorJ.getFloatBlue();
    
    if(updateLevel){
        this->vuMeter->setColor(this->colorJ);
    }
}
glm::vec3 Input::getColor(){
    return this->color;
}
Colour Input::getColorJ(){
    return this->colorJ;
}

void Input::draw(){
    // Draw 3D sphere.
    glPushMatrix();
    glTranslatef(this->center.x, this->center.y, this->center.z);
    //glRotatef(90, 1, 0, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  //GL_LINE
    glLineWidth(2);
    glColor3f(this->color.x, this->color.y, this->color.z);
    glutSolidSphere(sizeT, 8, 8);
    glTranslatef(-this->center.x, -this->center.y, -this->center.z);
    
    drawSpan();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

void Input::drawSpan()
{
    if(this->azimSpan == 0.0f && this->zeniSpan == 0.0f){
        return;
    }
    
    float radCir = sqrt((this->center.x*this->center.x)+(this->center.z*this->center.z));
    float angRad = -atan2(( - this->center.z * 10.0f), (this->center.x * 10.0f ));
    float angDeg = (angRad* 180.0f)/M_PI ;

    
    glTranslatef(0, this->center.y, 0);
    glRotatef((360.0f-angDeg) + (this->azimSpan* 90), 0, 1, 0);
    
    glPointSize(8);
    glBegin(GL_POINTS);
    for(int i = 0; i <= this->azimSpan * 90 ; i+=6){
        double angle = (M2_PI * i / 180);
        glVertex3f(radCir * cos(angle), 0.0f, radCir * sin(angle));
    }
    glEnd();
    
    glRotatef(-((360.0f-angDeg) + (this->azimSpan* 90)), 0, 1, 0);
    glTranslatef(0, -this->center.y, 0);
    

    /*int y = 0;
    for(int i = 1; i <= this->zeniSpan*20; ++i){
        
        float radC = sqrt((this->center.x*this->center.x)+(this->center.z*this->center.z)+(-y*y));//radCir -  (sinf((M_PI * (i*9.0f) / 180)))*10.0f ;
        float yH = (this->center.y  + y); //(sinf((M_PI * (i*9.0f) / 180)))*10.0f);

        if(yH > 11.0f || radC < 0.0f){
            //break;
            //yH = (this->center.y +  (sinf((M_PI * (-i*9.0f) / 180)))*10.0f);
        }
        glTranslatef(0, yH, 0);
        glRotatef((360.0f-angDeg) + (this->azimSpan* 90), 0, 1, 0);

        glPointSize(8);
        glBegin(GL_POINTS);
        for(int i = 0; i <= this->azimSpan * 90 ; i+=6){
            double angle = (M2_PI * i / 180);
            glVertex3f(radC * cos(angle), 0.0f, radC * sin(angle));
        }
        glEnd();

        glRotatef(-((360.0f-angDeg) + (this->azimSpan* 90)), 0, 1, 0);
        glTranslatef(0, -yH, 0);
        y+=1;
        
    }
    
    for(int i = 1; i  <= (this->zeniSpan*20) ; i++){
        
        float radC = sqrt((this->center.x*this->center.x)+(this->center.z*this->center.z)+(i*i)); //radCir + (i);
        float yH = (this->center.y - i);//  (sinf((M_PI * ((i)*9.0f) / 180)))*10.0f);
        
        if(yH < 0.0f || radC > 11.0f){
            break;
            //yH = (this->center.y +  (sinf((M_PI * (-i*9.0f) / 180)))*10.0f);
        }
        glTranslatef(0, yH, 0);
        glRotatef((360.0f-angDeg) + (this->azimSpan* 90), 0, 1, 0);
        
        glPointSize(8);
        glBegin(GL_POINTS);
        for(int i = 0; i <= this->azimSpan * 90 ; i+=6){
            double angle = (M2_PI * i / 180);
            glVertex3f(radC * cos(angle), 0.0f, radC * sin(angle));
        }
        glEnd();
        
        glRotatef(-((360.0f-angDeg) + (this->azimSpan* 90)), 0, 1, 0);
        glTranslatef(0, -yH, 0);
        
    }*/
}

void Input::updateValues(float az, float ze, float azS, float zeS, float heS, float g){
    
    this->azimuth = az; //fmod(((az/M_PI)-M_PI)*-10.0f,(M2_PI));
    this->zenith  = ze; //(M_PI2) - (M_PI * ze);     //((ze-0.5f)/M_PI)*-10.0f;
    
    this->azimSpan = azS;
    this->zeniSpan = zeS;
    
    this->gain = g;
    
    this->center.x = (10.0f * sinf(this->zenith)*cosf(this->azimuth));
    this->center.z = (10.0f * sinf(this->zenith)*sinf(this->azimuth));
    this->center.y = ((10.0f * cosf(this->zenith)) + (sizeT/2.0f )) * heS;
    
    this->radius = sqrt((this->center.x*this->center.x)+(this->center.y*this->center.y)+(this->center.z*this->center.z));
}

void Input::updateValuesOld(float az, float ze, float azS, float zeS, float g){

    this->azimuth = fmod(((az/M_PI)-M_PI)*-10.0f,(M2_PI));
    this->zenith  = (M_PI2) - (M_PI * ze);     //((ze-0.5f)/M_PI)*-10.0f;
    
    this->azimSpan = azS;
    this->zeniSpan = zeS;
    
    this->gain = g;
    
    this->center.x = (10.0f * sinf(this->zenith)*cosf(this->azimuth));
    this->center.z = (10.0f * sinf(this->zenith)*sinf(this->azimuth));
    this->center.y = (10.0f * cosf(this->zenith)) + (sizeT/2.0f );
    
    this->radius = sqrt((this->center.x*this->center.x)+(this->center.y*this->center.y)+(this->center.z*this->center.z));
}
