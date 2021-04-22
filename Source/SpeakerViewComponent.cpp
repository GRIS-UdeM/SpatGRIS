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

#include "SpeakerViewComponent.h"

#include <algorithm>

#include "GlSphere.h"
#include "MainComponent.h"

juce::Colour const SpeakerViewComponent::COLOR_SPEAKER{ 222u, 222u, 222u };
juce::Colour const SpeakerViewComponent::COLOR_DIRECT_OUT_SPEAKER{ 64u, 64u, 64u };
juce::Colour const SpeakerViewComponent::COLOR_SPEAKER_SELECT{ 255u, 163u, 23u };

Pool<tl::optional<ViewportSourceData>> ThreadsafePtr<tl::optional<ViewportSourceData>>::pool{ 32 };
Pool<float> ThreadsafePtr<float>::pool{ 32 };

//==============================================================================
void SpeakerViewComponent::initialise()
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glColor3f(1.0, 1.0, 1.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(80.0, static_cast<GLdouble>(getWidth()) / static_cast<GLdouble>(getHeight()), 0.5, 75.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(4.0, 6.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    int noArgs{};
    glutInit(&noArgs, nullptr);
}

//==============================================================================
void SpeakerViewComponent::setConfig(ViewportConfig const & config)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mLock };
    mData.config = config;
    repaint();
}

//==============================================================================
void SpeakerViewComponent::setCameraPosition(CartesianVector const & position) noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mLock };
    mData.state.cameraPosition = PolarVector::fromCartesian(position);
}

//==============================================================================
void SpeakerViewComponent::render()
{
    ASSERT_OPEN_GL_THREAD;
    jassert(juce::OpenGLHelpers::isContextActive());

    juce::ScopedLock const lock{ mLock };

    static constexpr auto MIN_ZOOM = 0.3f;
    static constexpr auto MAX_ZOOM = 3.0f;
    static constexpr auto ZOOM_RANGE = MAX_ZOOM - MIN_ZOOM;
    static constexpr auto ZOOM_CURVE = 0.5f;
    static constexpr auto INVERSE_ZOOM_CURVE = 1.0f / ZOOM_CURVE;

    auto const currentTime{ glutGet(GLUT_ELAPSED_TIME) };
    auto const secondsElapsed{ narrow<float>(currentTime - mData.state.lastRenderTimeMs) / 100.0f };
    auto const zoomToAdd{ secondsElapsed * mData.state.cameraZoomVelocity };

    if (zoomToAdd) {
        int test{};
    }

    auto const currentZoom{ (mData.state.cameraPosition.length - MIN_ZOOM) / ZOOM_RANGE };
    auto const scaledZoom{ std::pow(currentZoom, ZOOM_CURVE) };
    auto const scaledTargetZoom{ scaledZoom + zoomToAdd };
    auto const unclippedTargetZoom{ std::pow(scaledTargetZoom, INVERSE_ZOOM_CURVE) * ZOOM_RANGE + MIN_ZOOM };
    auto const targetZoom{ std::clamp(unclippedTargetZoom, MIN_ZOOM, MAX_ZOOM) };

    mData.state.cameraPosition.length = targetZoom;
    mData.state.cameraZoomVelocity *= std::pow(0.5f, secondsElapsed);
    mData.state.lastRenderTimeMs = currentTime;

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH, GL_NICEST);

    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH, GL_NICEST);

#ifndef WIN32
    glEnable(GL_MULTISAMPLE_ARB);
    glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
