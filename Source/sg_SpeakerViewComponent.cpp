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

#include "sg_SpeakerViewComponent.hpp"

#include "sg_GlSphere.hpp"
#include "sg_MainComponent.hpp"

#include <algorithm>

juce::Colour const SpeakerViewComponent::COLOR_SPEAKER{ 222u, 222u, 222u };
juce::Colour const SpeakerViewComponent::COLOR_DIRECT_OUT_SPEAKER{ 64u, 64u, 64u };
juce::Colour const SpeakerViewComponent::COLOR_SPEAKER_SELECT{ 255u, 163u, 23u };

//==============================================================================
void SpeakerViewComponent::initialise()
{
    // TODO : continuous repainting should be set to false
    // openGLContext.setContinuousRepainting(true);

    juce::gl::glClearColor(0.0, 0.0, 0.0, 1.0);
    juce::gl::glColor3f(1.0, 1.0, 1.0);

    juce::gl::glMatrixMode(juce::gl::GL_PROJECTION);
    juce::gl::glLoadIdentity();
    gluPerspective(80.0, static_cast<GLdouble>(getWidth()) / static_cast<GLdouble>(getHeight()), 0.5, 75.0);

    juce::gl::glMatrixMode(juce::gl::GL_MODELVIEW);
    juce::gl::glLoadIdentity();
    gluLookAt(4.0, 6.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    int noArgs{};
    glutInit(&noArgs, nullptr);
}

//==============================================================================
static bool isOpenGlThread()
{
    auto const * currentThread{ juce::Thread::getCurrentThread() };
    if (!currentThread) {
        return false;
    }

    return currentThread->getThreadName() == "Pool";
}

//==============================================================================
bool isOpenGlOrMessageThread()
{
    return juce::MessageManager::existsAndIsCurrentThread() || isOpenGlThread();
}

//==============================================================================
Position SpeakerViewComponent::getCameraPosition() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mLock };

    return mData.coldData.cameraPosition;
}

//==============================================================================
void SpeakerViewComponent::setConfig(WarmViewportConfig const & config, SourcesData const & sources)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mLock };

    mData.warmData = config;
    mData.hotSourcesDataUpdaters.clear();
    for (auto const & source : sources) {
        mData.hotSourcesDataUpdaters.add(source.key);
    }
    repaint();
}

//==============================================================================
void SpeakerViewComponent::setCameraPosition(CartesianVector const & position) noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mLock };

    mData.coldData.cameraPosition = PolarVector{ position };
}

//==============================================================================
void SpeakerViewComponent::setTriplets(juce::Array<Triplet> triplets) noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mLock };
    mData.coldData.triplets = std::move(triplets);
}

//==============================================================================
void SpeakerViewComponent::render()
{
    static constexpr auto MIN_ZOOM = 0.7f;
    static constexpr auto MAX_ZOOM = 7.0f;
    static constexpr auto ZOOM_RANGE = MAX_ZOOM - MIN_ZOOM;
    static constexpr auto ZOOM_CURVE = 0.7f;
    static constexpr auto INVERSE_ZOOM_CURVE = 1.0f / ZOOM_CURVE;

    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    if (!juce::OpenGLHelpers::isContextActive()) {
        jassertfalse;
        return;
    }

    juce::ScopedLock const lock{ mLock };

    // Process zoom smoothed animation
    auto const currentTime{ juce::Time::currentTimeMillis() };
    auto const deciSecondsElapsed{
        std::clamp(narrow<float>(currentTime - mData.coldData.lastRenderTimeMs) / 100.0f, 0.0f, 1.0f)
    };
    auto const zoomToAdd{ deciSecondsElapsed * mData.coldData.cameraZoomVelocity };
    auto const currentZoom{ (mData.coldData.cameraPosition.getPolar().length - MIN_ZOOM) / ZOOM_RANGE };
    auto const scaledZoom{ std::pow(currentZoom, ZOOM_CURVE) };
    auto const scaledTargetZoom{ std::max(scaledZoom + zoomToAdd, 0.0f) };
    auto const unclippedTargetZoom{ std::pow(scaledTargetZoom, INVERSE_ZOOM_CURVE) * ZOOM_RANGE + MIN_ZOOM };
    auto const targetZoom{ std::clamp(unclippedTargetZoom, MIN_ZOOM, MAX_ZOOM) };

    jassert(std::isfinite(targetZoom));

    mData.coldData.cameraPosition = mData.coldData.cameraPosition.withRadius(targetZoom);
    mData.coldData.cameraZoomVelocity *= std::pow(0.5f, deciSecondsElapsed);
    mData.coldData.lastRenderTimeMs = currentTime;

    // Init OpenGL context
    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT | juce::gl::GL_DEPTH_BUFFER_BIT);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);

    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);

    juce::gl::glEnable(juce::gl::GL_LINE_SMOOTH);
    juce::gl::glHint(juce::gl::GL_LINE_SMOOTH, juce::gl::GL_NICEST);

    juce::gl::glEnable(juce::gl::GL_POINT_SMOOTH);
    juce::gl::glHint(juce::gl::GL_POINT_SMOOTH, juce::gl::GL_NICEST);

#ifndef WIN32
    juce::gl::glEnable(juce::gl::GL_MULTISAMPLE_ARB);
    juce::gl::glHint(juce::gl::GL_MULTISAMPLE_FILTER_HINT_NV, juce::gl::GL_NICEST);
