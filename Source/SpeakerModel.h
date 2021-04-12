/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
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
#include "StrongTypes.hpp"
#include "VuMeterComponent.h"
#include "VuMeterModel.h"

class MainContentComponent;
enum class PortState;

//==============================================================================
struct Triplet {
    output_patch_t id1;
    output_patch_t id2;
    output_patch_t id3;
};

//==============================================================================
glm::vec3 const COLOR_SPEAKER{ 0.87f, 0.87f, 0.87f };
glm::vec3 const COLOR_DIRECT_OUT_SPEAKER{ 0.25f, 0.25f, 0.25f };
glm::vec3 const COLOR_SPEAKER_SELECT{ 1.00f, 0.64f, 0.09f };
glm::vec3 const SIZE_SPEAKER{ 0.5f, 0.5f, 0.5f };
glm::vec3 const DEFAULT_CENTER{ 0.0f, 0.0f, 0.0f };

constexpr float OVER = 0.02f;

//==============================================================================
class SpeakerModel
{
    SmallGrisLookAndFeel & mLookAndFeel;

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

    output_patch_t mOutputPatch;

public:
    //==============================================================================
    SpeakerModel(MainContentComponent & mainContentComponent,
                 SmallGrisLookAndFeel & smallGrisLookAndFeel,
                 output_patch_t outputPatch,
                 float azimuth,
                 float zenith,
                 float radius);
    //==============================================================================
    SpeakerModel() = delete;
    ~SpeakerModel() = default;

    SpeakerModel(SpeakerModel const &) = delete;
    SpeakerModel(SpeakerModel &&) = delete;

    SpeakerModel & operator=(SpeakerModel const &) = delete;
    SpeakerModel & operator=(SpeakerModel &&) = delete;
    //==============================================================================
    [[nodiscard]] bool isSelected() const { return mSelected; }
    void selectSpeaker();
    void unSelectSpeaker();

    // Should not be used, use getIdSpeaker() instead.
    [[nodiscard]] float getAlpha() const;
    void setState(PortState state);

    // Normalized for user
    void setCoordinate(glm::vec3 value);
    void normalizeRadius();
    void setAziZenRad(glm::vec3 value);
    [[nodiscard]] output_patch_t getOutputPatch() const { return mOutputPatch; }
    void setOutputPatch(output_patch_t const value);
    void setGain(float const value) { mGain = value; }
    [[nodiscard]] float getGain() const { return mGain; }
    void setHighPassCutoff(float const value) { mHpCutoff = value; }
    [[nodiscard]] float getHighPassCutoff() const { return mHpCutoff; }
    [[nodiscard]] bool isDirectOut() const { return mDirectOut; }
    void setDirectOut(bool value);

    [[nodiscard]] glm::vec3 getCartesianCoords() const { return mCenter / 10.0f; }
    [[nodiscard]] glm::vec3 getPolarCoords() const;

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
    JUCE_LEAK_DETECTOR(SpeakerModel)
};
