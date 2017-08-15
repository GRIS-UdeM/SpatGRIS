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

#ifndef Input_h
#define Input_h

#include <stdio.h>
#include <iostream>

#ifdef __linux__
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>
#endif

#include "../glm/glm.hpp"

#include "../JuceLibraryCode/JuceHeader.h"

#include "GrisLookAndFeel.h"
#include "ParentLevelComponent.h"


class MainContentComponent;
class LevelComponent;

using namespace std;


class Input :   public ParentLevelComponent
{

public :
    Input(MainContentComponent * parent, GrisLookAndFeel * feel, int id = 0);
    ~Input();
    
    //ParentLevelComponent ===============================
    int getId(){ return this->idChannel ;};
    float getLevel();
    void setMuted(bool mute);
    void setSolo(bool solo);
    void setColor(Colour color, bool updateLevel = false);
    void selectClick(){};
    LevelComponent * getVuMeter(){ return this->vuMeter; }

    void resetPosition();

    glm::vec3 getCenter();
    glm::vec3 getColor();
    glm::vec3 getNumberColor();
    Colour    getColorJ();
    Colour    getColorJWithAlpha();
    float getAlpha();
    float getAziMuth(){ return this->azimuth; }
    float getZenith(){ return this->zenith; }
    float getRad(){ return this->radius; }

    glm::vec3 polToCar(float azimuth, float zenith);
    
    float getAziMuthSpan(){ return this->azimSpan; }
    float getZenithSpan(){ return this->zeniSpan; }
    
    void draw();
    void updateValues(float az, float ze, float azS, float zeS, float heS, float g);
    void updateValuesOld(float az, float ze, float azS, float zeS, float g);

    bool isInput() { return true; }
    void changeDirectOutChannel(int chn);
    void setDirectOutChannel(int chn);
    int getDirectOutChannel() { return this->directOutChannel; };
    void sendDirectOutToClient(int id, int chn);
    
private:
    void drawSpan();
    
    int idChannel;
    int directOutChannel;
    
    float azimuth;
    float zenith;
    float radius;
    
    float azimSpan;
    float zeniSpan;
    float gain;

    float sizeT = 0.3f;
    glm::vec3 center;
    glm::vec3 color;
    Colour    colorJ;
    
    LevelComponent *vuMeter;
    MainContentComponent * mainParent;
    GrisLookAndFeel * grisFeel;

};
#endif /* Input_h */