#endif

    drawBackground();

    gluPerspective(70.0, static_cast<GLdouble>(getWidth()) / static_cast<GLdouble>(getHeight()), 0.5, 75.0);
    juce::gl::glMatrixMode(juce::gl::GL_MODELVIEW);

    auto const & camPos{ mData.coldData.cameraPosition.getCartesian() };

    juce::gl::glLoadIdentity();
    gluLookAt(camPos.x, camPos.y, camPos.z, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    drawOriginGrid();

    // Draw sources
    auto const & viewSettings{ mData.warmData.viewSettings };
    for (auto & source : mData.hotSourcesDataUpdaters) {
        auto & exchanger{ source.value };
        auto *& ticket{ mData.coldData.mostRecentSourcesData[source.key] };
        exchanger.getMostRecent(ticket);
        if (ticket == nullptr) {
            continue;
        }
        auto const & sourceData{ ticket->get() };
        if (!sourceData) {
            continue;
        }
        drawSource(source.key, *sourceData);
    }

    // Draw speakers
    if (viewSettings.showSpeakers) {
        for (auto const & speaker : mData.warmData.speakers) {
            drawSpeaker(speaker.key, speaker.value);
        }
    }

    // Draw triplets
    if (viewSettings.showSpeakerTriplets) {
        drawTripletConnection();
    }

    // Draw Sphere / Cube
    if (viewSettings.showSphereOrCube) {
        switch (mData.warmData.spatMode) {
        case SpatMode::vbap:
            drawFieldSphere();
            break;
        case SpatMode::lbap:
            drawFieldCube();
            break;
        case SpatMode::hybrid:
            drawFieldSphere();
            drawFieldCube();
            break;
        }
    }

    // test speaker selection
    if (mData.coldData.shouldRayCast) {
        clickRay();
        mData.coldData.shouldRayCast = false;
    }

    juce::gl::glFlush();
}

//==============================================================================
void SpeakerViewComponent::paint(juce::Graphics & g)
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mLock };

    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText(mData.warmData.title, 18, 18, 300, 30, juce::Justification::left);
}

//==============================================================================
void SpeakerViewComponent::clickRay()
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    double matModelView[16], matProjection[16];
    int viewport[4];

    juce::gl::glGetDoublev(juce::gl::GL_MODELVIEW_MATRIX, matModelView);
    juce::gl::glGetDoublev(juce::gl::GL_PROJECTION_MATRIX, matProjection);
    juce::gl::glGetIntegerv(juce::gl::GL_VIEWPORT, viewport);

    auto const winX{ mData.coldData.rayClick.x * mDisplayScaling };
    auto const winY{ viewport[3] - mData.coldData.rayClick.y * mDisplayScaling };

    gluUnProject(winX, winY, 0.0, matModelView, matProjection, viewport, &mXs, &mYs, &mZs);
    gluUnProject(winX, winY, 1.0, matModelView, matProjection, viewport, &mXe, &mYe, &mZe);

    mRay.setRay(glm::vec3{ mXs, mYs, mZs }, glm::vec3{ mXe, mYe, mZe });

    tl::optional<output_patch_t> iBestSpeaker{};
    auto const & speakers{ mData.warmData.speakers };
    for (auto const speaker : speakers) {
        if (rayCast(speaker.value.position.getCartesian()) != -1.0f) {
            if (!iBestSpeaker) {
                iBestSpeaker = speaker.key;
            } else {
                if (speakerNearCam(speaker.value.position.getCartesian(),
                                   speakers[*iBestSpeaker].position.getCartesian())) {
                    iBestSpeaker = speaker.key;
                }
            }
        }
    }

    if (iBestSpeaker) {
        juce::MessageManager::callAsync([this, speaker = *iBestSpeaker] {
            mMainContentComponent.setSelectedSpeakers(juce::Array<output_patch_t>{ speaker });
        });
    }
}

//==============================================================================
void SpeakerViewComponent::mouseDown(const juce::MouseEvent & e)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::ScopedLock const lock{ mLock };
    mData.coldData.panMouseOrigin = e.getPosition().toFloat();
    mData.coldData.panCameraOrigin = mData.coldData.cameraPosition.getPolar();

    // Always check on which display the speaker view component is.
    mDisplayScaling = juce::Desktop::getInstance().getDisplays().getDisplayForPoint(e.getScreenPosition())->scale;

    if (e.mods.isLeftButtonDown()) {
        mData.coldData.rayClick.x = narrow<float>(e.getPosition().x);
        mData.coldData.rayClick.y = narrow<float>(e.getPosition().y);
        mData.coldData.shouldRayCast = true;
    } else if (e.mods.isRightButtonDown()) {
        mMainContentComponent.setSelectedSpeakers(juce::Array<output_patch_t>{});
    }
}

