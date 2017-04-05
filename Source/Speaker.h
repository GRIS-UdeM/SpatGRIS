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


#ifndef Speaker_h
#define Speaker_h

#include <stdio.h>
#include <iostream>
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>

#include "../glm/glm.hpp"

#include "../JuceLibraryCode/JuceHeader.h"

#include "GrisLookAndFeel.h"
#include "ParentLevelComponent.h"


class MainContentComponent;
class LevelComponent;


using namespace std;

static const glm::vec3 colorSpeaker = glm::vec3(0.85, 0.86, 0.87);
static const glm::vec3 colorSpeakerSelect = glm::vec3(1.0, 0.66, 0.67);
static const glm::vec3 sizeSpeaker = glm::vec3(0.5, 0.5, 0.5);
static const glm::vec3 defaultCenter = glm::vec3(0, 0, 0);

static double GetFloatPrecision(double value, double precision)
{
    return (floor((value * pow(10, precision) + 0.5)) / pow(10, precision));
}


class Speaker : 
                public ParentLevelComponent
{
public:
    
    Speaker(MainContentComponent *parent = nullptr, int idS = 1);
    Speaker(MainContentComponent *parent = nullptr, int idS = -1,
            int outP = -1, glm::vec3 center = defaultCenter, glm::vec3 extents = sizeSpeaker);
    ~Speaker();
    
    bool isSelected();
    void selectSpeaker();
    void unSelectSpeaker();
    
    //ParentLevelComponent ===============================
    int getId(){ return this->outputPatch ;};
    float getLevel();
    void setMuted(bool mute);
    void setSolo(bool solo);
    void setColor(Colour color, bool updateLevel = false);
    void selectClick();
    LevelComponent * getVuMeter(){ return this->vuMeter; }

    //Normalise for user =================================
    void setBounds(const Rectangle<int> &newBounds);
    int getIdSpeaker();
    glm::vec3 getCoordinate();
    glm::vec3 getAziZenRad();
    void setCoordinate(glm::vec3 value);
    void setAziZenRad(glm::vec3 value);
    int getOutputPatch();
    void setOutputPatch(int value);

    
    
    //OpenGL ==============================================
    glm::vec3 getMin();
    glm::vec3 getMax();
    glm::vec3 getCenter();

    bool isValid();
    void fix();
    void draw() ;
    
    
private :

    void newPosition(glm::vec3 center, glm::vec3 extents = sizeSpeaker);
    void newSpheriqueCoord(glm::vec3 aziZenRad, glm::vec3 extents = sizeSpeaker);
    
    glm::vec3 min = glm::vec3(0,0,0);
    glm::vec3 max = glm::vec3(0,0,0);
    glm::vec3 center;
    glm::vec3 aziZenRad;
    glm::vec3 color = colorSpeaker;
    
    int idSpeaker = -1;
    int outputPatch = -1;
    
    bool selected = false;

    
    MainContentComponent *mainParent;
    LevelComponent *vuMeter;
    
    GrisLookAndFeel mGrisFeel;
    
};


#endif /* Speaker_h */
