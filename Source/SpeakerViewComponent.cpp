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

#include "SpeakerViewComponent.h"

#include "GlSphere.h"
#include "MainComponent.h"

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

    // TODO : undefined behavior
    // auto argc = 1;
    // char * argv[1] = { const_cast<char *>("Something") };
    // glutInit(&argc, argv);
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
void SpeakerViewComponent::setCamPosition(float const angleX, float const angleY, float const distance)
{
    mCamAngleX = angleX;
    mCamAngleY = angleY;
    mDistance = distance;
}

//==============================================================================
void SpeakerViewComponent::render()
{
    jassert(juce::OpenGLHelpers::isContextActive());

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH, GL_NICEST);

    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH, GL_NICEST);

#if defined(WIN32) || defined(_WIN64)

#else
    glEnable(GL_MULTISAMPLE_ARB);
    glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
#endif

    drawBackground();

    gluPerspective(80.0, static_cast<GLdouble>(getWidth()) / static_cast<GLdouble>(getHeight()), 0.5, 75.0);
    glMatrixMode(GL_MODELVIEW);

    static auto constexpr DEG_TO_RAD{ juce::MathConstants<GLdouble>::pi / 180.0 };
    auto const cosY{ std::cos(mCamAngleY * DEG_TO_RAD) };
    auto const camX{ -mDistance * std::sin(mCamAngleX * DEG_TO_RAD) * cosY };
    auto const camY{ mDistance * std::sin(mCamAngleY * DEG_TO_RAD) };
    auto const camZ{ mDistance * std::cos(mCamAngleX * DEG_TO_RAD) * cosY };

    glLoadIdentity();
    gluLookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);
    mCamPos = glm::vec3(camX, camY, camZ);

    drawOriginGrid();

    // NOTE: For the moment, we are just using input values to draw, we aren't
    // changing them, so it's safe to go without the lock. When the function
    // mMainContentComponent.getInputsLock()->try_lock() returns false, this causes
    // a flicker in the 3D drawing. -belangeo

    // if (mMainContentComponent.getInputsLock().try_lock()) {
    for (auto * input : mMainContentComponent.getSourceInputs()) {
        input->draw();
        if (mShowNumber && input->getGain() != -1.0) {
            auto posT{ input->getCenter() };
            posT.y += SIZE_SPEAKER.y + 0.4f;
            drawText(std::to_string(input->getId()), posT, input->getNumberColor(), 0.003f, true, input->getAlpha());
        }
    }
    //    mMainContentComponent.getInputsLock().unlock();
    //}

    if (mMainContentComponent.getSpeakersLock().try_lock()) {
        if (!mHideSpeaker) {
            for (auto * speaker : mMainContentComponent.getSpeakers()) {
                speaker->draw();
                if (mShowNumber) {
                    auto posT{ speaker->getCenter() };
                    posT.y += SIZE_SPEAKER.y + 0.4f;
                    drawText(std::to_string(speaker->getOutputPatch()), posT, glm::vec3{ 0, 0, 0 }, 0.003f);
                }
            }
        }
        if (mShowTriplets) {
            drawTripletConnection();
        }
        mMainContentComponent.getSpeakersLock().unlock();
    }

    // Draw Sphere : Use many CPU
    if (mShowSphere) {
        if (mMainContentComponent.getSpeakersLock().try_lock()) {
            static auto constexpr MAX_RADIUS{ 10.0 };

            // Not sure why we used the farthest speaker to set the size of the sphere.
            // Does not make much sense to me. -belangeo
            // for (unsigned int i = 0; i < mMainContentComponent.getSpeakers().size(); ++i) {
            //    if (abs(mMainContentComponent.getSpeakers()[i]->getAziZenRad().z * 10.f) > maxRadius) {
            //        maxRadius = abs(mMainContentComponent.getSpeakers()[i]->getAziZenRad().z * 10.0f);
            //    }
            //}

            glPushMatrix();
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.0f);
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            glColor3f(0.8f, 0.2f, 0.1f);
            if (mMainContentComponent.getModeSelected() == ModeSpatEnum::LBAP) {
                // Draw a cube when in LBAP mode.
                for (auto i{ -10 }; i <= 10; i += 2) {
                    auto const i_f{ static_cast<float>(i) };
                    glBegin(GL_LINES);
                    glVertex3f(i_f, 10.0f, -10.0f);
                    glVertex3f(i_f, 10.0f, 10.0f);
                    glVertex3f(i_f, -10.0f, -10.0f);
                    glVertex3f(i_f, -10.0f, 10.0f);
                    glVertex3f(10.0f, -10.0f, i_f);
                    glVertex3f(10.0f, 10.0f, i_f);
                    glVertex3f(-10.0f, -10.0f, i_f);
                    glVertex3f(-10.0f, 10.0f, i_f);
                    glVertex3f(10.0f, i_f, -10.0f);
                    glVertex3f(10.0f, i_f, 10.0f);
                    glVertex3f(-10.0f, i_f, -10.0f);
                    glVertex3f(-10.0f, i_f, 10.0f);
                    glVertex3f(-10.0f, i_f, 10.0f);
                    glVertex3f(10.0f, i_f, 10.0f);
                    glVertex3f(-10.0f, i_f, -10.0f);
                    glVertex3f(10.0f, i_f, -10.0f);
                    glVertex3f(-10.0f, 10.0f, i_f);
                    glVertex3f(10.0f, 10.0f, i_f);
                    glVertex3f(-10.0f, -10.0f, i_f);
                    glVertex3f(10.0f, -10.0f, i_f);
                    glVertex3f(i_f, -10.0f, 10.0f);
                    glVertex3f(i_f, 10.0f, 10.0f);
                    glVertex3f(i_f, -10.0f, -10.0f);
                    glVertex3f(i_f, 10.0f, -10.0f);
                    glEnd();
                }
            } else {
#if defined(WIN32)
                drawSphere(std::max(MAX_RADIUS, 1.0));
#else
                glutSolidSphere(std::max(MAX_RADIUS, 1.0), 20, 20);
#endif
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glPopMatrix();
            mMainContentComponent.getSpeakersLock().unlock();
        }
    }

    if (mClickLeft) {
        clickRay();
    }

    // Draw Click Ray.
    // ray.draw();

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

    auto iBestSpeaker = -1;
    auto selected = -1;
    if (mMainContentComponent.getSpeakersLock().try_lock()) {
        for (int i{}; i < mMainContentComponent.getSpeakers().size(); ++i) {
            auto const * speaker{ mMainContentComponent.getSpeakers()[i] };
            if (speaker->isSelected()) {
                selected = i;
            }
            if (rayCast(speaker) != -1) {
                if (iBestSpeaker == -1) {
                    iBestSpeaker = i;
                } else {
                    if (speakerNearCam(speaker->getCenter(),
                                       mMainContentComponent.getSpeakers()[iBestSpeaker]->getCenter())) {
                        iBestSpeaker = i;
                    }
                }
            }
        }

        if (mControlOn && iBestSpeaker >= 0) {
            mMainContentComponent.selectTripletSpeaker(iBestSpeaker);
        } else {
            if (iBestSpeaker == -1) {
                iBestSpeaker = selected;
            }
            mMainContentComponent.selectSpeaker(iBestSpeaker);
        }
        mMainContentComponent.getSpeakersLock().unlock();
    }

    mControlOn = false;
}