//==============================================================================
void SpeakerViewComponent::mouseDrag(const juce::MouseEvent & e)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static constexpr auto CAM_SLOW_DOWN = 250.0f;
    static constexpr radians_t NEARLY_90_DEG{ degrees_t{ 89.99f } };

    juce::ScopedLock const lock{ mLock };
    if (e.mods.isLeftButtonDown()) {
        auto const delta{ (e.getPosition().toFloat() - mData.coldData.panMouseOrigin) };

        auto const azimuth{ mData.coldData.panCameraOrigin.azimuth - radians_t{ delta.x / CAM_SLOW_DOWN } };
        auto const elevation{ std::clamp(mData.coldData.panCameraOrigin.elevation
                                             + radians_t{ delta.y / CAM_SLOW_DOWN },
                                         -NEARLY_90_DEG,
                                         NEARLY_90_DEG) };

        mData.coldData.cameraPosition
            = mData.coldData.cameraPosition.getPolar().withBalancedAzimuth(azimuth).withClippedElevation(elevation);
    }
}

//==============================================================================
void SpeakerViewComponent::mouseWheelMove(const juce::MouseEvent & /*e*/, const juce::MouseWheelDetails & wheel)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mLock };

    // mData.state.cameraZoomVelocity -= wheel.deltaY * 0.05f;
    mData.coldData.cameraZoomVelocity -= wheel.deltaY * (mData.coldData.cameraZoomVelocity + 1.0f) * 0.1f;
}

//==============================================================================
void SpeakerViewComponent::drawBackground()
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    juce::gl::glMatrixMode(juce::gl::GL_PROJECTION);
    juce::gl::glLoadIdentity();
    juce::gl::glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    juce::gl::glDisable(juce::gl::GL_DEPTH_TEST);

    juce::gl::glMatrixMode(juce::gl::GL_MODELVIEW);
    juce::gl::glLoadIdentity();

    // Draw 2D image.
    juce::gl::glBegin(juce::gl::GL_QUADS);
    static auto const BRIGHT_COLOR{ juce::Colours::blue.withBrightness(0.6f).withSaturation(0.1f) };
    juce::gl::glColor3f(BRIGHT_COLOR.getFloatRed(), BRIGHT_COLOR.getFloatGreen(), BRIGHT_COLOR.getFloatBlue());
    // juce::gl::glColor3f(0.6f, 0.6f, 0.6f);
    // juce::gl::glColor3f(1.0f, 0.0f, 0.0f);
    juce::gl::glVertex2f(1.0f, 1.0f);
    juce::gl::glVertex2f(-1.0f, 1.0f);

    static auto const DARK_COLOR{ BRIGHT_COLOR.withBrightness(0.3f) };
    juce::gl::glColor3f(DARK_COLOR.getFloatRed(), DARK_COLOR.getFloatGreen(), DARK_COLOR.getFloatBlue());
    juce::gl::glVertex2f(0.0f, 0.0f);
    juce::gl::glVertex2f(1.0f, 0.0f);
    juce::gl::glEnd();

    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);
    juce::gl::glMatrixMode(juce::gl::GL_PROJECTION);
    juce::gl::glLoadIdentity();
}

