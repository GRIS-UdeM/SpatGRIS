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
#if defined(__APPLE__)
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

static int constexpr NUM_GRID_LINES{ 17 };
static float constexpr SCROLL_WHEEL_SPEED_MOUSE{ 1.8f };

//==============================================================================
class SpeakerViewComponent final : public juce::OpenGLAppComponent
{
    MainContentComponent & mMainContentComponent;

    bool mShowSphere = false;
    bool mShowNumber = false;
    bool mClickLeft = false;
    bool mControlOn = false;
    bool mHideSpeaker = false;
    bool mShowTriplets = false;

    float mCamAngleX = 80.0f;
    float mCamAngleY = 25.0f;
    float mDistance = 22.0f;

    float mSlowDownFactor = 3.0f; // Reduce the angle changing speed.
    float mDeltaClickX{};
    float mDeltaClickY{};

    double mRayClickX{};
    double mRayClickY{};

    double mDisplayScaling = 1.0;

    Ray mRay;

    glm::vec3 mCamPos;

    juce::String mNameConfig = "...";

    GLdouble mXs{};
    GLdouble mYs{};
    GLdouble mZs{};
    GLdouble mXe{};
    GLdouble mYe{};
    GLdouble mZe{};

public:
    //==============================================================================
    explicit SpeakerViewComponent(MainContentComponent & mainContentComponent);
    //==============================================================================
    SpeakerViewComponent() = delete;
    ~SpeakerViewComponent() override { shutdownOpenGL(); }

    SpeakerViewComponent(SpeakerViewComponent const &) = delete;
    SpeakerViewComponent(SpeakerViewComponent &&) = delete;

    SpeakerViewComponent & operator=(SpeakerViewComponent const &) = delete;
    SpeakerViewComponent & operator=(SpeakerViewComponent &&) = delete;
    //==============================================================================
    void setShowSphere(bool const value) { mShowSphere = value; }
    void setShowNumber(bool const value) { mShowNumber = value; }
    void setHideSpeaker(bool const value) { mHideSpeaker = value; }
    void setShowTriplets(bool const value) { mShowTriplets = value; }
    void setNameConfig(juce::String const & name);

    float getCamAngleX() const { return mCamAngleX; };
    float getCamAngleY() const { return mCamAngleY; };
    float getCamDistance() const { return mDistance; };

    void setCamPosition(float angleX, float angleY, float distance);
    //==============================================================================
    void initialise() override;
    void shutdown() override {}

    void render() override;
    void paint(juce::Graphics & g) override;

    void mouseDown(const juce::MouseEvent & e) override;
    void mouseDrag(const juce::MouseEvent & e) override;
    void mouseWheelMove(const juce::MouseEvent & e, const juce::MouseWheelDetails & wheel) override;

private:
    //==============================================================================
    [[nodiscard]] float rayCast(Speaker const * speaker) const;
    [[nodiscard]] bool speakerNearCam(glm::vec3 speak1, glm::vec3 speak2) const;

    void clickRay();

    void drawOriginGrid() const;
    void drawText(std::string const & val,
                  glm::vec3 position,
                  glm::vec3 color,
                  float scale = 0.005f,
                  bool camLock = true,
                  float alpha = 1.0f) const;
    void drawTripletConnection() const;
    //==============================================================================
    static void drawBackground();
    static void drawTextOnGrid(std::string const & val, glm::vec3 position, float scale = 0.003f);
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerViewComponent)
}; // class SpeakerViewComponent
