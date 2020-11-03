/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#pragma once

#include <cmath>

#include "macros.h"

DISABLE_WARNINGS
#if defined(__linux__)

#elif defined(WIN32) || defined(_WIN64)
    #include <GL/freeglut.h>
    #include <GL/gl.h>
    #include <windows.h>
#else
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/gl3.h>
    #include <OpenGl/glu.h>
#endif

#include "../glm/glm.hpp"

#include <JuceHeader.h>
ENABLE_WARNINGS

#include "Ray.h"

class MainContentComponent;
class Speaker;

static const int NbrGridLines = 17;
static const float ScroolWheelSpeedMouse = 1.8f;

//==============================================================================
class SpeakerViewComponent final : public juce::OpenGLAppComponent
{
public:
    SpeakerViewComponent(MainContentComponent * parent = nullptr) : mainParent(parent) {}
    ~SpeakerViewComponent() final { this->shutdownOpenGL(); }
    //==============================================================================
    void initialise() final;
    void shutdown() final {}

    void setShowSphere(bool value) { this->showShpere = value; }
    void setShowNumber(bool value) { this->showNumber = value; }
    void setHideSpeaker(bool value) { this->hideSpeaker = value; }
    void setShowTriplets(bool value) { this->showTriplets = value; }
    void setNameConfig(juce::String name)
    {
        this->nameConfig = name;
        this->repaint();
    }

    void render() final;
    void paint(juce::Graphics & g) final;

    void mouseDown(const juce::MouseEvent & e) final;
    void mouseDrag(const juce::MouseEvent & e) final;
    void mouseWheelMove(const juce::MouseEvent & e, const juce::MouseWheelDetails & wheel) final;

    float getCamAngleX() const { return this->camAngleX; };
    float getCamAngleY() const { return this->camAngleY; };
    float getCamDistance() const { return this->distance; };

    void setCamPosition(float angleX, float angleY, float distance);

private:
    //==============================================================================
    float raycast(Speaker * speaker) const;
    bool speakerNearCam(glm::vec3 speak1, glm::vec3 speak2) const;

    void clickRay();
    void drawBackground() const;
    void drawOriginGrid() const;
    void drawText(std::string val,
                  glm::vec3 position,
                  glm::vec3 color,
                  float scale = 0.005f,
                  bool camLock = true,
                  float alpha = 1.0f) const;
    void drawTextOnGrid(std::string val, glm::vec3 position, float scale = 0.003f) const;

    void drawTrippletConn() const;
    //==============================================================================
    MainContentComponent * mainParent;

    bool showShpere = false;
    bool showNumber = false;
    bool clickLeft = false;
    bool controlOn = false;
    bool hideSpeaker = false;
    bool showTriplets = false;

    float camAngleX = 80.0f;
    float camAngleY = 25.0f;
    float distance = 22.0f;

    float slowDownFactor = 3.0f; // Reduce the angle changing speed.
    float deltaClickX;
    float deltaClickY;

    double rayClickX;
    double rayClickY;

    double displayScaling = 1.0;

    Ray ray;

    glm::vec3 camPos;

    juce::String nameConfig = "...";

    GLdouble xS, yS, zS = 0;
    GLdouble xE, yE, zE = 0;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpeakerViewComponent)
};