//==============================================================================
void SpeakerViewComponent::drawOriginGrid() const
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    static auto const DRAW_SQUARE = [](float const length) -> void {
        juce::gl::glBegin(juce::gl::GL_LINES);
        juce::gl::glVertex3f(-length, length, 0.0f);
        juce::gl::glVertex3f(length, length, 0.0f);
        juce::gl::glVertex3f(length, -length, 0.0f);
        juce::gl::glVertex3f(length, length, 0.0f);
        juce::gl::glVertex3f(length, -length, 0.0f);
        juce::gl::glVertex3f(-length, -length, 0.0f);
        juce::gl::glVertex3f(-length, -length, 0.0f);
        juce::gl::glVertex3f(-length, length, 0.0f);
        juce::gl::glEnd();
    };

    static auto const DRAW_CIRCLE = [](float const radius) -> void {
        juce::gl::glBegin(juce::gl::GL_LINE_LOOP);
        for (int i{}; i <= 180; ++i) {
            auto const angle{ juce::MathConstants<float>::twoPi * narrow<float>(i) / 180.0f };
            juce::gl::glVertex3f(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f);
        }
        juce::gl::glEnd();
    };

    juce::gl::glLineWidth(1.5f);

    auto const drawDomeGrid = [&]() {
        // light grey
        juce::gl::glColor3f(0.59f, 0.59f, 0.59f);
        DRAW_CIRCLE(MAX_RADIUS / 2.0f);
        DRAW_CIRCLE(MAX_RADIUS);

        // dark grey
        juce::gl::glColor3f(0.49f, 0.49f, 0.49f);
        DRAW_CIRCLE(MAX_RADIUS / 4.0f);
        DRAW_CIRCLE(MAX_RADIUS / 4.0f * 3.0f);
        DRAW_CIRCLE(MAX_RADIUS / 4.0f * 5.0f);
    };

    auto const drawCubeGrid = [&]() {
        // light grey
        juce::gl::glColor3f(0.59f, 0.59f, 0.59f);
        DRAW_SQUARE(MAX_RADIUS);
        DRAW_SQUARE(SPACE_LIMIT);

        // dark grey
        juce::gl::glColor3f(0.49f, 0.49f, 0.49f);
        DRAW_CIRCLE(MAX_RADIUS);
        DRAW_CIRCLE(SPACE_LIMIT);
        DRAW_SQUARE(MAX_RADIUS / 2.0f);
    };

    // Squares & circles
    auto const spatMode{ mMainContentComponent.getData().speakerSetup.spatMode };
    switch (spatMode) {
    case SpatMode::vbap:
        drawDomeGrid();
        break;
    case SpatMode::lbap:
        drawCubeGrid();
        break;
    case SpatMode::hybrid:
        drawDomeGrid();
        drawCubeGrid();
        break;
    }

    // 3D RGB line.
    juce::gl::glLineWidth(2.0f);
    juce::gl::glBegin(juce::gl::GL_LINES);
    // X
    juce::gl::glColor3f(0.0f, 0.4f, 0.0f);
    juce::gl::glVertex3f(0.0f, 0.0f, 0.0f);
    juce::gl::glVertex3f(-MAX_RADIUS, 0.0f, 0.0f);
    juce::gl::glColor3f(0.0f, 1.0f, 0.0f);
    juce::gl::glVertex3f(MAX_RADIUS, 0.0f, 0.0f);
    juce::gl::glVertex3f(0.0f, 0.0f, 0.0f);
    // Y
    juce::gl::glColor3f(0.4f, 0.0f, 0.0f);
    juce::gl::glVertex3f(0.0f, -MAX_RADIUS, 0.0f);
    juce::gl::glVertex3f(0.0f, 0.0f, 0.0f);
    juce::gl::glColor3f(1.0f, 0.0f, 0.0f);
    juce::gl::glVertex3f(0.0f, 0.0f, 0.0f);
    juce::gl::glVertex3f(0.0f, MAX_RADIUS, 0.0f);
    // Z
    juce::gl::glColor3f(0.0f, 0.0f, 1.0f);
    juce::gl::glVertex3f(0.0f, 0.0f, 0.0f);
    juce::gl::glVertex3f(0.0f, 0.0f, MAX_RADIUS);

    juce::gl::glEnd();

    // Grid.
    juce::gl::glLineWidth(1.0f);
    juce::gl::glColor3f(0.49f, 0.49f, 0.49f);

    // Draw aligned cross
    auto const drawAlignedCross = [&](float const alignedCrossLength) {
        juce::gl::glBegin(juce::gl::GL_LINE_LOOP);
        juce::gl::glVertex3f(0.0f, -alignedCrossLength, 0.0f);
        juce::gl::glVertex3f(0.0f, alignedCrossLength, 0.0f);
        juce::gl::glEnd();
        juce::gl::glBegin(juce::gl::GL_LINE_LOOP);
        juce::gl::glVertex3f(-alignedCrossLength, 0.0f, 0.0f);
        juce::gl::glVertex3f(alignedCrossLength, 0.0f, 0.0f);
        juce::gl::glEnd();
    };

    auto const drawDiagonalCross = [&](float const diagonalCrossLength) {
        juce::gl::glBegin(juce::gl::GL_LINE_LOOP);
        juce::gl::glVertex3f(std::cos(QUARTER_PI.get()) * diagonalCrossLength,
                             std::sin(QUARTER_PI.get()) * diagonalCrossLength,
                             0.0f);
        juce::gl::glVertex3f(-std::cos(QUARTER_PI.get()) * diagonalCrossLength,
                             -std::sin(QUARTER_PI.get()) * diagonalCrossLength,
                             0.0f);
        juce::gl::glEnd();
        juce::gl::glBegin(juce::gl::GL_LINE_LOOP);
        juce::gl::glVertex3f(std::cos(QUARTER_PI.get() * 3.0f) * diagonalCrossLength,
                             std::sin(QUARTER_PI.get() * 3.0f) * diagonalCrossLength,
                             0.0f);
        juce::gl::glVertex3f(-std::cos(QUARTER_PI.get() * 3.0f) * diagonalCrossLength,
                             -std::sin(QUARTER_PI.get() * 3.0f) * diagonalCrossLength,
                             0.0f);
        juce::gl::glEnd();
    };

    auto const drawDomeCrosses = [&]() {
        static auto constexpr CROSS_LENGTH{ MAX_RADIUS * 1.5f };
        drawAlignedCross(CROSS_LENGTH);
        drawDiagonalCross(CROSS_LENGTH);
    };

    auto const drawCubeCrosses = [&]() {
        drawAlignedCross(SPACE_LIMIT);
        drawDiagonalCross(SPACE_LIMIT * juce::MathConstants<float>::sqrt2);
    };

    switch (spatMode) {
    case SpatMode::vbap:
        drawDomeCrosses();
        break;
    case SpatMode::lbap:
        drawCubeCrosses();
        break;
    case SpatMode::hybrid:
        drawDomeCrosses();
        drawCubeCrosses();
        break;
    }

    static auto constexpr HALF_CHAR_WIDTH = 0.025f;
    drawText("X", CartesianVector{ MAX_RADIUS, -HALF_CHAR_WIDTH, 0.0f }, juce::Colours::white, 0.0005f);
    drawText("Y", CartesianVector{ -HALF_CHAR_WIDTH, MAX_RADIUS, 0.0f }, juce::Colours::white, 0.0005f);
    drawText("Z", CartesianVector{ -HALF_CHAR_WIDTH, 0.0f, MAX_RADIUS + 0.03f }, juce::Colours::white, 0.0005f);

    drawTextOnGrid("0", glm::vec3(0.02f, 0.94f, 0.0f), 0.00035f);
    drawTextOnGrid("90", glm::vec3(0.91f, -0.08f, 0.0f), 0.00035f);
    drawTextOnGrid("180", glm::vec3(0.03f, -0.94f, 0.0f), 0.00035f);
    drawTextOnGrid("270", glm::vec3(-0.98f, -0.08f, 0.0f), 0.00035f);
}

