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

#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>

#include <math.h>

#include "../JuceLibraryCode/JuceHeader.h"
#include "Resources/WavefrontObjParser.h"
#include "../glm/glm.hpp"

#include "ToolsGL.h"

class ToolsGL;

using namespace std;

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
    void drawText( string val, glm::vec3 position, bool camLock = true);
    void drawCube(float x, float y, float z);
    

    float camAngleX=35;
    float camAngleY=35;
    float distance = 12.0f;
    
    float deltaClickX;
    float deltaClickY;
    
    Ray r;
    std::vector<Speaker *> listSpeaker;
    glm::vec3 camPos;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerViewComponent)
};

#endif  // SPEAKERVIEWCOMPONENT_H_INCLUDED
