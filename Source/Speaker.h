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


static const glm::vec3 colorSpeaker = glm::vec3(0.85, 0.86, 0.87);
static const glm::vec3 colorSpeakerSelect = glm::vec3(1.0, 0.66, 0.67);
static const glm::vec3 sizeSpeaker = glm::vec3(0.5, 0.5, 0.5);

using namespace std;


static double GetFloatPrecision(double value, double precision)
{
    return (floor((value * pow(10, precision) + 0.5)) / pow(10, precision));
}

class Speaker : public Component,
                public TextEditor::Listener
{
public:
    
    Speaker();
    Speaker(glm::vec3 center, glm::vec3 extents = sizeSpeaker);
    ~Speaker();
    
    bool isSelected();
    void selectSpeaker();
    void unSelectSpeaker();
    
    //Normalise for user =================================
    glm::vec3 getCoordinate();
    glm::vec3 getAziZenRad();
    
    
    
    //OpenGL ==============================================
    glm::vec3 getMin();
    glm::vec3 getMax();
    glm::vec3 getCenter();

    bool isValid();
    void fix();
    void draw() ;
    
    //Juce =================================================
    void focusOfChildComponentChanged (FocusChangeType cause);
    void focusLost (FocusChangeType cause);

    void textEditorFocusLost (TextEditor &textEditor);
    void textEditorReturnKeyPressed (TextEditor &textEditor);
    void paint (Graphics& g);
    
private :

    void newPosition(glm::vec3 center, glm::vec3 extents = sizeSpeaker);
    void newSpheriqueCoord(glm::vec3 aziZenRad, glm::vec3 extents = sizeSpeaker);
    
    glm::vec3 min = glm::vec3(0,0,0);
    glm::vec3 max = glm::vec3(0,0,0);
    glm::vec3 center;
    glm::vec3 aziZenRad;
    glm::vec3 color = colorSpeaker;
    
    bool selected = false;
    
    Label *label;
    TextEditor *teCenterX;
    TextEditor *teCenterY;
    TextEditor *teCenterZ;
    
    TextEditor *teAzimuth;
    TextEditor *teZenith;
    TextEditor *teRadius;
    
    
    GrisLookAndFeel mGrisFeel;
};

extern vector<Speaker *> listSpeaker;
#endif /* Speaker_h */
