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
#include "ParentLevelComponent.h"

class LevelComponent;
class MainContentComponent;

//==============================================================================
struct Triplet {
    int id1;
    int id2;
    int id3;
};

//==============================================================================
glm::vec3 const ColorSpeaker = glm::vec3(0.87, 0.87, 0.87);
glm::vec3 const ColorDirectOutSpeaker = glm::vec3(0.25, 0.25, 0.25);
glm::vec3 const ColorSpeakerSelect = glm::vec3(1.00, 0.64, 0.09);
glm::vec3 const SizeSpeaker = glm::vec3(0.5, 0.5, 0.5);
glm::vec3 const DefaultCenter = glm::vec3(0, 0, 0);

constexpr float Over = 0.02f;

//==============================================================================
class Speaker final : public ParentLevelComponent
{
public:
    Speaker(MainContentComponent * parent = nullptr,
            int idS = -1,
            int outP = -1,
            float azimuth = 0.0f,
            float zenith = 0.0f,
            float radius = 1.0f);
    ~Speaker() override;
    //==============================================================================
    bool isSelected() const { return this->selected; }
    void selectSpeaker();
    void unSelectSpeaker();

    // ParentLevelComponent
    int getId() const override { return -1; } // Should not be used, use getIdSpeaker() instead.
    int getButtonInOutNumber() const override { return this->outputPatch; };
    float getLevel() const override;
    float getAlpha();
    void setMuted(bool mute) override;
    void setSolo(bool solo) override;
    void setColor(juce::Colour /*color*/, bool /*updateLevel = false*/) override {}
    void selectClick(bool select = true) override;

    LevelComponent const * getVuMeter() const override { return this->vuMeter; }
    LevelComponent * getVuMeter() override { return this->vuMeter; }

    // Normalized for user
    void setBounds(const juce::Rectangle<int> & newBounds);
    void setSpeakerId(int const id) { this->idSpeaker = id; };
    int getIdSpeaker() const { return this->idSpeaker; }
    void setCoordinate(glm::vec3 value);
    void normalizeRadius();
    void setAziZenRad(glm::vec3 value);
    int getOutputPatch() const { return this->outputPatch; }
    void setOutputPatch(int value);
    void setGain(float const value) { this->gain = value; }
    float getGain() const { return this->gain; }
    void setHighPassCutoff(float const value) { this->hpCutoff = value; }
    float getHighPassCutoff() const { return this->hpCutoff; }
    bool isDirectOut() const { return this->directOut; }
    void setDirectOut(bool value);

    glm::vec3 getCoordinate() const { return this->center / 10.0f; }
    glm::vec3 getAziZenRad() const
    {
        return glm::vec3(this->aziZenRad.x, this->aziZenRad.y, this->aziZenRad.z / 10.0f);
    }

    bool isInput() const override { return false; }

    void changeDirectOutChannel(int /*chn*/) override{};
    void setDirectOutChannel(int /*chn*/) override{};
    int getDirectOutChannel() const override { return 0; };
    void sendDirectOutToClient(int /*id*/, int /*chn*/) override{};

    // OpenGL
    glm::vec3 getMin() const { return this->min; }
    glm::vec3 getMax() const { return this->max; }
    glm::vec3 getCenter() const { return this->center; }

    bool isValid() const
    {
        return (this->min.x < this->max.x && this->min.y < this->max.y && this->min.z < this->max.z);
    }
    void fix();
    void draw();
    //==============================================================================
    int idSpeaker = -1;
    int outputPatch = -1;

private:
    //==============================================================================
    void newPosition(glm::vec3 center, glm::vec3 extents = SizeSpeaker);
    void newSpheriqueCoord(glm::vec3 aziZenRad, glm::vec3 extents = SizeSpeaker);
    //==============================================================================
    glm::vec3 min = glm::vec3(0, 0, 0);
    glm::vec3 max = glm::vec3(0, 0, 0);
    glm::vec3 center;
    glm::vec3 aziZenRad;
    glm::vec3 color = ColorSpeaker;

    bool directOut = false;
    bool selected = false;

    float levelColour = 1.0f;
    float gain = 0.0f;
    float hpCutoff = 0.0f;

    MainContentComponent * mainParent;
    LevelComponent * vuMeter;

    SmallGrisLookAndFeel mGrisFeel;

    int directOutChannel; // Not used for output.
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Speaker);
};