//==============================================================================
void SpeakerViewComponent::mouseDown(const juce::MouseEvent & e)
{
    mDeltaClickX = mCamAngleX - e.getPosition().x / mSlowDownFactor;
    mDeltaClickY = mCamAngleY - e.getPosition().y / mSlowDownFactor;

    // Always check on which display the speaker view component is.
    mDisplayScaling = juce::Desktop::getInstance().getDisplays().findDisplayForPoint(e.getScreenPosition()).scale;

    if (e.mods.isLeftButtonDown()) {
        mRayClickX = static_cast<double>(e.getPosition().x);
        mRayClickY = static_cast<double>(e.getPosition().y);
        mClickLeft = true;
        mControlOn = e.mods.isCtrlDown();
    } else if (e.mods.isRightButtonDown()) {
        mMainContentComponent.selectSpeaker(-1);
    }
}

//==============================================================================
void SpeakerViewComponent::mouseDrag(const juce::MouseEvent & e)
{
    if (e.mods.isLeftButtonDown()) {
        mCamAngleX = e.getPosition().x / mSlowDownFactor + mDeltaClickX;
        mCamAngleY = e.getPosition().y / mSlowDownFactor + mDeltaClickY;
        mCamAngleY = mCamAngleY < -89.99f ? -89.99f : mCamAngleY > 89.99f ? 89.99f : mCamAngleY;
    }
}

