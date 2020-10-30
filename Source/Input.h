/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Belanger, Nicolas Masson

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

#include <iostream>

#include "macros.h"

DISABLE_WARNINGS
#if defined(__linux__)
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glut.h>
#elif defined(WIN32) || defined(_WIN64)
    #include <GL/freeglut.h>
    #include <windows.h>
#else
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGl/glu.h>
#endif

#include "../glm/glm.hpp"

#include <JuceHeader.h>
ENABLE_WARNINGS

#include "GrisLookAndFeel.h"
#include "LevelComponent.h"
#include "ParentLevelComponent.h"

class MainContentComponent;

//==============================================================================
class Input final : public ParentLevelComponent
{
public:
    Input(MainContentComponent & mainContentComponent, SmallGrisLookAndFeel & lookAndFeel, int id = 0);
    ~Input() final = default;
    //==============================================================================
    void setMuted(bool mute);
    void setSolo(bool solo);
    void selectClick(bool select = true){};
    void setColor(juce::Colour color, bool updateLevel = false);
    //==============================================================================
    MainContentComponent const & getMainContentComponent() const { return this->mainContentComponent; }
    MainContentComponent & getMainContentComponent() { return this->mainContentComponent; }

    LevelComponent const * getVuMeter() const { return &this->vuMeter; }
    LevelComponent * getVuMeter() { return &this->vuMeter; }

    int getId() const final { return this->idChannel; }
    int getButtonInOutNumber() const { return this->idChannel; }
    float getLevel() const;
    float getAlpha() const;
    float getAziMuth() const { return this->azimuth; }
    float getZenith() const { return this->zenith; }
    float getRadius() const { return this->radius; }
    float getAziMuthSpan() const { return this->azimSpan; }
    float getZenithSpan() const { return this->zeniSpan; }
    float getGain() const { return this->gain; }

    glm::vec3 getCenter() const { return this->center; }
    glm::vec3 getColor() const { return this->color; }
    glm::vec3 getNumberColor() const
    {
        return glm::vec3(this->color.x * 0.5, this->color.y * 0.5, this->color.z * 0.5);
    }
    juce::Colour getColorJ() const { return this->colorJ; }
    juce::Colour getColorJWithAlpha() const;
    //==============================================================================
    glm::vec3 polToCar(float azimuth, float zenith) const;
    glm::vec3 polToCar3d(float azimuth, float zenith) const;
    //==============================================================================
    void resetPosition();
    void draw();
    void updateValues(float az, float ze, float azS, float zeS, float radius, float g, int mode);
    void updateValuesOld(float az, float ze, float azS, float zeS, float g);
    //==============================================================================
    bool isInput() const final { return true; }
    void changeDirectOutChannel(int const chn) final { this->directOutChannel = chn; }
    void setDirectOutChannel(int chn) final;
    int getDirectOutChannel() const final { return this->directOutChannel; };
    void sendDirectOutToClient(int id, int chn) final;

    void drawSpan();
    void drawSpanLBAP(float x, float y, float z);

private:
    //==============================================================================
    MainContentComponent & mainContentComponent;
    SmallGrisLookAndFeel & lookAndFeel;

    int idChannel;
    int directOutChannel;

    float azimuth;
    float zenith;
    float radius;

    float azimSpan;
    float zeniSpan;
    float gain;
    float sizeT = 0.3f;

    glm::vec3 center;
    glm::vec3 color;
    juce::Colour colorJ;

    LevelComponent vuMeter;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Input);
};