//==============================================================================
void SpeakerViewComponent::drawText(juce::String const & val,
                                    CartesianVector const & position,
                                    juce::Colour const color,
                                    float const scale,
                                    bool const camLock) const
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    juce::gl::glPushMatrix();
    juce::gl::glTranslatef(position.x, position.y, position.z);

    if (camLock) {
        auto const & camPos{ mData.coldData.cameraPosition.getPolar() };
        auto const azimuth{ degrees_t{ camPos.azimuth + HALF_PI } };

        juce::gl::glRotatef(degrees_t{ 90.0f }.get(), 1.0f, 0.0f, 0.0f);
        juce::gl::glRotatef(azimuth.get(), 0.0f, 1.0f, 0.0f);
    }

    juce::gl::glScalef(scale, scale, scale);
    juce::gl::glColor4f(color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), color.getAlpha());
    for (auto const c : val) {
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, c);
    }
    juce::gl::glTranslatef(-position.x, -position.y, -position.z);
    juce::gl::glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawTextOnGrid(std::string const & val, glm::vec3 const position, float const scale)
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    juce::gl::glPushMatrix();
    juce::gl::glTranslatef(position.x, position.y, position.z);
    juce::gl::glRotatef(0.0f, 0.0f, 0.0f, 1.0f);

    juce::gl::glScalef(scale, scale, scale);
    juce::gl::glColor3f(1.0f, 1.0f, 1.0f);
    for (auto const c : val) {
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, c);
    }
    juce::gl::glTranslatef(-position.x, -position.y, -position.z);
    juce::gl::glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawTripletConnection() const
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    for (auto const & triplet : mData.coldData.triplets) {
        auto const & spk1{ mData.warmData.speakers[triplet.id1].position.getCartesian() };
        auto const & spk2{ mData.warmData.speakers[triplet.id2].position.getCartesian() };
        auto const & spk3{ mData.warmData.speakers[triplet.id3].position.getCartesian() };

        juce::gl::glLineWidth(1.0f);
        juce::gl::glBegin(juce::gl::GL_LINES);
        juce::gl::glColor3f(0.8f, 0.8f, 0.8f);
        juce::gl::glVertex3f(spk1.x, spk1.y, spk1.z);
        juce::gl::glVertex3f(spk2.x, spk2.y, spk2.z);
        juce::gl::glVertex3f(spk1.x, spk1.y, spk1.z);
        juce::gl::glVertex3f(spk3.x, spk3.y, spk3.z);
        juce::gl::glVertex3f(spk2.x, spk2.y, spk2.z);
        juce::gl::glVertex3f(spk3.x, spk3.y, spk3.z);
        juce::gl::glEnd();
    }
}

//==============================================================================
void SpeakerViewComponent::drawSource(source_index_t const index, ViewportSourceData const & source) const
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    if (source.colour.getFloatAlpha() == 0.0f) {
        return;
    }

    // Draw 3D sphere.
    juce::gl::glPushMatrix();
    auto const & pos{ source.position.getCartesian() };
    juce::gl::glTranslatef(pos.x, pos.y, pos.z);
    juce::gl::glPolygonMode(juce::gl::GL_FRONT_AND_BACK, juce::gl::GL_FILL);
    juce::gl::glLineWidth(2.0f);

    juce::gl::glColor4f(source.colour.getFloatRed(),
                        source.colour.getFloatGreen(),
                        source.colour.getFloatBlue(),
                        source.colour.getFloatAlpha());

    drawSphere<7>(SPHERE_RADIUS);

    juce::gl::glTranslatef(-pos.x, -pos.y, -pos.z);

    auto const getEffectiveSpatMode = [&]() {
        auto const & baseSpatMode{ mData.warmData.spatMode };
        switch (baseSpatMode) {
        case SpatMode::vbap:
        case SpatMode::lbap:
            return baseSpatMode;
        case SpatMode::hybrid:
            return source.hybridSpatMode;
        }
        jassertfalse;
        return SpatMode::vbap;
    };

    if (source.azimuthSpan != 0.0f || source.zenithSpan != 0.0f) {
        switch (getEffectiveSpatMode()) {
        case SpatMode::lbap:
            drawLbapSpan(source);
            break;
        case SpatMode::vbap:
            drawVbapSpan(source);
            break;
        default:
            jassertfalse;
        }
    }

    juce::gl::glPolygonMode(juce::gl::GL_FRONT_AND_BACK, juce::gl::GL_FILL);
    juce::gl::glPopMatrix();

    if (mData.warmData.viewSettings.showSourceNumbers) {
        auto position{ source.position.getCartesian() };
        position.y += SIZE_SPEAKER + 0.04f;
        drawText(juce::String{ index.get() }, position, source.colour, 0.0003f, true);
    }
}

