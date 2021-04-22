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
#include "LogicStrucs.hpp"
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

#define ASSERT_OPEN_GL_THREAD jassert(juce::Thread::getCurrentThread()->getThreadName() == "Pool")

class MainContentComponent;
class SpeakerModel;

static float constexpr SCROLL_WHEEL_SPEED_MOUSE{ 1.8f };

//==============================================================================
class SpeakerViewComponent final : public juce::OpenGLAppComponent
{
public:
    static const juce::Colour COLOR_SPEAKER;
    static const juce::Colour COLOR_DIRECT_OUT_SPEAKER;
    static const juce::Colour COLOR_SPEAKER_SELECT;
    static constexpr float SIZE_SPEAKER{ 0.05f };
    static auto constexpr MAX_RADIUS{ 1.0f };
    static auto constexpr SPACE_LIMIT{ MAX_RADIUS * LBAP_EXTENDED_RADIUS };

private:
    MainContentComponent & mMainContentComponent;
    juce::CriticalSection mLock{};

    ViewportData mData{};

    double mDisplayScaling = 1.0;

    Ray mRay;

    GLdouble mXs{};
    GLdouble mYs{};
    GLdouble mZs{};
    GLdouble mXe{};
    GLdouble mYe{};
    GLdouble mZe{};

public:
    static constexpr auto SPHERE_RADIUS = 0.03f;
    static constexpr auto HALF_SPHERE_RADIUS = SPHERE_RADIUS / 2.0f;
    //==============================================================================
    explicit SpeakerViewComponent(MainContentComponent & mainContentComponent)
        : mMainContentComponent(mainContentComponent)
    {
    }
    ~SpeakerViewComponent() override { shutdownOpenGL(); }

    SpeakerViewComponent(SpeakerViewComponent const &) = delete;
    SpeakerViewComponent(SpeakerViewComponent &&) = delete;

    SpeakerViewComponent & operator=(SpeakerViewComponent const &) = delete;
    SpeakerViewComponent & operator=(SpeakerViewComponent &&) = delete;
    //==============================================================================
    auto & getData() noexcept { return mData; }
    auto const & getData() const noexcept { return mData; }

    void setConfig(ViewportConfig const & config);
    void setCameraPosition(CartesianVector const & position) noexcept;

    auto const & getLock() const noexcept { return mLock; }
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
    [[nodiscard]] float rayCast(CartesianVector const & speakerPosition) const;
    [[nodiscard]] bool speakerNearCam(CartesianVector const & speak1, CartesianVector const & speak2) const;

    void clickRay();

    void drawOriginGrid() const;
    void drawText(juce::String const & val,
                  CartesianVector const & position,
                  juce::Colour color,
                  float scale = 0.005f,
                  bool camLock = true) const;
    void drawTripletConnection() const;
    void drawSource(source_index_t index, ViewportSourceData const & source) const;
    void drawSpeaker(output_patch_t outputPatch, ViewportSpeakerConfig const & speaker);
    //==============================================================================
    static void drawBackground();
    static void drawTextOnGrid(std::string const & val, glm::vec3 position, float scale = 0.003f);
    static void drawVbapSpan(ViewportSourceData const & source);
    static void drawLbapSpan(ViewportSourceData const & source);
    static void drawFieldSphere();
    static void drawFieldCube();
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerViewComponent)
}; // class SpeakerViewComponent
