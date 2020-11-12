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
glm::vec3 const COLOR_SPEAKER{ 0.87f, 0.87f, 0.87f };
glm::vec3 const COLOR_DIRECT_OUT_SPEAKER{ 0.25f, 0.25f, 0.25f };
glm::vec3 const COLOR_SPEAKER_SELECT{ 1.00f, 0.64f, 0.09f };
glm::vec3 const SIZE_SPEAKER{ 0.5f, 0.5f, 0.5f };
glm::vec3 const DEFAULT_CENTER{ 0.0f, 0.0f, 0.0f };

constexpr float OVER = 0.02f;

//==============================================================================
class Speaker final : public ParentLevelComponent
{
    glm::vec3 mMin{ 0.0f, 0.0f, 0.0f };
    glm::vec3 mMax{ 0.0f, 0.0f, 0.0f };
    glm::vec3 mCenter;
    glm::vec3 mAziZenRad;
    glm::vec3 mColor{ COLOR_SPEAKER };

    bool mDirectOut = false;
    bool mSelected = false;

    float mLevelColour = 1.0f;
    float mGain = 0.0f;
    float mHpCutoff = 0.0f;

    MainContentComponent * mMainContentComponent;
    LevelComponent * mVuMeter;

    SmallGrisLookAndFeel mLookAndFeel;

    int mDirectOutChannel; // Not used for output.

    int mId = -1;
    int mOutputPatch = -1;

public:
    //==============================================================================
    explicit Speaker(MainContentComponent * parent = nullptr,
                     int idS = -1,
                     int outP = -1,
                     float azimuth = 0.0f,
                     float zenith = 0.0f,
                     float radius = 1.0f);
    //==============================================================================
    Speaker() = delete;
    ~Speaker() override;

    Speaker(Speaker const &) = delete;
    Speaker(Speaker &&) = delete;

    Speaker & operator=(Speaker const &) = delete;
    Speaker & operator=(Speaker &&) = delete;
    //==============================================================================
    [[nodiscard]] bool isSelected() const { return this->mSelected; }
    void selectSpeaker();
    void unSelectSpeaker();

    // ParentLevelComponent
    [[nodiscard]] int getId() const override { return -1; } // Should not be used, use getIdSpeaker() instead.
    [[nodiscard]] int getButtonInOutNumber() const override { return this->mOutputPatch; };
    [[nodiscard]] float getLevel() const override;
    [[nodiscard]] float getAlpha();
    void setMuted(bool mute) override;
    void setSolo(bool solo) override;
    void setColor(juce::Colour /*color*/, bool /*updateLevel = false*/) override {}
    void selectClick(bool select = true) override;

    [[nodiscard]] LevelComponent const * getVuMeter() const override { return this->mVuMeter; }
    [[nodiscard]] LevelComponent * getVuMeter() override { return this->mVuMeter; }

    // Normalized for user
    // void setBounds(const juce::Rectangle<int> & newBounds);
    void setSpeakerId(int const id) { this->mId = id; };
    [[nodiscard]] int getIdSpeaker() const { return this->mId; }
    void setCoordinate(glm::vec3 value);
    void normalizeRadius();
    void setAziZenRad(glm::vec3 value);
    [[nodiscard]] int getOutputPatch() const { return this->mOutputPatch; }
    void setOutputPatch(int value);
    void setGain(float const value) { this->mGain = value; }
    [[nodiscard]] float getGain() const { return this->mGain; }
    void setHighPassCutoff(float const value) { this->mHpCutoff = value; }
    [[nodiscard]] float getHighPassCutoff() const { return this->mHpCutoff; }
    [[nodiscard]] bool isDirectOut() const { return this->mDirectOut; }
    void setDirectOut(bool value);

    [[nodiscard]] glm::vec3 getCoordinate() const { return this->mCenter / 10.0f; }
    [[nodiscard]] glm::vec3 getAziZenRad() const;

    [[nodiscard]] bool isInput() const override { return false; }

    void changeDirectOutChannel(int /*chn*/) override{};
    void setDirectOutChannel(int /*chn*/) override{};
    [[nodiscard]] int getDirectOutChannel() const override { return 0; };
    void sendDirectOutToClient(int /*id*/, int /*chn*/) override{};

    // OpenGL
    [[nodiscard]] glm::vec3 getMin() const { return this->mMin; }
    [[nodiscard]] glm::vec3 getMax() const { return this->mMax; }
    [[nodiscard]] glm::vec3 getCenter() const { return this->mCenter; }

    [[nodiscard]] bool isValid() const;
    void fix();
    void draw();

private:
    //==============================================================================
    void newPosition(glm::vec3 center, glm::vec3 extents = SIZE_SPEAKER);
    void newSpheriqueCoord(glm::vec3 aziZenRad, glm::vec3 extents = SIZE_SPEAKER);
    //==============================================================================
    JUCE_LEAK_DETECTOR(Speaker)
};
