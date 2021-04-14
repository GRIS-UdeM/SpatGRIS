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

#include "CartesianVector.h"
#include "PolarVector.h"
#include "macros.h"
struct SpeakerData;
enum class SpatMode;
struct SourceData;
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
#include "constants.hpp"

class MainContentComponent;
class SpeakerModel;

static float constexpr SCROLL_WHEEL_SPEED_MOUSE{ 1.8f };

//==============================================================================
class SpeakerViewComponent final : public juce::OpenGLAppComponent
{
public:
    static glm::vec3 const COLOR_SPEAKER{ 0.87f, 0.87f, 0.87f };
    static glm::vec3 const COLOR_DIRECT_OUT_SPEAKER{ 0.25f, 0.25f, 0.25f };
    static glm::vec3 const COLOR_SPEAKER_SELECT{ 1.00f, 0.64f, 0.09f };
    static glm::vec3 const SIZE_SPEAKER{ 0.5f, 0.5f, 0.5f };
    static glm::vec3 const DEFAULT_CENTER{ 0.0f, 0.0f, 0.0f };
    static auto constexpr MAX_RADIUS{ 10.0f };
    static auto constexpr SPACE_LIMIT{ MAX_RADIUS * LBAP_EXTENDED_RADIUS };

private:
    MainContentComponent & mMainContentComponent;

    bool mShowSphere = false;
    bool mShowNumber = false;
    bool mClickLeft = false;
    bool mControlOn = false;
    bool mHideSpeakers = false;
    bool mShowTriplets = false;

    PolarVector mCamVector{ radians_t{}, radians_t{}, 22.0f };

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
    static constexpr auto SPHERE_RADIUS = 0.3f;
    static constexpr auto HALF_SPHERE_RADIUS = SPHERE_RADIUS / 2.0f;
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
    void setHideSpeaker(bool const value) { mHideSpeakers = value; }
    void setShowTriplets(bool const value) { mShowTriplets = value; }
    void setNameConfig(juce::String const & name);

    void setCamPosition(CartesianVector const & position);
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
    [[nodiscard]] float rayCast(SpeakerData const & speaker) const;
    [[nodiscard]] bool speakerNearCam(glm::vec3 speak1, glm::vec3 speak2) const;

    void clickRay();

    void drawOriginGrid() const;
    void drawText(juce::String const & val,
                  CartesianVector const & position,
                  juce::Colour color,
                  float scale = 0.005f,
                  bool camLock = true) const;
    void drawTripletConnection() const;
    void drawSource(SourceData const & source, SpatMode const spatMode) const;
    //==============================================================================
    static void drawBackground();
    static void drawTextOnGrid(std::string const & val, glm::vec3 position, float scale = 0.003f);
    static void drawVbapSpan(SourceData const & source);
    static void drawLbapSpan(SourceData const & source);
    void drawSpeaker(SpeakerData const & speaker) const;
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerViewComponent)
}; // class SpeakerViewComponent