#endif

    drawBackground();

    gluPerspective(80.0, static_cast<GLdouble>(getWidth()) / static_cast<GLdouble>(getHeight()), 0.5, 75.0);
    glMatrixMode(GL_MODELVIEW);

    auto const & camPos{ mData.state.cameraPosition.toCartesian() };

    glLoadIdentity();
    gluLookAt(camPos.x, camPos.y, camPos.z, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    drawOriginGrid();

    // Draw sources
    auto const & viewSettings{ mData.config.viewSettings };
    for (auto & sourceQueue : mData.sources) {
        auto const & source{ *sourceQueue.value.get() };
        if (!source) {
            continue;
        }
        drawSource(sourceQueue.key, *source);
    }

    // Draw speakers
    if (viewSettings.showSpeakers) {
        for (auto const & speaker : mData.config.speakers) {
            drawSpeaker(speaker.key, speaker.value);
        }
    }

    // Draw triplets
    if (viewSettings.showSpeakerTriplets) {
        drawTripletConnection();
    }

    // Draw Sphere / Cube
    if (viewSettings.showSphereOrCube) {
        switch (mData.config.spatMode) {
        case SpatMode::hrtfVbap:
        case SpatMode::vbap:
            drawFieldCube();
            break;
        case SpatMode::lbap:
            drawFieldSphere();
        case SpatMode::stereo:
            break;
        default:
            jassertfalse;
            break;
        }
    }

    // test speaker selection
    if (mData.state.shouldRayCast) {
        clickRay();
        mData.state.shouldRayCast = false;
    }

    glFlush();
}

//==============================================================================
void SpeakerViewComponent::paint(juce::Graphics & g)
{
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText(mData.config.title, 18, 18, 300, 30, juce::Justification::left);
}

//==============================================================================
void SpeakerViewComponent::clickRay()
{
    ASSERT_OPEN_GL_THREAD;

    double matModelView[16], matProjection[16];
    int viewport[4];

    glGetDoublev(GL_MODELVIEW_MATRIX, matModelView);
    glGetDoublev(GL_PROJECTION_MATRIX, matProjection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    auto const winX{ mData.state.rayClick.x * mDisplayScaling };
    auto const winY{ viewport[3] - mData.state.rayClick.y * mDisplayScaling };

    gluUnProject(winX, winY, 0.0, matModelView, matProjection, viewport, &mXs, &mYs, &mZs);
    gluUnProject(winX, winY, 1.0, matModelView, matProjection, viewport, &mXe, &mYe, &mZe);

    mRay.setRay(glm::vec3{ mXs, mYs, mZs }, glm::vec3{ mXe, mYe, mZe });

    tl::optional<output_patch_t> iBestSpeaker{};
    auto const & speakers{ mData.config.speakers };
    for (auto const speaker : speakers) {
        if (rayCast(speaker.value.position) != -1.0f) {
            if (!iBestSpeaker) {
                iBestSpeaker = speaker.key;
            } else {
                if (speakerNearCam(speaker.value.position, speakers[*iBestSpeaker].position)) {
                    iBestSpeaker = speaker.key;
                }
            }
        }
    }

    if (iBestSpeaker) {
        juce::MessageManager::callAsync([this, speaker = *iBestSpeaker] {
            mMainContentComponent.handleSpeakerSelected(juce::Array<output_patch_t>{ speaker });
        });
    }
}

//==============================================================================
void SpeakerViewComponent::mouseDown(const juce::MouseEvent & e)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::ScopedLock const lock{ mLock };
    mData.state.panMouseOrigin = e.getPosition().toFloat();
    mData.state.panCameraOrigin = mData.state.cameraPosition;

    // Always check on which display the speaker view component is.
    mDisplayScaling = juce::Desktop::getInstance().getDisplays().getDisplayForPoint(e.getScreenPosition())->scale;

    if (e.mods.isLeftButtonDown()) {
        mData.state.rayClick.x = narrow<float>(e.getPosition().x);
        mData.state.rayClick.y = narrow<float>(e.getPosition().y);
        mData.state.shouldRayCast = true;
    } else if (e.mods.isRightButtonDown()) {
        mMainContentComponent.handleSpeakerSelected(juce::Array<output_patch_t>{});
    }
}

