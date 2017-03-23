//
//  Speaker.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-02.
//
//

#ifndef Speaker_h
#define Speaker_h

#include <stdio.h>
#include <iostream>
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>

#include "../glm/glm.hpp"

#include "../JuceLibraryCode/JuceHeader.h"

#include "../../GrisCommonFiles/GrisLookAndFeel.h"
#include "ParentLevelComponent.h"

class MainContentComponent;
class LevelComponent;


static const glm::vec3 colorSpeaker = glm::vec3(0.85, 0.86, 0.87);
static const glm::vec3 colorSpeakerSelect = glm::vec3(1.0, 0.66, 0.67);
static const glm::vec3 sizeSpeaker = glm::vec3(0.5, 0.5, 0.5);
static const glm::vec3 defaultCenter = glm::vec3(0, 0, 0);

using namespace std;



static double GetFloatPrecision(double value, double precision)
{
    return (floor((value * pow(10, precision) + 0.5)) / pow(10, precision));
}

class Speaker : public Component,
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
    
    LevelComponent * getVuMeter(){ return this->vuMeter; }
    float getLevel();
    void setMuted(bool mute);
    //Normalise for user =================================
    void setBounds(const Rectangle<int> &newBounds);
    int getIdSpeaker();
    glm::vec3 getCoordinate();
    glm::vec3 getAziZenRad();
    void setCoordinate(glm::vec3 value);
    void setAziZenRad(glm::vec3 value);
    int getOutputPatch();
    void setOutputPatch(int value);
    int getId(){ return this->outputPatch ;};
    
    
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
