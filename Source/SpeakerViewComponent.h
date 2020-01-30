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

#ifndef SPEAKERVIEWCOMPONENT_H_INCLUDED
#define SPEAKERVIEWCOMPONENT_H_INCLUDED

#include <math.h>
#ifdef __linux__
//#include <GL/gl.h>
//#include <GL/glu.h>
//#include <GLES3/gl3.h>
//#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <OpenGL/gl3.h>
#include <GLUT/glut.h>
#endif

#include "../JuceLibraryCode/JuceHeader.h"

#include "../glm/glm.hpp"

#include "Ray.h"

class MainContentComponent;
class Speaker;

using namespace std;

static const int    NbrGridLines = 17;
static const float  ScroolWheelSpeedMouse = 1.8f;

class SpeakerViewComponent : public OpenGLAppComponent {
public:
    SpeakerViewComponent(MainContentComponent *parent = nullptr);
    ~SpeakerViewComponent();
    
    void initialise() override;
    void shutdown() override;
    
    void setShowSphere(bool value) { this->showShpere = value; }
    void setShowNumber(bool value) { this->showNumber = value; }
    void setHighPerfor(bool value) { this->highPerf = value; }
    void setHideSpeaker(bool value) { this->hideSpeaker = value; }
    void setShowTriplets(bool value) { this->showTriplets = value; }
    void setNameConfig(String name) { this->nameConfig = name; this->repaint(); }
    
    void render() override;
    void paint(Graphics& g) override;
    
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;

    float getCamAngleX() { return this->camAngleX; };
    float getCamAngleY() { return this->camAngleY; };
    float getCamDistance() { return this->distance; };

    void setCamPosition(float angleX, float angleY, float distance);
    
private:
    
    float raycast(Speaker *speaker);
    bool speakerNearCam(glm::vec3 speak1, glm::vec3 speak2);
    
    void clickRay();
    void drawBackground();
    void drawOriginGrid();
    void drawText( string val, glm::vec3 position, glm::vec3 color, float scale = 0.005f, bool camLock = true, float alpha = 1.0f);
    void drawTextOnGrid( string val, glm::vec3 position,float scale = 0.003f);
    
    void drawTrippletConn();
    
    bool showShpere = false;
    bool showNumber = false;
    bool highPerf   = false;
    bool clickLeft  = false;
    bool controlOn  = false;
    bool hideSpeaker = false;
    bool showTriplets = false;
    
    float camAngleX= 80.0f;
    float camAngleY= 25.0f;
    float distance = 22.0f;

    float slowDownFactor = 3.0f; // Reduce the angle changing speed.
    float deltaClickX;
    float deltaClickY;
    
    double rayClickX;
    double rayClickY;

    double displayScaling = 1.0;
    
    Ray ray;
    MainContentComponent *mainParent;
    glm::vec3 camPos;

    String nameConfig = "...";
    
    GLdouble xS, yS, zS = 0;
    GLdouble xE, yE, zE = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerViewComponent)
};

#endif  // SPEAKERVIEWCOMPONENT_H_INCLUDED
