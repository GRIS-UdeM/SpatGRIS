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

#ifndef SPEAKERVIEWCOMPONENT_H_INCLUDED
#define SPEAKERVIEWCOMPONENT_H_INCLUDED


#include "../JuceLibraryCode/JuceHeader.h"
#include "Resources/WavefrontObjParser.h"
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>
#include "../glm/glm.hpp"


#include <math.h>

using namespace std;

struct hitinfo {
    glm::vec2 lambda;
    int bi;
};

class SpeakerObj{
public:
    SpeakerObj(glm::vec3 pos = glm::vec3(0,0,0));
    ~SpeakerObj();
    
    void draw();
    
    void setPosition(glm::vec3 pos);
    glm::vec3 getPosition();
    
    glm::vec3 getMin(){ return glm::vec3(this->position.x-0.5f,this->position.y-0.5f,this->position.z-0.5f); }
    glm::vec3 getMax(){ return glm::vec3(this->position.x+0.5f,this->position.y+0.5f,this->position.z+0.5f); }
    

private:
    glm::vec3 position;
    glm::vec3 color = glm::vec3(0.85, 0.86, 0.87);
    
    const float sizex = 0.5f;
    const float sizey = 0.5f;
    const float sizez = 0.5f;

};

class SpeakerViewComponent : public OpenGLAppComponent {
public:
    //==============================================================================
    SpeakerViewComponent();
    
    ~SpeakerViewComponent();
    
    void initialise() override;
    
    void shutdown() override;
    

    void render() override;
    
    void paint (Graphics& g) override;
    void resized() override;
    
    void mouseDown(const MouseEvent& e)override;
    void mouseDrag (const MouseEvent& e)override;
    
    void mouseWheelMove(const MouseEvent& e,const MouseWheelDetails& wheel)override;

    

    
private:

    void drawBackground();
    void drawOriginGrid();
    void drawCube(float x, float y, float z);
    

    float camAngleX=35;
    float camAngleY=35;
    
    glm::vec3 startClick;
    glm::vec3 endClick;
    glm::vec2 posC;
    
    std::vector<SpeakerObj> listSpeaker;
    
    ComponentDragger myDragger;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerViewComponent)
};




#endif  // SPEAKERVIEWCOMPONENT_H_INCLUDED
