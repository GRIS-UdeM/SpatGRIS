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

const glm::vec3 colorSpeaker = glm::vec3(0.85, 0.86, 0.87);
const glm::vec3 colorSpeakerSelect = glm::vec3(1.0, 0.66, 0.67);
const glm::vec3 sizeSpeaker = glm::vec3(0.5, 0.5, 0.5);

class Speaker : public Component,
                public TextEditor::Listener
{
public:
    
    Speaker();
    Speaker(glm::vec3 center, glm::vec3 extents = sizeSpeaker);
    ~Speaker();
    
    glm::vec3 getMin();
    glm::vec3 getMax();
    glm::vec3 getCenter();
    
    bool isValid();
    void fix();
    
    bool isSelected();
    void selectSpeaker();
    void unSelectSpeaker();
    
    void draw() ;
    
private :

    float yAngle;
    float zAngle;
    
    glm::vec3 min = glm::vec3(0,0,0);
    glm::vec3 max = glm::vec3(0,0,0);
    glm::vec3 center;
    glm::vec3 color = colorSpeaker;
    
    bool selected = false;
};

#endif /* Speaker_h */