//==============================================================================
void SpeakerViewComponent::mouseDrag(const juce::MouseEvent & e)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static constexpr float CAM_SLOW_DOWN = 250.0f;
    static constexpr radians_t NEARLY_90_DEG{ degrees_t{ 89.99f } };

    juce::ScopedLock const lock{ mLock };
    if (e.mods.isLeftButtonDown()) {
        auto const delta{ (e.getPosition().toFloat() - mData.state.panMouseOrigin) };

        mData.state.cameraPosition.azimuth
            = (mData.state.panCameraOrigin.azimuth - radians_t{ delta.x / CAM_SLOW_DOWN }).centered();
        mData.state.cameraPosition.elevation
            = std::clamp((mData.state.panCameraOrigin.elevation + radians_t{ delta.y / CAM_SLOW_DOWN }).centered(),
                         -NEARLY_90_DEG,
                         NEARLY_90_DEG);
    }
}

//==============================================================================
void SpeakerViewComponent::mouseWheelMove(const juce::MouseEvent & /*e*/, const juce::MouseWheelDetails & wheel)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mLock };

    // mData.state.cameraZoomVelocity -= wheel.deltaY * 0.05f;
    mData.state.cameraZoomVelocity -= wheel.deltaY * (mData.state.cameraZoomVelocity + 1.0f) * 0.1f;
}

