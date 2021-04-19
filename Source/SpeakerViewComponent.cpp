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

glm::vec3 const SpeakerViewComponent::COLOR_SPEAKER{ 0.87f, 0.87f, 0.87f };
glm::vec3 const SpeakerViewComponent::COLOR_DIRECT_OUT_SPEAKER{ 0.25f, 0.25f, 0.25f };
glm::vec3 const SpeakerViewComponent::COLOR_SPEAKER_SELECT{ 1.00f, 0.64f, 0.09f };
glm::vec3 const SpeakerViewComponent::SIZE_SPEAKER{ 0.5f, 0.5f, 0.5f };
glm::vec3 const SpeakerViewComponent::DEFAULT_CENTER{ 0.0f, 0.0f, 0.0f };

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
    gluLookAt(4.0, 6.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    int noArgs{};
    glutInit(&noArgs, nullptr);
}

//==============================================================================
SpeakerViewComponent::SpeakerViewComponent(MainContentComponent & mainContentComponent)
    : mMainContentComponent(mainContentComponent)
{
}

//==============================================================================
void SpeakerViewComponent::setNameConfig(juce::String const & name)
{
    mNameConfig = name;
    repaint();
}

//==============================================================================
void SpeakerViewComponent::setCamPosition(CartesianVector const & position)
{
    mCamVector = PolarVector::fromCartesian(position);
}

