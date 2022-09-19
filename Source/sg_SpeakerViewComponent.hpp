/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_CartesianVector.hpp"
#include "sg_LogicStrucs.hpp"
#include "sg_Ray.hpp"
#include "sg_Warnings.hpp"
#include "sg_constants.hpp"

#include <JuceHeader.h>

DISABLE_WARNING_PUSH
DISABLE_WARNING_NAMELESS_STRUCT
DISABLE_WARNING_UNREFERENCED_FUNCTION
#if defined(__APPLE__)
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/gl3.h>
    #include <OpenGl/glu.h>
#endif
DISABLE_WARNING_POP

#define ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD jassert(isOpenGlOrMessageThread())

namespace gris
{
struct SpeakerData;
enum class SpatMode;
struct SourceData;

bool isOpenGlOrMessageThread();

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
    static auto constexpr SPACE_LIMIT{ MAX_RADIUS * MBAP_EXTENDED_RADIUS };

private:
    //==============================================================================
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
    //==============================================================================
    static constexpr auto SPHERE_RADIUS = 0.03f;
    static constexpr auto HALF_SPHERE_RADIUS = SPHERE_RADIUS / 2.0f;
    //==============================================================================
    explicit SpeakerViewComponent(MainContentComponent & mainContentComponent)
        : mMainContentComponent(mainContentComponent)
    {
    }
    ~SpeakerViewComponent() override { shutdownOpenGL(); }
    SG_DELETE_COPY_AND_MOVE(SpeakerViewComponent)
    //==============================================================================
    auto & getData() noexcept { return mData; }
    auto const & getData() const noexcept { return mData; }

    Position getCameraPosition() const noexcept;

    void setConfig(ViewportConfig const & config, SourcesData const & sources);
    void setCameraPosition(CartesianVector const & position) noexcept;
    void setTriplets(juce::Array<Triplet> triplets) noexcept;

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
    static void drawTextOnGrid(std::string const & val, CartesianVector const & position, float scale = 0.003f);
    static void drawVbapSpan(ViewportSourceData const & source);
    static void drawMbapSpan(ViewportSourceData const & source);
    static void drawFieldSphere();
    static void drawFieldCube();
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerViewComponent)
}; // class SpeakerViewComponent

} // namespace gris