//==============================================================================
void SpeakerViewComponent::drawBackground()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw 2D image.
    glBegin(GL_QUADS);
    glColor3f(0.6f, 0.6f, 0.6f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);

    glColor3f(0.3f, 0.3f, 0.3f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(1.0f, 0.0f);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

//==============================================================================
void SpeakerViewComponent::drawOriginGrid() const
{
    ASSERT_OPEN_GL_THREAD;

    static auto const drawSquare = [](float const length) -> void {
        glBegin(GL_LINES);
        glVertex3f(-length, length, 0.0f);
        glVertex3f(length, length, 0.0f);
        glVertex3f(length, -length, 0.0f);
        glVertex3f(length, length, 0.0f);
        glVertex3f(length, -length, 0.0f);
        glVertex3f(-length, -length, 0.0f);
        glVertex3f(-length, -length, 0.0f);
        glVertex3f(-length, length, 0.0f);
        glEnd();
    };

    static auto const drawCircle = [](float const radius) -> void {
        glBegin(GL_LINE_LOOP);
        for (int i{}; i <= 180; ++i) {
            auto const angle{ juce::MathConstants<float>::twoPi * narrow<float>(i) / 180.0f };
            glVertex3f(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f);
        }
        glEnd();
    };

    glLineWidth(1.5f);

    // Squares & circles
    auto const spatMode{ mMainContentComponent.getData().appData.spatMode };
    if (spatMode == SpatMode::lbap) {
        // light grey
        glColor3f(0.59f, 0.59f, 0.59f);
        drawSquare(MAX_RADIUS);
        drawSquare(SPACE_LIMIT);

        // dark grey
        glColor3f(0.49f, 0.49f, 0.49f);
        drawCircle(MAX_RADIUS);
        drawCircle(SPACE_LIMIT);
        drawSquare(MAX_RADIUS / 2.0f);
    } else {
        // light grey
        glColor3f(0.59f, 0.59f, 0.59f);
        drawCircle(MAX_RADIUS / 2.0f);
        drawCircle(MAX_RADIUS);

        // dark grey
        glColor3f(0.49f, 0.49f, 0.49f);
        drawCircle(MAX_RADIUS / 4.0f);
        drawCircle(MAX_RADIUS / 4.0f * 3.0f);
        drawCircle(MAX_RADIUS / 4.0f * 5.0f);
    }

    // 3D RGB line.
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    // X
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, -MAX_RADIUS, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.4f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, MAX_RADIUS, 0.0f);
    // Y
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex3f(-MAX_RADIUS, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(MAX_RADIUS, 0.0f, 0.0f);
    // Z
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, MAX_RADIUS);

    glEnd();

    // Grid.
    glLineWidth(1.0f);
    glColor3f(0.49f, 0.49f, 0.49f);

    // Draw aligned cross
    auto const alignedCrossLength{ spatMode == SpatMode::lbap ? SPACE_LIMIT : MAX_RADIUS * 1.5f };
    glBegin(GL_LINE_LOOP);
    glVertex3f(0.0f, -alignedCrossLength, 0.0f);
    glVertex3f(0.0f, alignedCrossLength, 0.0f);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(-alignedCrossLength, 0.0f, 0.0f);
    glVertex3f(alignedCrossLength, 0.0f, 0.0f);
    glEnd();

    // Draw diagonal cross
    auto const diagonalCrossLength{ spatMode == SpatMode::lbap ? SPACE_LIMIT * juce::MathConstants<float>::sqrt2
                                                               : alignedCrossLength };
    glBegin(GL_LINE_LOOP);
    static auto constexpr QUARTER_PI{ juce::MathConstants<float>::halfPi / 2.0f };
    glVertex3f(std::cos(QUARTER_PI) * diagonalCrossLength, std::sin(QUARTER_PI) * diagonalCrossLength, 0.0f);
    glVertex3f(-std::cos(QUARTER_PI) * diagonalCrossLength, -std::sin(QUARTER_PI) * diagonalCrossLength, 0.0f);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(std::cos(QUARTER_PI * 3.0f) * diagonalCrossLength,
               std::sin(QUARTER_PI * 3.0f) * diagonalCrossLength,
               0.0f);
    glVertex3f(-std::cos(QUARTER_PI * 3.0f) * diagonalCrossLength,
               -std::sin(QUARTER_PI * 3.0f) * diagonalCrossLength,
               0.0f);
    glEnd();

    static auto constexpr HALF_CHAR_WIDTH = 0.025f;
    drawText("X", CartesianVector{ -HALF_CHAR_WIDTH, -MAX_RADIUS, 0.0f }, juce::Colours::white, 0.0005f);
    drawText("Y", CartesianVector{ MAX_RADIUS, -HALF_CHAR_WIDTH, 0.0f }, juce::Colours::white, 0.0005f);
    drawText("Z", CartesianVector{ -HALF_CHAR_WIDTH, 0.0f, MAX_RADIUS }, juce::Colours::white, 0.0005f);

    drawTextOnGrid("0", glm::vec3(0.94f, -0.03f, 0.0f), 0.00035f);
    drawTextOnGrid("90", glm::vec3(-0.08f, -0.9f, 0.0f), 0.00035f);
    drawTextOnGrid("180", glm::vec3(-0.98f, -0.015f, 0.0f), 0.00035f);
    drawTextOnGrid("270", glm::vec3(-0.08f, 0.98f, 0.0f), 0.00035f);
}

//==============================================================================
void SpeakerViewComponent::drawText(juce::String const & val,
                                    CartesianVector const & position,
                                    juce::Colour const color,
                                    float const scale,
                                    bool const camLock) const
{
    ASSERT_OPEN_GL_THREAD;

    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);

    if (camLock) {
        auto const & camPos{ mData.state.cameraPosition };
        auto const azimuth{ camPos.azimuth.toDegrees() + HALF_PI };

        glRotatef(HALF_PI.toDegrees().get(), 1.0f, 0.0f, 0.0f);
        glRotatef(azimuth.get(), 0.0f, 1.0f, 0.0f);
    }

    glScalef(scale, scale, scale);
    glColor4f(color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), color.getAlpha());
    for (auto const c : val) {
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, c);
    }
    glTranslatef(-position.x, -position.y, -position.z);
    glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawTextOnGrid(std::string const & val, glm::vec3 const position, float const scale)
{
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(-HALF_PI.toDegrees().get(), 0.0f, 0.0f, 1.0f);
    // glRotatef(-HALF_PI.toDegrees().get(), 1.0f, 0.0f, 0.0f);

    glScalef(scale, scale, scale);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (auto const c : val) {
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, c);
    }
    glTranslatef(-position.x, -position.y, -position.z);
    glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawTripletConnection() const
{
    ASSERT_OPEN_GL_THREAD;

    auto const & data{ mMainContentComponent.getData() };
    for (auto const & triplet : mMainContentComponent.getTriplets()) {
        auto const & spk1{ data.speakerSetup.speakers[triplet.id1].position };
        auto const & spk2{ data.speakerSetup.speakers[triplet.id2].position };
        auto const & spk3{ data.speakerSetup.speakers[triplet.id3].position };

        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glColor3f(0.8f, 0.8f, 0.8f);
        glVertex3f(spk1.x, spk1.y, spk1.z);
        glVertex3f(spk2.x, spk2.y, spk2.z);
        glVertex3f(spk1.x, spk1.y, spk1.z);
        glVertex3f(spk3.x, spk3.y, spk3.z);
        glVertex3f(spk2.x, spk2.y, spk2.z);
        glVertex3f(spk3.x, spk3.y, spk3.z);
        glEnd();
    }
}