//==============================================================================
void SpeakerViewComponent::drawVbapSpan(ViewportSourceData const & source)
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    static auto constexpr NUM{ 8 };

    juce::gl::glPointSize(4);
    juce::gl::glBegin(juce::gl::GL_POINTS);

    auto const position{ source.position };
    auto const & azimuth{ position.getPolar().azimuth };
    auto const & elevation{ position.getPolar().elevation };
    auto const & length{ position.getPolar().length };

    for (int i{}; i < NUM; ++i) {
        radians_t const aziDev{ source.azimuthSpan * narrow<float>(i) * 0.42f };
        for (int j{}; j < 2; ++j) {
            auto const newAzimuth{ j ? azimuth + aziDev : azimuth - aziDev };

            {
                CartesianVector const vertex{ source.position.getPolar().withBalancedAzimuth(newAzimuth) };
                juce::gl::glVertex3f(vertex.x, vertex.y, vertex.z);
            }
            for (int k{}; k < 4; ++k) {
                radians_t const eleDev{ (static_cast<float>(k) + 1.0f) * source.zenithSpan * 0.38f };
                for (int l{}; l < 2; ++l) {
                    auto newElevation{ l ? elevation + eleDev : elevation - eleDev };
                    newElevation = std::clamp(newElevation, radians_t{}, HALF_PI);
                    CartesianVector const vertex{ PolarVector{ newAzimuth, newElevation, length * MAX_RADIUS } };
                    juce::gl::glVertex3f(vertex.x, vertex.y, vertex.z);
                }
            }
        }
    }

    juce::gl::glEnd();
    juce::gl::glPointSize(1);
}

//==============================================================================
void SpeakerViewComponent::drawFieldSphere()
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    juce::gl::glPushMatrix();
    juce::gl::glPolygonMode(juce::gl::GL_FRONT_AND_BACK, juce::gl::GL_LINE);
    juce::gl::glLineWidth(1.0f);
    juce::gl::glRotatef(degrees_t{ 90.0f }.get(), 1.0f, 0.0f, 0.0f);
    juce::gl::glColor3f(0.8f, 0.2f, 0.1f);

    drawSphere<16>(std::max(MAX_RADIUS, 1.0f));

    juce::gl::glPolygonMode(juce::gl::GL_FRONT_AND_BACK, juce::gl::GL_FILL);
    juce::gl::glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawFieldCube()
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    juce::gl::glPushMatrix();
    juce::gl::glPolygonMode(juce::gl::GL_FRONT_AND_BACK, juce::gl::GL_LINE);
    juce::gl::glLineWidth(1.0f);
    juce::gl::glRotatef(degrees_t{ 90.0f }.get(), 1.0f, 0.0f, 0.0f);
    juce::gl::glColor3f(0.8f, 0.2f, 0.1f);
    // Draw a cube when in LBAP mode.
    static constexpr auto DEFINITION = 10;
    for (auto i{ -DEFINITION }; i <= DEFINITION; i += 2) {
        auto const i_f{ narrow<float>(i) / narrow<float>(DEFINITION) };
        juce::gl::glBegin(juce::gl::GL_LINES);
        juce::gl::glVertex3f(i_f, MAX_RADIUS, -MAX_RADIUS);
        juce::gl::glVertex3f(i_f, MAX_RADIUS, MAX_RADIUS);
        juce::gl::glVertex3f(i_f, -MAX_RADIUS, -MAX_RADIUS);
        juce::gl::glVertex3f(i_f, -MAX_RADIUS, MAX_RADIUS);
        juce::gl::glVertex3f(MAX_RADIUS, -MAX_RADIUS, i_f);
        juce::gl::glVertex3f(MAX_RADIUS, MAX_RADIUS, i_f);
        juce::gl::glVertex3f(-MAX_RADIUS, -MAX_RADIUS, i_f);
        juce::gl::glVertex3f(-MAX_RADIUS, MAX_RADIUS, i_f);
        juce::gl::glVertex3f(MAX_RADIUS, i_f, -MAX_RADIUS);
        juce::gl::glVertex3f(MAX_RADIUS, i_f, MAX_RADIUS);
        juce::gl::glVertex3f(-MAX_RADIUS, i_f, -MAX_RADIUS);
        juce::gl::glVertex3f(-MAX_RADIUS, i_f, MAX_RADIUS);
        juce::gl::glVertex3f(-MAX_RADIUS, i_f, MAX_RADIUS);
        juce::gl::glVertex3f(MAX_RADIUS, i_f, MAX_RADIUS);
        juce::gl::glVertex3f(-MAX_RADIUS, i_f, -MAX_RADIUS);
        juce::gl::glVertex3f(MAX_RADIUS, i_f, -MAX_RADIUS);
        juce::gl::glVertex3f(-MAX_RADIUS, MAX_RADIUS, i_f);
        juce::gl::glVertex3f(MAX_RADIUS, MAX_RADIUS, i_f);
        juce::gl::glVertex3f(-MAX_RADIUS, -MAX_RADIUS, i_f);
        juce::gl::glVertex3f(MAX_RADIUS, -MAX_RADIUS, i_f);
        juce::gl::glVertex3f(i_f, -MAX_RADIUS, MAX_RADIUS);
        juce::gl::glVertex3f(i_f, MAX_RADIUS, MAX_RADIUS);
        juce::gl::glVertex3f(i_f, -MAX_RADIUS, -MAX_RADIUS);
        juce::gl::glVertex3f(i_f, MAX_RADIUS, -MAX_RADIUS);
        juce::gl::glEnd();
    }
    juce::gl::glPolygonMode(juce::gl::GL_FRONT_AND_BACK, juce::gl::GL_FILL);
    juce::gl::glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawLbapSpan(ViewportSourceData const & source)
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    static auto constexpr NUM = 4;

    juce::gl::glPointSize(4);
    juce::gl::glBegin(juce::gl::GL_POINTS);

    auto const & pos{ source.position.getCartesian() };

    // For the same elevation as the source position.
    for (auto i{ 1 }; i <= NUM; ++i) {
        auto const raddev{ narrow<float>(i) * source.azimuthSpan / narrow<float>(NUM - 1) };
        if (i < NUM) {
            juce::gl::glVertex3f(pos.x + raddev, pos.y + raddev, pos.z);
            juce::gl::glVertex3f(pos.x + raddev, pos.y - raddev, pos.z);
            juce::gl::glVertex3f(pos.x - raddev, pos.y + raddev, pos.z);
            juce::gl::glVertex3f(pos.x - raddev, pos.y - raddev, pos.z);
        }
        juce::gl::glVertex3f(pos.x + raddev, pos.y, pos.z);
        juce::gl::glVertex3f(pos.x - raddev, pos.y, pos.z);
        juce::gl::glVertex3f(pos.x, pos.y + raddev, pos.z);
        juce::gl::glVertex3f(pos.x, pos.y - raddev, pos.z);
    }

    // For all other elevation levels.
    for (int j{}; j < NUM; ++j) {
        auto const eledev{ narrow<float>(j + 1) / narrow<float>(NUM) * source.zenithSpan };
        for (int k{}; k < 2; ++k) {
            auto const zTemp{ std::clamp(k != 0 ? pos.z + eledev : pos.z - eledev, 0.0f, 1.0f) };
            for (auto i{ 1 }; i <= NUM; ++i) {
                auto const raddev{ narrow<float>(i) * source.azimuthSpan / narrow<float>(NUM - 1) };
                if (i < NUM) {
                    juce::gl::glVertex3f(pos.x + raddev, pos.y + raddev, zTemp);
                    juce::gl::glVertex3f(pos.x + raddev, pos.y - raddev, zTemp);
                    juce::gl::glVertex3f(pos.x - raddev, pos.y + raddev, zTemp);
                    juce::gl::glVertex3f(pos.x - raddev, pos.y - raddev, zTemp);
                }
                juce::gl::glVertex3f(pos.x + raddev, pos.y, zTemp);
                juce::gl::glVertex3f(pos.x - raddev, pos.y, zTemp);
                juce::gl::glVertex3f(pos.x, pos.y + raddev, zTemp);
                juce::gl::glVertex3f(pos.x, pos.y - raddev, zTemp);
            }
        }
    }

    juce::gl::glEnd();
    juce::gl::glPointSize(1);
}