//==============================================================================
void SpeakerViewComponent::mouseWheelMove(const juce::MouseEvent & /*e*/, const juce::MouseWheelDetails & wheel)
{
    mDistance -= wheel.deltaY * SCROLL_WHEEL_SPEED_MOUSE;
    mDistance = std::clamp(mDistance, 1.0f, 70.f);
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
    glLineWidth(1.5f);
    glColor3f(0.59f, 0.59f, 0.59f);

    if (mMainContentComponent.getModeSelected() == ModeSpatEnum::LBAP) {
        // Draw light squares.
        for (auto j{ 3.5f }; j < 19.0f; j += 6.75f) {
            glBegin(GL_LINES);
            glVertex3f(-j, 0.0f, j);
            glVertex3f(j, 0.0f, j);
            glVertex3f(j, 0.0f, -j);
            glVertex3f(j, 0.0f, j);
            glVertex3f(j, 0.0f, -j);
            glVertex3f(-j, 0.0f, -j);
            glVertex3f(-j, 0.0f, -j);
            glVertex3f(-j, 0.0f, j);
            glEnd();
        }
    } else {
        // Draw light circles.
        for (auto j{ 5.0f }; j < 11.f; j += 5.0f) {
            glBegin(GL_LINE_LOOP);
            for (int i{}; i <= 180; ++i) {
                auto const angle{ juce::MathConstants<float>::twoPi * static_cast<float>(i) / 180.0f };
                glVertex3f(std::cos(angle) * j, 0.0f, std::sin(angle) * j);
            }
            glEnd();
        }
    }

    // 3D RGB line.
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex3f(-10.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor3f(1.0f, 0, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(10.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    glColor3f(0.0f, 0.4f, 0.0f);
    glVertex3f(0.0f, 0.0f, -10.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 10.0f);
    glEnd();

    // Grid.
    glLineWidth(1.0f);
    glColor3f(0.49f, 0.49f, 0.49f);

    glBegin(GL_LINE_LOOP);
    glVertex3f(0.0f, 0.0f, static_cast<float>(-NUM_GRID_LINES));
    glVertex3f(0.0f, 0.0f, static_cast<float>(NUM_GRID_LINES));
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(static_cast<float>(-NUM_GRID_LINES), 0.0f, 0.0f);
    glVertex3f(static_cast<float>(NUM_GRID_LINES), 0.0f, 0.0f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    static auto constexpr quarterPi{ juce::MathConstants<float>::halfPi / 2.0f };
    glVertex3f(std::cos(quarterPi) * 14.5f, 0.0f, std::sin(quarterPi) * 14.5f);
    glVertex3f(-std::cos(quarterPi) * 14.5f, 0.0f, -std::sin(quarterPi) * 14.5f);

    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(std::cos(quarterPi * 3.0f) * 14.5f, 0.0f, std::sin(quarterPi * 3.0f) * 14.5f);
    glVertex3f(-std::cos(quarterPi * 3.0f) * 14.5f, 0.0f, -std::sin(quarterPi * 3.0f) * 14.5f);
    glEnd();

    if (mMainContentComponent.getModeSelected() == ModeSpatEnum::LBAP) {
        // Draw grey squares.
        for (auto j{ 6.875f }; j < 15.0f; j += 6.75f) {
            glBegin(GL_LINES);
            glVertex3f(-j, 0.0f, j);
            glVertex3f(j, 0.0f, j);
            glVertex3f(j, 0.0f, -j);
            glVertex3f(j, 0.0f, j);
            glVertex3f(j, 0.0f, -j);
            glVertex3f(-j, 0.0f, -j);
            glVertex3f(-j, 0.0f, -j);
            glVertex3f(-j, 0.0f, j);
            glEnd();
        }
    } else {
        // Draw grey circles.
        for (auto j{ 2.5f }; j < 13.0f; j += 5.0f) {
            glBegin(GL_LINE_LOOP);
            for (int i{}; i <= 180; ++i) {
                auto const angle = (juce::MathConstants<float>::twoPi * static_cast<float>(i) / 180.0f);
                glVertex3f(std::cos(angle) * j, 0.0f, std::sin(angle) * j);
            }
            glEnd();
        }
    }

    drawText("X", glm::vec3(10.0f, 0.1f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    drawText("Y", glm::vec3(0.0f, 0.1f, 10.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    drawText("Z", glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));

    drawTextOnGrid("0", glm::vec3(9.4f, 0.0f, 0.1f));
    drawTextOnGrid("90", glm::vec3(-0.8f, 0.0f, 9.0f));
    drawTextOnGrid("180", glm::vec3(-9.8f, 0.0f, 0.1f));
    drawTextOnGrid("270", glm::vec3(-0.8f, 0.0f, -9.8f));
}

//==============================================================================
void SpeakerViewComponent::drawText(std::string const & val,
                                    glm::vec3 const position,
                                    glm::vec3 const color,
                                    float const scale,
                                    bool const camLock,
                                    float const alpha) const
{
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);

    if (camLock) {
        glRotatef((-mCamAngleX), 0, 1, 0);
        if (mCamAngleY < 0 || mCamAngleY > 90.f) {
            glRotatef(-mCamAngleY, 0, 1, 0);
        }
    }

    glScalef(scale, scale, scale);
    glColor4f(color.x, color.y, color.z, alpha);
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
    glRotatef(270.0, 0.0f, 0.0f, 10.0f);

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
    for (auto const & triplet : mMainContentComponent.getTriplets()) {
        auto const * spk1{ mMainContentComponent.getSpeakerFromOutputPatch(triplet.id1) };
        auto const * spk2{ mMainContentComponent.getSpeakerFromOutputPatch(triplet.id2) };
        auto const * spk3{ mMainContentComponent.getSpeakerFromOutputPatch(triplet.id3) };

        if (spk1 != nullptr && spk2 != nullptr && spk3 != nullptr) {
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            glColor3f(0.8f, 0.8f, 0.8f);
            glVertex3f(spk1->getCenter().x, spk1->getCenter().y, spk1->getCenter().z);
            glVertex3f(spk2->getCenter().x, spk2->getCenter().y, spk2->getCenter().z);
            glVertex3f(spk1->getCenter().x, spk1->getCenter().y, spk1->getCenter().z);
            glVertex3f(spk3->getCenter().x, spk3->getCenter().y, spk3->getCenter().z);
            glVertex3f(spk2->getCenter().x, spk2->getCenter().y, spk2->getCenter().z);
            glVertex3f(spk3->getCenter().x, spk3->getCenter().y, spk3->getCenter().z);
            glEnd();
        }
    }
}

//==============================================================================
float SpeakerViewComponent::rayCast(Speaker const * const speaker) const
{
    auto const t1{ (speaker->getMin().x - mRay.getPosition().x) / mRay.getNormal().x };
    auto const t2{ (speaker->getMax().x - mRay.getPosition().x) / mRay.getNormal().x };
    auto const t3{ (speaker->getMin().y - mRay.getPosition().y) / mRay.getNormal().y };
    auto const t4{ (speaker->getMax().y - mRay.getPosition().y) / mRay.getNormal().y };
    auto const t5{ (speaker->getMin().z - mRay.getPosition().z) / mRay.getNormal().z };
    auto const t6{ (speaker->getMax().z - mRay.getPosition().z) / mRay.getNormal().z };

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
bool SpeakerViewComponent::speakerNearCam(glm::vec3 const speak1, glm::vec3 const speak2) const
{
    return (std::sqrt(exp2(speak1.x - mCamPos.x) + exp2(speak1.y - mCamPos.y) + exp2(speak1.z - mCamPos.z))
            <= std::sqrt(std::exp2(speak2.x - mCamPos.x) + exp2(speak2.y - mCamPos.y) + exp2(speak2.z - mCamPos.z)));
}
