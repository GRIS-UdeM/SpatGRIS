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

#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>
#include "../glm/glm.hpp"

#include <math.h>

#include "Ray.h"


class MainContentComponent;
class Speaker;


using namespace std;


static const int nbrGridLines = 16;


class SpeakerViewComponent : public OpenGLAppComponent {
public:
    //==============================================================================
    SpeakerViewComponent(MainContentComponent *parent = nullptr);
    
    ~SpeakerViewComponent();
    
    void initialise() override;
    
    void shutdown() override;
    

    void setNameConfig(String name){ this->nameConfig = name; }
    void render() override;
    
    void paint (Graphics& g) override;
    void resized() override;
    
    void mouseDown(const MouseEvent& e)override;
    void mouseDrag (const MouseEvent& e)override;
    
    void mouseWheelMove(const MouseEvent& e,const MouseWheelDetails& wheel)override;
    
    
private:
    
    float raycast(Speaker *speaker);
    bool speakerNearCam(glm::vec3 speak1, glm::vec3 speak2, glm::vec3 cam);
    
    void drawBackground();
    void drawOriginGrid();
    void drawText( string val, glm::vec3 position, bool camLock = true);
    void drawCube(float x, float y, float z);
    

    float camAngleX=35;
    float camAngleY=35;
    float distance = 12.0f;
    
    float deltaClickX;
    float deltaClickY;
    
    Ray ray;
    MainContentComponent *mainParent;
    glm::vec3 camPos;
    glm::vec4 perspectivCam;
    String nameConfig = "...";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerViewComponent)
};

#endif  // SPEAKERVIEWCOMPONENT_H_INCLUDED