//==============================================================================
void SpeakerViewComponent::drawSpeaker(output_patch_t const outputPatch, ViewportSpeakerConfig const & speaker)
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    static constexpr auto DEFAULT_ALPHA = 0.75f;

    auto const & showSpeakerLevels{ mData.warmData.viewSettings.showSpeakerLevels };
    auto const getAlpha = [&]() {
        if (!showSpeakerLevels) {
            return DEFAULT_ALPHA;
        }
        auto & exchanger{ mData.hotSpeakersAlphaUpdaters[outputPatch] };
        auto *& ticket{ mData.coldData.mostRecentSpeakersAlpha[outputPatch] };
        exchanger.getMostRecent(ticket);
        if (ticket == nullptr) {
            return DEFAULT_ALPHA;
        }
        return ticket->get();
    };

    auto const alpha{ getAlpha() };

    auto const getSpeakerColour = [&]() {
        if (speaker.isSelected) {
            return COLOR_SPEAKER_SELECT.withAlpha(alpha);
        }
        if (speaker.isDirectOutOnly && !showSpeakerLevels) {
            return COLOR_DIRECT_OUT_SPEAKER.withAlpha(alpha);
        }
        return COLOR_SPEAKER.withAlpha(alpha);
    };

    auto const & center{ speaker.position.getCartesian() };
    auto const & vector{ speaker.position.getPolar() };

    juce::gl::glPushMatrix();
    juce::gl::glTranslatef(center.x, center.y, center.z);
    juce::gl::glRotatef(degrees_t(PI + vector.azimuth).get(), 0.0f, 0.0f, 1.0f);
    juce::gl::glRotatef(degrees_t{ vector.elevation }.get(), 0.0f, 1.0f, 0.0f);
    juce::gl::glTranslatef(-center.x, -center.y, -center.z);

    CartesianVector const vertexMin{ center.x - SIZE_SPEAKER, center.y - SIZE_SPEAKER, center.z - SIZE_SPEAKER };
    CartesianVector const vertexMax{ center.x + SIZE_SPEAKER, center.y + SIZE_SPEAKER, center.z + SIZE_SPEAKER };
    auto const colour{ getSpeakerColour() };

    // Draw box
    juce::gl::glBegin(juce::gl::GL_QUADS);
    juce::gl::glColor4f(colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    juce::gl::glVertex3f(vertexMin.x, vertexMin.y, vertexMax.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMin.y, vertexMax.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMax.y, vertexMax.z);
    juce::gl::glVertex3f(vertexMin.x, vertexMax.y, vertexMax.z);

    juce::gl::glVertex3f(vertexMax.x, vertexMin.y, vertexMax.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMin.y, vertexMin.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMax.y, vertexMin.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMax.y, vertexMax.z);

    juce::gl::glVertex3f(vertexMin.x, vertexMax.y, vertexMax.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMax.y, vertexMax.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMax.y, vertexMin.z);
    juce::gl::glVertex3f(vertexMin.x, vertexMax.y, vertexMin.z);

    juce::gl::glVertex3f(vertexMin.x, vertexMin.y, vertexMin.z);
    juce::gl::glVertex3f(vertexMin.x, vertexMax.y, vertexMin.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMax.y, vertexMin.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMin.y, vertexMin.z);

    juce::gl::glVertex3f(vertexMin.x, vertexMin.y, vertexMin.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMin.y, vertexMin.z);
    juce::gl::glVertex3f(vertexMax.x, vertexMin.y, vertexMax.z);
    juce::gl::glVertex3f(vertexMin.x, vertexMin.y, vertexMax.z);

    juce::gl::glVertex3f(vertexMin.x, vertexMin.y, vertexMin.z);
    juce::gl::glVertex3f(vertexMin.x, vertexMin.y, vertexMax.z);
    juce::gl::glVertex3f(vertexMin.x, vertexMax.y, vertexMax.z);
    juce::gl::glVertex3f(vertexMin.x, vertexMax.y, vertexMin.z);

    juce::gl::glEnd();

    // Draw selection wireframe
    if (speaker.isSelected) {
        juce::gl::glPolygonMode(juce::gl::GL_FRONT_AND_BACK, juce::gl::GL_LINE);
        juce::gl::glLineWidth(3.0f);
        juce::gl::glColor3f(0.0f, 0.0f, 0.0f);
        juce::gl::glBegin(juce::gl::GL_LINES);

        static constexpr auto OVER = 0.001f;

        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMax.z + OVER);

        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMax.z + OVER);

        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMin.z - OVER);
        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMin.z - OVER);
        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMin.z - OVER);

        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMax.z + OVER);
        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMax.z + OVER);

        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMin.z - OVER);
        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMin.z - OVER);

        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMax.z + OVER);
        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMin.z - OVER);

        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMax.z + OVER);
        juce::gl::glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMin.z - OVER);

        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMax.z + OVER);
        juce::gl::glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        juce::gl::glEnd();
        juce::gl::glPolygonMode(juce::gl::GL_FRONT_AND_BACK, juce::gl::GL_FILL);
    }

    // draw direction line
    juce::gl::glLineWidth(2.0f);
    juce::gl::glBegin(juce::gl::GL_LINES);
    if (speaker.isSelected) {
        juce::gl::glColor4f(0.0f, 0.0f, 0.0f, alpha);
    } else {
        juce::gl::glColor4f(0.37f, 0.37f, 0.37f, alpha);
    }
    juce::gl::glVertex3f(center.x + SIZE_SPEAKER, center.y, center.z);
    juce::gl::glVertex3f(center.x + 0.12f, center.y, center.z);
    juce::gl::glEnd();

    juce::gl::glPopMatrix();

    if (mData.warmData.viewSettings.showSpeakerNumbers) {
        auto posT{ speaker.position.getCartesian() };
        posT.z += SIZE_SPEAKER + 0.04f;
        drawText(juce::String{ outputPatch.get() }, posT, juce::Colours::black, 0.0003f);
    }
}