//==============================================================================
void SpeakerViewComponent::drawSource(source_index_t const index, ViewportSourceData const & source) const
{
    ASSERT_OPEN_GL_THREAD;

    if (source.colour.getAlpha() == 0.0f) {
        return;
    }

    // Draw 3D sphere.
    glPushMatrix();
    auto const & pos{ source.position };
    glTranslatef(pos.x, pos.y, pos.z);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(2.0f);

    glColor4f(source.colour.getFloatRed(),
              source.colour.getFloatGreen(),
              source.colour.getFloatBlue(),
              source.colour.getAlpha());

#if defined(__APPLE__)
    glutSolidSphere(static_cast<double>(SPHERE_RADIUS), 8, 8);
#else
    drawSphere(SPHERE_RADIUS);
#endif

    glTranslatef(-pos.x, -pos.y, -pos.z);

    if (source.azimuthSpan != 0.0f || source.zenithSpan != 0.0f) {
        switch (mData.config.spatMode) {
        case SpatMode::lbap:
            drawLbapSpan(source);
            break;
        case SpatMode::vbap:
            drawVbapSpan(source);
            break;
        case SpatMode::hrtfVbap:
        case SpatMode::stereo:
            break;
        default:
            jassertfalse;
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();

    if (mData.config.viewSettings.showSpeakerNumbers) {
        auto position{ source.position };
        position.y += SIZE_SPEAKER + 0.04f;
        drawText(juce::String{ index.get() }, position, source.colour, 0.0003f, true);
    }
}

//==============================================================================
void SpeakerViewComponent::drawVbapSpan(ViewportSourceData const & source)
{
    static auto constexpr NUM{ 8 };

    glPointSize(4);
    glBegin(GL_POINTS);

    auto const polarCoords{ PolarVector::fromCartesian(source.position) };
    auto const & azimuth{ polarCoords.azimuth };
    auto const & elevation{ polarCoords.elevation };
    auto const & length{ polarCoords.length };

    for (int i{}; i < NUM; ++i) {
        auto const aziDev{ azimuth * narrow<float>(i) * 0.5f * 0.42f };
        for (int j{}; j < 2; ++j) {
            auto newAzimuth{ j ? azimuth + aziDev : azimuth - aziDev };

            newAzimuth = newAzimuth.centered();

            {
                auto const vertex{
                    PolarVector{ newAzimuth, elevation, polarCoords.length * MAX_RADIUS }.toCartesian()
                };
                glVertex3f(vertex.x, vertex.y, vertex.z);
            }
            for (int k{}; k < 4; ++k) {
                radians_t const eleDev{ (static_cast<float>(k) + 1.0f) * source.zenithSpan * 2.0f * 0.38f };
                for (int l{}; l < 2; ++l) {
                    auto newElevation{ l ? elevation + eleDev : elevation - eleDev };
                    newElevation = std::clamp(newElevation, radians_t{}, HALF_PI);
                    auto const vertex{ PolarVector{ newAzimuth, newElevation, length * MAX_RADIUS }.toCartesian() };
                    glVertex3f(vertex.x, vertex.y, vertex.z);
                }
            }
        }
    }

    glEnd();
    glPointSize(1);
}

//==============================================================================
void SpeakerViewComponent::drawFieldSphere()
{
    glPushMatrix();
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.0f);
    glRotatef(HALF_PI.toDegrees().get(), 1.0f, 0.0f, 0.0f);
    glColor3f(0.8f, 0.2f, 0.1f);
#if defined(WIN32)
    drawSphere(std::max(MAX_RADIUS, 1.0f));
#else
    glutSolidSphere(std::max(MAX_RADIUS, 1.0f), SPACE_LIMIT, SPACE_LIMIT);
#endif
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawFieldCube()
{
    glPushMatrix();
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.0f);
    glRotatef(HALF_PI.toDegrees().get(), 1.0f, 0.0f, 0.0f);
    glColor3f(0.8f, 0.2f, 0.1f);
    // Draw a cube when in LBAP mode.
    static constexpr int DEFINITION = 10;
    for (auto i{ -DEFINITION }; i <= DEFINITION; i += 2) {
        auto const i_f{ narrow<float>(i) / narrow<float>(DEFINITION) };
        glBegin(GL_LINES);
        glVertex3f(i_f, MAX_RADIUS, -MAX_RADIUS);
        glVertex3f(i_f, MAX_RADIUS, MAX_RADIUS);
        glVertex3f(i_f, -MAX_RADIUS, -MAX_RADIUS);
        glVertex3f(i_f, -MAX_RADIUS, MAX_RADIUS);
        glVertex3f(MAX_RADIUS, -MAX_RADIUS, i_f);
        glVertex3f(MAX_RADIUS, MAX_RADIUS, i_f);
        glVertex3f(-MAX_RADIUS, -MAX_RADIUS, i_f);
        glVertex3f(-MAX_RADIUS, MAX_RADIUS, i_f);
        glVertex3f(MAX_RADIUS, i_f, -MAX_RADIUS);
        glVertex3f(MAX_RADIUS, i_f, MAX_RADIUS);
        glVertex3f(-MAX_RADIUS, i_f, -MAX_RADIUS);
        glVertex3f(-MAX_RADIUS, i_f, MAX_RADIUS);
        glVertex3f(-MAX_RADIUS, i_f, MAX_RADIUS);
        glVertex3f(MAX_RADIUS, i_f, MAX_RADIUS);
        glVertex3f(-MAX_RADIUS, i_f, -MAX_RADIUS);
        glVertex3f(MAX_RADIUS, i_f, -MAX_RADIUS);
        glVertex3f(-MAX_RADIUS, MAX_RADIUS, i_f);
        glVertex3f(MAX_RADIUS, MAX_RADIUS, i_f);
        glVertex3f(-MAX_RADIUS, -MAX_RADIUS, i_f);
        glVertex3f(MAX_RADIUS, -MAX_RADIUS, i_f);
        glVertex3f(i_f, -MAX_RADIUS, MAX_RADIUS);
        glVertex3f(i_f, MAX_RADIUS, MAX_RADIUS);
        glVertex3f(i_f, -MAX_RADIUS, -MAX_RADIUS);
        glVertex3f(i_f, MAX_RADIUS, -MAX_RADIUS);
        glEnd();
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawLbapSpan(ViewportSourceData const & source)
{
    static auto constexpr NUM = 4;

    glPointSize(4);
    glBegin(GL_POINTS);

    auto const & pos{ source.position };

    // For the same elevation as the source position.
    for (auto i{ 1 }; i <= NUM; ++i) {
        auto const raddev{ static_cast<float>(i) / 4.0f * source.azimuthSpan * 6.0f };
        if (i < NUM) {
            glVertex3f(pos.x + raddev, pos.y, pos.z + raddev);
            glVertex3f(pos.x + raddev, pos.y, pos.z - raddev);
            glVertex3f(pos.x - raddev, pos.y, pos.z + raddev);
            glVertex3f(pos.x - raddev, pos.y, pos.z - raddev);
        }
        glVertex3f(pos.x + raddev, pos.y, pos.z);
        glVertex3f(pos.x - raddev, pos.y, pos.z);
        glVertex3f(pos.x, pos.y, pos.z + raddev);
        glVertex3f(pos.x, pos.y, pos.z - raddev);
    }

    // For all other elevation levels.
    for (int j{}; j < NUM; ++j) {
        auto const eledev{ static_cast<float>(j + 1) / 2.0f * source.zenithSpan * 15.0f };
        for (int k{}; k < 2; ++k) {
            float yTmp;
            if (k) {
                yTmp = pos.y + eledev;
                yTmp = yTmp > MAX_RADIUS ? MAX_RADIUS : yTmp;
            } else {
                yTmp = pos.y - eledev;
                yTmp = yTmp < -MAX_RADIUS ? -MAX_RADIUS : yTmp;
            }
            for (auto i{ 1 }; i <= NUM; ++i) {
                auto const raddev{ static_cast<float>(i) / 4.0f * source.azimuthSpan * 6.0f };
                if (i < NUM) {
                    glVertex3f(pos.x + raddev, yTmp, pos.z + raddev);
                    glVertex3f(pos.x + raddev, yTmp, pos.z - raddev);
                    glVertex3f(pos.x - raddev, yTmp, pos.z + raddev);
                    glVertex3f(pos.x - raddev, yTmp, pos.z - raddev);
                }
                glVertex3f(pos.x + raddev, yTmp, pos.z);
                glVertex3f(pos.x - raddev, yTmp, pos.z);
                glVertex3f(pos.x, yTmp, pos.z + raddev);
                glVertex3f(pos.x, yTmp, pos.z - raddev);
            }
        }
    }

    glEnd();
    glPointSize(1);
}

//==============================================================================
void SpeakerViewComponent::drawSpeaker(output_patch_t const outputPatch, ViewportSpeakerConfig const & speaker)
{
    static constexpr auto DEFAULT_ALPHA = 0.75f;

    auto const & showSpeakerLevels{ mData.config.viewSettings.showSpeakerLevels };
    auto const alpha{ showSpeakerLevels ? *mData.speakersAlpha[outputPatch].get() : DEFAULT_ALPHA };

    auto const getSpeakerColour = [&]() {
        if (speaker.isSelected) {
            return COLOR_SPEAKER_SELECT.withAlpha(alpha);
        }
        if (speaker.isDirectOutOnly && !showSpeakerLevels) {
            return COLOR_DIRECT_OUT_SPEAKER.withAlpha(alpha);
        }
        return COLOR_SPEAKER.withAlpha(alpha);
    };

    ASSERT_OPEN_GL_THREAD;

    auto const & center{ speaker.position };
    auto const vector{ PolarVector::fromCartesian(speaker.position) };

    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    glRotatef((PI + vector.azimuth).toDegrees().get(), 0.0f, 0.0f, 1.0f);
    glRotatef(vector.elevation.toDegrees().get(), 0.0f, 1.0f, 0.0f);
    glTranslatef(-center.x, -center.y, -center.z);

    CartesianVector const vertexMin{ center.x - SIZE_SPEAKER, center.y - SIZE_SPEAKER, center.z - SIZE_SPEAKER };
    CartesianVector const vertexMax{ center.x + SIZE_SPEAKER, center.y + SIZE_SPEAKER, center.z + SIZE_SPEAKER };
    auto const colour{ getSpeakerColour() };

    // Draw box
    glBegin(GL_QUADS);
    glColor4f(colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    glVertex3f(vertexMin.x, vertexMin.y, vertexMax.z);
    glVertex3f(vertexMax.x, vertexMin.y, vertexMax.z);
    glVertex3f(vertexMax.x, vertexMax.y, vertexMax.z);
    glVertex3f(vertexMin.x, vertexMax.y, vertexMax.z);

    glVertex3f(vertexMax.x, vertexMin.y, vertexMax.z);
    glVertex3f(vertexMax.x, vertexMin.y, vertexMin.z);
    glVertex3f(vertexMax.x, vertexMax.y, vertexMin.z);
    glVertex3f(vertexMax.x, vertexMax.y, vertexMax.z);

    glVertex3f(vertexMin.x, vertexMax.y, vertexMax.z);
    glVertex3f(vertexMax.x, vertexMax.y, vertexMax.z);
    glVertex3f(vertexMax.x, vertexMax.y, vertexMin.z);
    glVertex3f(vertexMin.x, vertexMax.y, vertexMin.z);

    glVertex3f(vertexMin.x, vertexMin.y, vertexMin.z);
    glVertex3f(vertexMin.x, vertexMax.y, vertexMin.z);
    glVertex3f(vertexMax.x, vertexMax.y, vertexMin.z);
    glVertex3f(vertexMax.x, vertexMin.y, vertexMin.z);

    glVertex3f(vertexMin.x, vertexMin.y, vertexMin.z);
    glVertex3f(vertexMax.x, vertexMin.y, vertexMin.z);
    glVertex3f(vertexMax.x, vertexMin.y, vertexMax.z);
    glVertex3f(vertexMin.x, vertexMin.y, vertexMax.z);

    glVertex3f(vertexMin.x, vertexMin.y, vertexMin.z);
    glVertex3f(vertexMin.x, vertexMin.y, vertexMax.z);
    glVertex3f(vertexMin.x, vertexMax.y, vertexMax.z);
    glVertex3f(vertexMin.x, vertexMax.y, vertexMin.z);

    glEnd();

    // Draw selection wireframe
    if (speaker.isSelected) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);
        glColor3f(0.0f, 0.0f, 0.0f);
        glBegin(GL_LINES);

        static constexpr auto OVER = 0.001f;

        glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMax.z + OVER);

        glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMax.z + OVER);

        glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMin.z - OVER);
        glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMin.z - OVER);
        glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMin.z - OVER);

        glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMax.z + OVER);
        glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMax.z + OVER);

        glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMin.z - OVER);
        glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMin.z - OVER);

        glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMax.z + OVER);
        glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMin.z - OVER);

        glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMax.z + OVER);
        glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMin.z - OVER);

        glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMax.z + OVER);
        glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        glEnd();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // draw direction line
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    if (speaker.isSelected) {
        glColor4f(0.0f, 0.0f, 0.0f, alpha);
    } else {
        glColor4f(0.37f, 0.37f, 0.37f, alpha);
    }
    glVertex3f(center.x + SIZE_SPEAKER, center.y, center.z);
    glVertex3f(center.x + 0.12f, center.y, center.z);
    glEnd();

    glPopMatrix();

    if (mData.config.viewSettings.showSpeakerNumbers) {
        auto posT{ speaker.position };
        posT.z += SIZE_SPEAKER + 0.04f;
        drawText(juce::String{ outputPatch.get() }, posT, juce::Colours::black, 0.0003f);
    }
}

//==============================================================================
float SpeakerViewComponent::rayCast(CartesianVector const & speakerPosition) const
{
    ASSERT_OPEN_GL_THREAD;

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
    ASSERT_OPEN_GL_THREAD;

    auto const camPosition{ mData.state.cameraPosition.toCartesian() };
    auto const distance1{ (speak1 - camPosition).length2() };
    auto const distance2{ (speak2 - camPosition).length2() };
    return distance1 < distance2;
}