//==============================================================================
void SpeakerViewComponent::render()
{
    jassert(juce::OpenGLHelpers::isContextActive());
    ASSERT_OPEN_GL_THREAD;

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

    auto const camPos{ mCamVector.toCartesian() };

    glLoadIdentity();
    gluLookAt(camPos.x, camPos.y, camPos.z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    drawOriginGrid();

    // TODO : do a try-lock of leave function

    auto const & data{ mMainContentComponent.getData() };

    for (auto const source : data.project.sources) {
        if (!source.value->position) {
            continue;
        }
        drawSource(*source.value, data.appData.spatMode);
        if (mShowNumber) {
            // Show number
            auto position{ *source.value->position };
            position.y += SIZE_SPEAKER.y + 0.4f;
            drawText(juce::String{ source.key.get() }, position, juce::Colours::black, 0.003f, true);
        }
    }

    if (!mHideSpeakers) {
        for (auto const speaker : data.speakerSetup.speakers) {
            drawSpeaker(*speaker.value);
            if (mShowNumber) {
                auto posT{ speaker.value->position };
                posT.y += SIZE_SPEAKER.y + 0.4f;
                drawText(juce::String{ speaker.key.get() }, posT, juce::Colours::black, 0.003f);
            }
        }
    }
    if (mShowTriplets) {
        drawTripletConnection();
    }

    // Draw Sphere / Cube
    if (mShowSphere) {
        glPushMatrix();
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.0f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.8f, 0.2f, 0.1f);
        if (data.appData.spatMode == SpatMode::lbap) {
            // Draw a cube when in LBAP mode.
            for (auto i{ -static_cast<int>(MAX_RADIUS) }; i <= static_cast<int>(MAX_RADIUS); i += 2) {
                auto const i_f{ narrow<float>(i) };
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
        } else {
#if defined(WIN32)
            drawSphere(std::max(MAX_RADIUS, 1.0f));
#else
            glutSolidSphere(std::max(MAX_RADIUS, 1.0f), SPACE_LIMIT, SPACE_LIMIT);
#endif
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glPopMatrix();
    }

    if (mClickLeft) {
        clickRay();
    }

    glFlush();
}

//==============================================================================
void SpeakerViewComponent::paint(juce::Graphics & g)
{
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText(mNameConfig, 18, 18, 300, 30, juce::Justification::left);
}

//==============================================================================
void SpeakerViewComponent::clickRay()
{
    ASSERT_OPEN_GL_THREAD;

    mClickLeft = false;
    double matModelView[16], matProjection[16];
    int viewport[4];

    glGetDoublev(GL_MODELVIEW_MATRIX, matModelView);
    glGetDoublev(GL_PROJECTION_MATRIX, matProjection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    auto const winX{ mRayClickX * mDisplayScaling };
    auto const winY{ viewport[3] - mRayClickY * mDisplayScaling };

    gluUnProject(winX, winY, 0.0, matModelView, matProjection, viewport, &mXs, &mYs, &mZs);
    gluUnProject(winX, winY, 1.0, matModelView, matProjection, viewport, &mXe, &mYe, &mZe);

    mRay.setRay(glm::vec3{ mXs, mYs, mZs }, glm::vec3{ mXe, mYe, mZe });

    tl::optional<output_patch_t> iBestSpeaker{};
    tl::optional<output_patch_t> selected{};
    auto const & speakers{ mMainContentComponent.getData().speakerSetup.speakers };
    // TODO : take lock
    for (auto const speaker : speakers) {
        if (speaker.value->isSelected) {
            selected = speaker.key;
        }
        if (rayCast(*speaker.value) != -1) {
            if (!iBestSpeaker) {
                iBestSpeaker = speaker.key;
            } else {
                if (speakerNearCam(speaker.value->position, speakers[*iBestSpeaker].position)) {
                    iBestSpeaker = speaker.key;
                }
            }
        }
    }

    if (mControlOn && iBestSpeaker) {
        juce::MessageManager::callAsync(
            [=] { mMainContentComponent.handleSpeakerSelected(juce::Array<output_patch_t>{ *iBestSpeaker }); });
    }

    mControlOn = false;
}

//==============================================================================
void SpeakerViewComponent::mouseDown(const juce::MouseEvent & e)
{
    // TODO : this used cam angle before : why?
    mDeltaClickX = mCamVector.azimuth.get() - e.getPosition().x / mSlowDownFactor;
    mDeltaClickY = mCamVector.elevation.get() - e.getPosition().y / mSlowDownFactor;

    // Always check on which display the speaker view component is.
    mDisplayScaling = juce::Desktop::getInstance().getDisplays().getDisplayForPoint(e.getScreenPosition())->scale;

    if (e.mods.isLeftButtonDown()) {
        mRayClickX = narrow<double>(e.getPosition().x);
        mRayClickY = narrow<double>(e.getPosition().y);
        mClickLeft = true;
        mControlOn = e.mods.isCtrlDown();
    } else if (e.mods.isRightButtonDown()) {
        mMainContentComponent.handleSpeakerSelected(juce::Array<output_patch_t>{});
    }
}

//==============================================================================
void SpeakerViewComponent::mouseDrag(const juce::MouseEvent & e)
{
    static constexpr radians_t NEARLY_90_DEG{ degrees_t{ 89.99f } };

    if (e.mods.isLeftButtonDown()) {
        mCamVector.azimuth = radians_t{ e.getPosition().x / mSlowDownFactor + mDeltaClickX };
        mCamVector.elevation = std::clamp(radians_t{ e.getPosition().y / mSlowDownFactor + mDeltaClickY },
                                          -NEARLY_90_DEG,
                                          NEARLY_90_DEG);
    }
}

//==============================================================================
void SpeakerViewComponent::mouseWheelMove(const juce::MouseEvent & /*e*/, const juce::MouseWheelDetails & wheel)
{
    mCamVector.length -= std::clamp(wheel.deltaY * SCROLL_WHEEL_SPEED_MOUSE, 1.0f, 70.0f);
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
        glVertex3f(-length, 0.0f, length);
        glVertex3f(length, 0.0f, length);
        glVertex3f(length, 0.0f, -length);
        glVertex3f(length, 0.0f, length);
        glVertex3f(length, 0.0f, -length);
        glVertex3f(-length, 0.0f, -length);
        glVertex3f(-length, 0.0f, -length);
        glVertex3f(-length, 0.0f, length);
        glEnd();
    };

    static auto const drawCircle = [](float const radius) -> void {
        glBegin(GL_LINE_LOOP);
        for (int i{}; i <= 180; ++i) {
            auto const angle{ juce::MathConstants<float>::twoPi * narrow<float>(i) / 180.0f };
            glVertex3f(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
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
        drawCircle(2.5f);
        drawCircle(7.5f);
        drawCircle(12.5f);
    }

    // 3D RGB line.
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex3f(-MAX_RADIUS, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(MAX_RADIUS, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, MAX_RADIUS, 0.0f);
    glColor3f(0.0f, 0.4f, 0.0f);
    glVertex3f(0.0f, 0.0f, -MAX_RADIUS);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, MAX_RADIUS);
    glEnd();

    // Grid.
    glLineWidth(1.0f);
    glColor3f(0.49f, 0.49f, 0.49f);

    // Draw aligned cross
    auto const alignedCrossLength{ spatMode == SpatMode::lbap ? SPACE_LIMIT : MAX_RADIUS * 1.5f };
    glBegin(GL_LINE_LOOP);
    glVertex3f(0.0f, 0.0f, -alignedCrossLength);
    glVertex3f(0.0f, 0.0f, alignedCrossLength);
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
    glVertex3f(std::cos(QUARTER_PI) * diagonalCrossLength, 0.0f, std::sin(QUARTER_PI) * diagonalCrossLength);
    glVertex3f(-std::cos(QUARTER_PI) * diagonalCrossLength, 0.0f, -std::sin(QUARTER_PI) * diagonalCrossLength);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(std::cos(QUARTER_PI * 3.0f) * diagonalCrossLength,
               0.0f,
               std::sin(QUARTER_PI * 3.0f) * diagonalCrossLength);
    glVertex3f(-std::cos(QUARTER_PI * 3.0f) * diagonalCrossLength,
               0.0f,
               -std::sin(QUARTER_PI * 3.0f) * diagonalCrossLength);
    glEnd();

    drawText("X", CartesianVector{ 0.0f, 0.1f, MAX_RADIUS }, juce::Colours::white);
    drawText("Y", CartesianVector{ MAX_RADIUS, 0.1f, 0.0f }, juce::Colours::white);
    drawText("Z", CartesianVector{ 0.0f, MAX_RADIUS, 0.0f }, juce::Colours::white);

    drawTextOnGrid("0", glm::vec3(9.4f, 0.0f, 0.1f));
    drawTextOnGrid("90", glm::vec3(-0.8f, 0.0f, 9.0f));
    drawTextOnGrid("180", glm::vec3(-9.8f, 0.0f, 0.1f));
    drawTextOnGrid("270", glm::vec3(-0.8f, 0.0f, -9.8f));
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
        glRotatef((-mCamVector.azimuth.get()), 0.0f, 1.0f, 0.0f);
        if (mCamVector.elevation.get() < 0.0f || mCamVector.elevation.get() > 90.0f) {
            glRotatef(-mCamVector.elevation.get(), 0.0f, 1.0f, 0.0f);
        }
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

    glRotatef(-90.0, 1.0f, 0.0f, 0.0f);
    glRotatef(270.0, 0.0f, 0.0f, MAX_RADIUS);

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
void SpeakerViewComponent::drawSource(SourceData const & source, SpatMode const spatMode) const
{
    static auto constexpr ALPHA{ 0.75f };

    ASSERT_OPEN_GL_THREAD;

    if (!source.position) {
        return;
    }

    // If isSourceLevelShown is on and alpha below 0.01, don't draw.
    if (mMainContentComponent.isSourceLevelShown() && getAlpha() <= 0.01f) {
        return;
    }

    // Draw 3D sphere.
    glPushMatrix();
    auto const & pos{ *source.position };
    glTranslatef(pos.x, pos.y, pos.z);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(2.0f);

    auto const & color{ source.colour };
    if (mMainContentComponent.isSourceLevelShown()) {
        glColor4f(color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), color.getAlpha());
    } else {
        glColor4f(color.getFloatRed(), color.getFloatGreen(), color.getFloatBlue(), ALPHA);
    }

#if defined(__APPLE__)
    glutSolidSphere(static_cast<double>(SPHERE_RADIUS), 8, 8);
#else
    drawSphere(SPHERE_RADIUS);
#endif

    glTranslatef(-pos.x, -pos.y, -pos.z);

    if ((source.azimuthSpan != 0.0f || source.zenithSpan != 0.0f) && mMainContentComponent.isSpanShown()) {
        switch (spatMode) {
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
}

//==============================================================================
void SpeakerViewComponent::drawVbapSpan(SourceData const & source)
{
    static auto constexpr NUM{ 8 };

    if (!source.vector) {
        return;
    }

    glPointSize(4);
    glBegin(GL_POINTS);

    auto const & azimuth{ source.vector->azimuth };
    auto const & elevation{ source.vector->elevation };
    auto const & length{ source.vector->length };

    for (int i{}; i < NUM; ++i) {
        auto const aziDev{ azimuth * narrow<float>(i) * 0.5f * 0.42f };
        for (int j{}; j < 2; ++j) {
            auto newAzimuth{ j ? azimuth + aziDev : azimuth - aziDev };

            newAzimuth = newAzimuth.centered();

            {
                auto const vertex{
                    PolarVector{ newAzimuth, elevation, source.vector->length * MAX_RADIUS }.toCartesian()
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
void SpeakerViewComponent::drawLbapSpan(SourceData const & source)
{
    static auto constexpr NUM = 4;

    if (!source.position) {
        return;
    }

    glPointSize(4);
    glBegin(GL_POINTS);

    auto const & pos{ *source.position };

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
void SpeakerViewComponent::drawSpeaker(SpeakerData const & speaker) const
{
    static auto constexpr ALPHA = 0.75f;

    ASSERT_OPEN_GL_THREAD;

    auto const & center{ speaker.position };
    auto const & vector{ speaker.vector };

    glPushMatrix();

    glTranslatef(center.x, center.y, center.z);

    glRotatef(180.0f - vector.azimuth.toDegrees().get(), 0.0f, 1.0f, 0.0f);
    // TODO : why a branch here ?
    // if (mMainContentComponent.getModeSelected() == SpatMode::lbap) {
    glRotatef(-vector.elevation.get() + vector.elevation.get() * vector.length / 20.0f, 0.0f, 0.0f, 1.0f);
    //}
    /* else {
        glRotatef(-mAziZenRad.y, 0.0f, 0.0f, 1.0f);
    }*/
    glTranslatef(-1.0f * center.x, -1.0f * center.y, -1.0f * center.z);

    glBegin(GL_QUADS);

    if (mMainContentComponent.isSpeakerLevelShown()) {
        static constexpr auto USED_TO_BE_OLD_COLOR{ 1.0f };
        auto const alpha{ getAlpha() };
        auto const levelColour = alpha + (USED_TO_BE_OLD_COLOR - alpha) * 0.5f;
        glColor4f(levelColour, levelColour, levelColour, ALPHA);
    } else {
        glColor4f(COLOR_SPEAKER.x, COLOR_SPEAKER.y, COLOR_SPEAKER.z, ALPHA);
    }

    CartesianVector const vertexMin{ center.x - SIZE_SPEAKER.x, center.y - SIZE_SPEAKER.y, center.z - SIZE_SPEAKER.z };
    CartesianVector const vertexMax{ center.x + SIZE_SPEAKER.x, center.y + SIZE_SPEAKER.y, center.z + SIZE_SPEAKER.z };

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

    if (speaker.isSelected) {
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glColor4f(0.0f, 0.0f, 0.0f, ALPHA);
        glVertex3f(center.x + SIZE_SPEAKER.x, center.y, center.z);
        glVertex3f(center.x + 1.2f, center.y, center.z);
        glEnd();

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(4.0f);
        glBegin(GL_LINES);

        jassertfalse;
        // TODO : what does OVER mean?
        // glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        // glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMax.z + OVER);

        // glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        // glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMax.z + OVER);

        // glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMin.z - OVER);
        // glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        // glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMin.z - OVER);
        // glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        // glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        // glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMin.z - OVER);

        // glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMax.z + OVER);
        // glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMax.z + OVER);

        // glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMin.z - OVER);
        // glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMin.z - OVER);

        // glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMax.z + OVER);
        // glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        // glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        // glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMin.z - OVER);

        // glVertex3f(vertexMin.x - OVER, vertexMin.y - OVER, vertexMax.z + OVER);
        // glVertex3f(vertexMin.x - OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        // glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMin.z - OVER);
        // glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMin.z - OVER);

        // glVertex3f(vertexMax.x + OVER, vertexMin.y - OVER, vertexMax.z + OVER);
        // glVertex3f(vertexMax.x + OVER, vertexMax.y + OVER, vertexMax.z + OVER);

        glEnd();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glColor4f(0.37f, 0.37f, 0.37f, ALPHA);
        glVertex3f(center.x + SIZE_SPEAKER.x, center.y, center.z);
        glVertex3f(center.x + 1.2f, center.y, center.z);
        glEnd();
    }

    glPopMatrix();
}

//==============================================================================
float SpeakerViewComponent::rayCast(SpeakerData const & /*speaker*/) const
{
    ASSERT_OPEN_GL_THREAD;

    // TODO

    // jassertfalse;
    return -1.0f;

    // auto const t1{ (speaker->getMin().x - mRay.getPosition().x) / mRay.getNormal().x };
    // auto const t2{ (speaker->getMax().x - mRay.getPosition().x) / mRay.getNormal().x };
    // auto const t3{ (speaker->getMin().y - mRay.getPosition().y) / mRay.getNormal().y };
    // auto const t4{ (speaker->getMax().y - mRay.getPosition().y) / mRay.getNormal().y };
    // auto const t5{ (speaker->getMin().z - mRay.getPosition().z) / mRay.getNormal().z };
    // auto const t6{ (speaker->getMax().z - mRay.getPosition().z) / mRay.getNormal().z };

    // auto const tMin{ std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6)) };
    // auto const tMax{ std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6)) };

    //// if tMax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
    // if (tMax < 0) {
    //    return -1;
    //}

    //// if tMin > tMax, ray doesn't intersect AABB
    // if (tMin > tMax) {
    //    return -1;
    //}

    // if (tMin < 0.0f) {
    //    return tMax;
    //}
    // return tMin;
}

//==============================================================================
bool SpeakerViewComponent::speakerNearCam(CartesianVector const & speak1, CartesianVector const & speak2) const
{
    ASSERT_OPEN_GL_THREAD;

    auto const camPosition{ mCamVector.toCartesian() };
    auto const distance1{ (speak1 - camPosition).length2() };
    auto const distance2{ (speak2 - camPosition).length2() };
    return distance1 < distance2;
}