//==============================================================================
float SpeakerViewComponent::rayCast(CartesianVector const & speakerPosition) const
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    auto const t1{ (speakerPosition.x - SIZE_SPEAKER - mRay.getPosition().x) / mRay.getNormal().x };
    auto const t2{ (speakerPosition.x + SIZE_SPEAKER - mRay.getPosition().x) / mRay.getNormal().x };
    auto const t3{ (speakerPosition.y - SIZE_SPEAKER - mRay.getPosition().y) / mRay.getNormal().y };
    auto const t4{ (speakerPosition.y + SIZE_SPEAKER - mRay.getPosition().y) / mRay.getNormal().y };
    auto const t5{ (speakerPosition.z - SIZE_SPEAKER - mRay.getPosition().z) / mRay.getNormal().z };
    auto const t6{ (speakerPosition.z + SIZE_SPEAKER - mRay.getPosition().z) / mRay.getNormal().z };

    auto const tMin{ std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6)) };
    auto const tMax{ std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6)) };

    // if tMax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
    if (tMax < 0) {
        return -1;
    }

    // if tMin > tMax, ray doesn't intersect AABB
    if (tMin > tMax) {
        return -1;
    }

    if (tMin < 0.0f) {
        return tMax;
    }
    return tMin;
}

//==============================================================================
bool SpeakerViewComponent::speakerNearCam(CartesianVector const & speak1, CartesianVector const & speak2) const
{
    ASSERT_IS_OPEN_GL_OR_MESSAGE_THREAD;

    auto const & camPosition{ mData.coldData.cameraPosition.getCartesian() };
    auto const distance1{ (speak1 - camPosition).length2() };
    auto const distance2{ (speak2 - camPosition).length2() };
    return distance1 < distance2;
}
