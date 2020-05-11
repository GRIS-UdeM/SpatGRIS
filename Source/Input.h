/*
 This file is part of SpatGRIS2.
 
 Developers: Olivier Belanger, Nicolas Masson
 
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

#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include <iostream>

#if defined(__linux__)
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#elif defined(WIN32) || defined(_WIN64)
#include <windows.h>
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

class Input : public ParentLevelComponent
{
public :
    Input(MainContentComponent * parent, SmallGrisLookAndFeel * feel, int id = 0);
    ~Input() = default;
    
    // ParentLevelComponent
    int getId() { return this->idChannel; };
    int getButtonInOutNumber() { return this->idChannel; };
    float getLevel();
    void setMuted(bool mute);
    void setSolo(bool solo);
    void setColor(Colour color, bool updateLevel = false);
    void selectClick(bool select = true) {};
    LevelComponent * getVuMeter() { return this->vuMeter.get(); }

    void resetPosition();

    glm::vec3 getCenter();
    glm::vec3 getColor();
    glm::vec3 getNumberColor();
    Colour    getColorJ();
    Colour    getColorJWithAlpha();
    float getAlpha();
    float getAziMuth() { return this->azimuth; }
    float getZenith() { return this->zenith; }
    float getRadius() { return this->radius; }

    glm::vec3 polToCar(float azimuth, float zenith);
    glm::vec3 polToCar3d(float azimuth, float zenith);
    
    float getAziMuthSpan() { return this->azimSpan; }
    float getZenithSpan() { return this->zeniSpan; }

    float getGain() { return this->gain; }
    
    void draw();
    void updateValues(float az, float ze, float azS, float zeS, float radius, float g, int mode);
    void updateValuesOld(float az, float ze, float azS, float zeS, float g);

    bool isInput() { return true; }
    void changeDirectOutChannel(int chn);
    void setDirectOutChannel(int chn);
    int getDirectOutChannel() { return this->directOutChannel; };
    void sendDirectOutToClient(int id, int chn);

    MainContentComponent * mainParent;
    
private:
    void drawSpan();
    void drawSpanLBAP(float x, float y, float z);

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
    Colour colorJ;
    
    std::unique_ptr<LevelComponent> vuMeter{};
    SmallGrisLookAndFeel *grisFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Input);
};

#endif /* INPUT_H */
