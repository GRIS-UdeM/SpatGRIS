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
#elif defined(__APPLE__)
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
    SmallGrisLookAndFeel mLookAndFeel;

    MainContentComponent & mMainContentComponent;

    glm::vec3 mMin{ 0.0f, 0.0f, 0.0f };
    glm::vec3 mMax{ 0.0f, 0.0f, 0.0f };
    glm::vec3 mCenter{};
    glm::vec3 mAziZenRad{};
    glm::vec3 mColor{ COLOR_SPEAKER };

    bool mDirectOut{ false };
    bool mSelected{ false };

    float mLevelColour{ 1.0f };
    float mGain{ 0.0f };
    float mHpCutoff{ 0.0f };

    int mDirectOutChannel{}; // Not used for output.

    int mId;
    int mOutputPatch;

    LevelComponent mVuMeter;

public:
    //==============================================================================
    Speaker(MainContentComponent & mainContentComponent,
            int id,
            int outputPatch,
            float azimuth,
            float zenith,
            float radius);
    //==============================================================================
    Speaker() = delete;
    ~Speaker() override = default;

    Speaker(Speaker const &) = delete;
    Speaker(Speaker &&) = delete;

    Speaker & operator=(Speaker const &) = delete;
    Speaker & operator=(Speaker &&) = delete;
    //==============================================================================
    [[nodiscard]] bool isSelected() const { return mSelected; }
    void selectSpeaker();
    void unSelectSpeaker();

    // ParentLevelComponent
    [[nodiscard]] int getId() const override
    {
        jassertfalse;
        return -1;
    } // Should not be used, use getIdSpeaker() instead.
    [[nodiscard]] int getButtonInOutNumber() const override { return mOutputPatch; };
    [[nodiscard]] float getLevel() const override;
    [[nodiscard]] float getAlpha() const;
    void setMuted(bool mute) override;
    void setSolo(bool solo) override;
    void setColor(juce::Colour /*color*/, bool /*updateLevel = false*/) override {}
    void selectClick(bool select = true) override;

    [[nodiscard]] LevelComponent const * getVuMeter() const override { return &mVuMeter; }
    [[nodiscard]] LevelComponent * getVuMeter() override { return &mVuMeter; }

    // Normalized for user
    void setSpeakerId(int const id) { mId = id; };
    [[nodiscard]] int getIdSpeaker() const { return mId; }
    void setCoordinate(glm::vec3 value);
    void normalizeRadius();
    void setAziZenRad(glm::vec3 value);
    [[nodiscard]] int getOutputPatch() const { return mOutputPatch; }
    void setOutputPatch(int value);
    void setGain(float const value) { mGain = value; }
    [[nodiscard]] float getGain() const { return mGain; }
    void setHighPassCutoff(float const value) { mHpCutoff = value; }
    [[nodiscard]] float getHighPassCutoff() const { return mHpCutoff; }
    [[nodiscard]] bool isDirectOut() const { return mDirectOut; }
    void setDirectOut(bool value);

    [[nodiscard]] glm::vec3 getCoordinate() const { return mCenter / 10.0f; }
    [[nodiscard]] glm::vec3 getAziZenRad() const;

    [[nodiscard]] bool isInput() const override { return false; }

    void changeDirectOutChannel(int /*chn*/) override{};
    void setDirectOutChannel(int /*chn*/) override{};
    [[nodiscard]] int getDirectOutChannel() const override { return 0; };
    void sendDirectOutToClient(int /*id*/, int /*chn*/) override{};

    // OpenGL
    [[nodiscard]] glm::vec3 getMin() const { return mMin; }
    [[nodiscard]] glm::vec3 getMax() const { return mMax; }
    [[nodiscard]] glm::vec3 getCenter() const { return mCenter; }

    [[nodiscard]] bool isValid() const;
    void fix();
    void draw();

private:
    //==============================================================================
    void newPosition(glm::vec3 center, glm::vec3 extents = SIZE_SPEAKER);
    void newSphericalCoord(glm::vec3 aziZenRad, glm::vec3 extents = SIZE_SPEAKER);
    //==============================================================================
    JUCE_LEAK_DETECTOR(Speaker)
};
