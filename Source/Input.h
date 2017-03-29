//
//  Input.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-16.
//
//

#ifndef Input_h
#define Input_h

#include <stdio.h>
#include <iostream>

#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>

#include "../glm/glm.hpp"

#include "../JuceLibraryCode/JuceHeader.h"

#include "../../GrisCommonFiles/GrisLookAndFeel.h"
#include "ParentLevelComponent.h"

using namespace std;

class MainContentComponent;
class LevelComponent;


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
    LevelComponent * getVuMeter(){ return this->vuMeter; }
    
    glm::vec3 getCenter();
    glm::vec3 getColor();
    float getAziMuth(){ return this->azimuth; }
    float getZenith(){ return this->zenith; }
    float getRad(){ return 10.0f; }
    
    void draw();
    void updateValues(float az, float ze, float azS, float zeS, float g);
    
private:
    int idChannel;
    
    float azimuth;
    float zenith;
    float azimSpan;
    float zeniSpan;
    float gain;
    
    float sizeT = 0.5f;
    glm::vec3 center;
    glm::vec3 color;
    
    LevelComponent *vuMeter;
    MainContentComponent * mainParent;
    GrisLookAndFeel * grisFeel;
};
#endif /* Input_h */
