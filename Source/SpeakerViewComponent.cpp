/*
 This file is part of SpatGRIS2.
 
 Developers: Olivier Belanger, Nicolas Masson
 
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

#include "MainComponent.h"

//==============================================================================
SpeakerViewComponent::SpeakerViewComponent(MainContentComponent *parent) {
    this->mainParent = parent;
}

//==============================================================================
SpeakerViewComponent::~SpeakerViewComponent() {
    this->shutdownOpenGL();
}

//==============================================================================
void SpeakerViewComponent::initialise() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glColor3f(1.0, 1.0, 1.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(80, (float)this->getWidth() / this->getHeight(), 0.5f, 75.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(4, 6, 5, 0, 0, 0, 0, 1, 0);

    int argc = 1;
    char *argv[1] = {(char*)"Something"};
    glutInit(&argc, argv);
}

//==============================================================================
void SpeakerViewComponent::shutdown() {}

//==============================================================================
void SpeakerViewComponent::setCamPosition(float angleX, float angleY, float distance) {
    this->camAngleX = angleX;
    this->camAngleY = angleY;
    this->distance = distance;
}

//==============================================================================
void SpeakerViewComponent::render() {
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
    
    this->drawBackground();

    gluPerspective(80, (float)this->getWidth() / this->getHeight(), 0.5f, 75.0f);
    glMatrixMode(GL_MODELVIEW);

    float degToRad = 0.017453292519943295; // M_PI / 180.0f;
    float cosY = cosf(camAngleY * degToRad);
    float camX = -distance * sinf(camAngleX * degToRad) * cosY;
    float camY = distance * sinf(camAngleY * degToRad);
    float camZ = distance * cosf(camAngleX * degToRad) * cosY;

    glLoadIdentity();
    gluLookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);
    this->camPos = glm::vec3(camX, camY, camZ);

    this->drawOriginGrid();

    // NOTE: For the moment, we are just using input values to draw, we aren't
    // changing them, so it's safe to go without the lock. When the function
    // this->mainParent->getLockInputs()->try_lock() returns false, this causes
    // a flicker in the 3D drawing. -belangeo

    if (true) { //(this->mainParent->getLockInputs()->try_lock()) {
        for (unsigned int i = 0; i < this->mainParent->getListSourceInput().size(); ++i) {
            Input *input = this->mainParent->getListSourceInput()[i];
            input->draw();
            if (this->showNumber && input->getGain() != -1.0) {
                glm::vec3 posT = input->getCenter();
                posT.y += SizeSpeaker.y + 0.4f;
                this->drawText(std::to_string(input->getId()), posT, input->getNumberColor(), 0.003f, true, input->getAlpha());
            }
        }
        //this->mainParent->getLockInputs()->unlock();
    }

    if (this->mainParent->getLockSpeakers().try_lock()) {
        if (!this->hideSpeaker) {
            for (unsigned int i = 0; i < this->mainParent->getListSpeaker().size(); ++i) {
                this->mainParent->getListSpeaker()[i]->draw();
                if (this->showNumber) {
                    glm::vec3 posT = this->mainParent->getListSpeaker()[i]->getCenter();
                    posT.y += SizeSpeaker.y + 0.4f;
                    this->drawText(std::to_string(this->mainParent->getListSpeaker()[i]->getOutputPatch()),
                                   posT, glm::vec3(0, 0, 0), 0.003f);
                }
            }
        }
        if (this->showTriplets) {
            this->drawTrippletConn();
        }
        this->mainParent->getLockSpeakers().unlock();
    }
    
    // Draw Sphere : Use many CPU
    if (this->showShpere) {
        if (this->mainParent->getLockSpeakers().try_lock()) {
            float maxRadius = 10.0f;

            // Not sure why we used the farthest speaker to set the size of the sphere.
            // Does not make much sense to me. -belangeo
            //for (unsigned int i = 0; i < this->mainParent->getListSpeaker().size(); ++i) {
            //    if (abs(this->mainParent->getListSpeaker()[i]->getAziZenRad().z * 10.f) > maxRadius) {
            //        maxRadius = abs(this->mainParent->getListSpeaker()[i]->getAziZenRad().z * 10.0f);
            //    }
            //}

            glPushMatrix();
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1);
            glRotatef(90, 1, 0, 0);
            glColor3f(0.8, 0.2, 0.1);
            if (this->mainParent->getModeSelected() == LBAP) {
                // Draw a cube when in LBAP mode.
                for (int i = -10; i <= 10; i += 2) {
                    glBegin(GL_LINES);
                    glVertex3f(i, 10, -10); glVertex3f(i, 10, 10);
                    glVertex3f(i, -10, -10); glVertex3f(i, -10, 10);
                    glVertex3f(10, -10, i); glVertex3f(10, 10, i);
                    glVertex3f(-10, -10, i); glVertex3f(-10, 10, i);
                    glVertex3f(10, i, -10); glVertex3f(10, i, 10);
                    glVertex3f(-10, i, -10); glVertex3f(-10, i, 10);
                    glVertex3f(-10, i, 10); glVertex3f(10, i, 10);
                    glVertex3f(-10, i, -10); glVertex3f(10, i, -10);
                    glVertex3f(-10, 10, i); glVertex3f(10, 10, i);
                    glVertex3f(-10, -10, i); glVertex3f(10, -10, i);
                    glVertex3f(i, -10, 10); glVertex3f(i, 10, 10);
                    glVertex3f(i, -10, -10); glVertex3f(i, 10, -10);
                    glEnd();
                }
            } else {
                glutSolidSphere(std::max(maxRadius, 1.0f), 20, 20);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glPopMatrix();
            this->mainParent->getLockSpeakers().unlock();
        }
    }
    
    if (this->clickLeft) {
        this->clickRay();
    }

    // Draw Click Ray.
    //this->ray.draw();

    glFlush();
}

//==============================================================================
void SpeakerViewComponent::paint (Graphics& g) {
    g.setColour(Colours::white);
    g.setFont(16);
    g.drawText(this->nameConfig, 18, 18, 300, 30, Justification::left);
}

//==============================================================================
void SpeakerViewComponent::clickRay() {
    this->clickLeft = false;
    double matModelView[16], matProjection[16];
    int viewport[4];
    
    glGetDoublev(GL_MODELVIEW_MATRIX, matModelView);
    glGetDoublev(GL_PROJECTION_MATRIX, matProjection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    double winX = this->rayClickX * this->displayScaling;
    double winY = viewport[3] - this->rayClickY * this->displayScaling;
    
    gluUnProject(winX, winY, 0.0, matModelView, matProjection, viewport, &xS, &yS, &zS);
    gluUnProject(winX, winY, 1.0, matModelView, matProjection, viewport, &xE, &yE, &zE);
    
    this->ray.setRay(glm::vec3(xS, yS, zS), glm::vec3(xE, yE, zE));
    
    int iBestSpeaker = -1;
    int selected = -1;
    if (this->mainParent->getLockSpeakers().try_lock()) {
        for (unsigned int i = 0; i < this->mainParent->getListSpeaker().size(); ++i) {
            if (this->mainParent->getListSpeaker()[i]->isSelected()) {
                selected = i;
            }
            if (raycast(this->mainParent->getListSpeaker()[i]) != -1) {
                if (iBestSpeaker == -1) {
                    iBestSpeaker = i;
                } else {
                    if (speakerNearCam(this->mainParent->getListSpeaker()[i]->getCenter(),
                                       this->mainParent->getListSpeaker()[iBestSpeaker]->getCenter())) {
                        iBestSpeaker = i;
                    }
                }
            }
        }
        
        if (this->controlOn && iBestSpeaker >= 0) {
            this->mainParent->selectTripletSpeaker(iBestSpeaker);
        } else {
            if (iBestSpeaker == -1) {
                iBestSpeaker = selected;
            }
            this->mainParent->selectSpeaker(iBestSpeaker);
        }
        this->mainParent->getLockSpeakers().unlock();
    }
    
    this->controlOn = false;
}

//==============================================================================
void SpeakerViewComponent::mouseDown (const MouseEvent& e) {
    this->deltaClickX = this->camAngleX - e.getPosition().x / this->slowDownFactor;
    this->deltaClickY = this->camAngleY - e.getPosition().y / this->slowDownFactor;

    // Always check on which display the speaker view component is.
    this->displayScaling = Desktop::getInstance().getDisplays().findDisplayForPoint(e.getScreenPosition()).scale;

    if (e.mods.isLeftButtonDown()) {
        this->rayClickX = (double)e.getPosition().x;
        this->rayClickY = (double)e.getPosition().y;
        this->clickLeft = true;
        this->controlOn = e.mods.isCtrlDown();
    } else if (e.mods.isRightButtonDown()) {
        this->mainParent->selectSpeaker(-1);
    }
}

//==============================================================================
void SpeakerViewComponent::mouseDrag (const MouseEvent& e) {
    if (e.mods.isLeftButtonDown()) {
        this->camAngleX = e.getPosition().x / this->slowDownFactor + this->deltaClickX;
        this->camAngleY = e.getPosition().y / this->slowDownFactor + this->deltaClickY;
        this->camAngleY = this->camAngleY < -89.99f ? -89.99f : this->camAngleY > 89.99f ? 89.99f : this->camAngleY;
    }
}

//==============================================================================
void SpeakerViewComponent::mouseWheelMove(const MouseEvent& e,const MouseWheelDetails& wheel) {
    this->distance -= (wheel.deltaY * ScroolWheelSpeedMouse);
    this->distance = this->distance < 1.0 ? 1.0 : this->distance > 70.0 ? 70.0 : this->distance;
}

//==============================================================================
void SpeakerViewComponent::drawBackground() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw 2D image.
    glBegin(GL_QUADS);
    glColor3f(0.6, 0.6, 0.6);
    glVertex2f(1.0, 1.0);
    glVertex2f(-1.0, 1.0);
    
    glColor3f(0.3, 0.3, 0.3);
    glVertex2f(0.0, 0.0);
    glVertex2f(1.0, 0.0);
    glEnd();
    
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

//==============================================================================
void SpeakerViewComponent::drawOriginGrid() {
    int i;
    double angle;

    glLineWidth(1.5f);
    glColor3f(0.59, 0.59, 0.59);

    if (this->mainParent->getModeSelected() == LBAP) {
        // Draw light squares.
        for (float j = 3.5; j < 19.f; j += 6.75f) {
            glBegin(GL_LINES);
            glVertex3f(-j, 0, j); glVertex3f(j, 0, j);
            glVertex3f(j, 0, -j); glVertex3f(j, 0, j);
            glVertex3f(j, 0, -j); glVertex3f(-j, 0, -j);
            glVertex3f(-j, 0, -j); glVertex3f(-j, 0, j);
            glEnd();
        }
    } else {
        // Draw light circles.
        for (float j = 5.0; j < 11.f; j += 5.0f) {
            glBegin(GL_LINE_LOOP);
            for (i = 0; i <= 180; ++i) {
                angle = (M2_PI * i / 180);
                glVertex3f(cos(angle) * j, 0.0f, sin(angle) * j);
            }
            glEnd();
        }
    }

    // 3D RGB line.
    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3f(0.4, 0, 0); glVertex3f(-10, 0, 0); glVertex3f(0, 0, 0);
    glColor3f(1, 0, 0);   glVertex3f(0, 0, 0);   glVertex3f(10, 0, 0);
    glColor3f(0, 0, 1);   glVertex3f(0, 0, 0);   glVertex3f(0, 10, 0);
    glColor3f(0, 0.4, 0); glVertex3f(0, 0, -10); glVertex3f(0, 0, 0);
    glColor3f(0, 1, 0);   glVertex3f(0, 0, 0);   glVertex3f(0, 0, 10);
    glEnd();
    
    // Grid.
    glLineWidth(1);
    glColor3f(0.49, 0.49, 0.49);
    
    glBegin(GL_LINE_LOOP);
    glVertex3f(0, 0, -NbrGridLines);
    glVertex3f(0, 0, NbrGridLines);
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    glVertex3f(-NbrGridLines, 0, 0);
    glVertex3f(NbrGridLines, 0, 0);
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    glVertex3f(cos(M_PI4) * 14.5f, 0.0f, sin(M_PI4) * 14.5f);
    glVertex3f(-cos(M_PI4) * 14.5f, 0.0f, -sin(M_PI4) * 14.5f);
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    glVertex3f(cos(M_PI4 * 3.0f) * 14.5f, 0.0f, sin(M_PI4 * 3.0f) * 14.5f);
    glVertex3f(-cos(M_PI4 * 3.0f) * 14.5f, 0.0f, -sin(M_PI4 * 3.0f) * 14.5f);
    glEnd();

    if (this->mainParent->getModeSelected() == LBAP) {
        // Draw grey squares.
        for (float j = 6.875; j < 15.f; j += 6.75f) {
            glBegin(GL_LINES);
            glVertex3f(-j, 0, j); glVertex3f(j, 0, j);
            glVertex3f(j, 0, -j); glVertex3f(j, 0, j);
            glVertex3f(j, 0, -j); glVertex3f(-j, 0, -j);
            glVertex3f(-j, 0, -j); glVertex3f(-j, 0, j);
            glEnd();
        }
    } else {
        // Draw grey circles.
        for (float j = 2.5; j < 13.f; j += 5.0f) {
            glBegin(GL_LINE_LOOP);
            for (i = 0; i <= 180; ++i) {
                angle = (M2_PI * i / 180);
                glVertex3f(cos(angle) * j, 0.0f, sin(angle) * j);
            }
            glEnd();
        }
    }

    drawText("X", glm::vec3(10, 0.1, 0), glm::vec3(1, 1, 1));
    drawText("Y", glm::vec3(0,  0.1, 10), glm::vec3(1, 1, 1));
    drawText("Z", glm::vec3(0,  10,  0), glm::vec3(1, 1, 1));
    
    drawTextOnGrid("0", glm::vec3(9.4,  0, 0.1));
    drawTextOnGrid("90", glm::vec3(-0.8, 0, 9.0));
    drawTextOnGrid("180", glm::vec3(-9.8, 0, 0.1));
    drawTextOnGrid("270", glm::vec3(-0.8, 0,-9.8));
}

//==============================================================================
void SpeakerViewComponent::drawText(std::string val, glm::vec3 position, glm::vec3 color,
                                    float scale, bool camLock, float alpha) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);

    if (camLock) {
        glRotatef((-this->camAngleX), 0, 1, 0);
        if (this->camAngleY < 0 || this->camAngleY > 90.f) {
            glRotatef(-this->camAngleY, 0, 1, 0);
        }
    }

    glScalef(scale, scale, scale);
    glColor4f(color.x, color.y, color.z, alpha);
    for (char & c : val) {
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, c);
    }
    glTranslatef(-position.x, -position.y, -position.z);
    glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawTextOnGrid(std::string val, glm::vec3 position, float scale) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    
    glRotatef(-90.0, 1, 0, 0);
    glRotatef(270.0, 0, 0, 10);
    
    
    glScalef(scale, scale, scale);
    glColor3f(1, 1, 1);
    for (char & c : val) {
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, c);
    }
    glTranslatef(-position.x, -position.y, -position.z);
    glPopMatrix();
}

//==============================================================================
void SpeakerViewComponent::drawTrippletConn() {
    for (unsigned int i = 0; i < this->mainParent->getListTriplet().size(); ++i) {
        Speaker *spk1 = this->mainParent->getSpeakerFromOutputPatch(this->mainParent->getListTriplet()[i].id1);
        Speaker *spk2 = this->mainParent->getSpeakerFromOutputPatch(this->mainParent->getListTriplet()[i].id2);
        Speaker *spk3 = this->mainParent->getSpeakerFromOutputPatch(this->mainParent->getListTriplet()[i].id3);

        if (spk1 != nullptr && spk2 != nullptr && spk3 != nullptr) {
            glLineWidth(1);
            glBegin(GL_LINES);
            glColor3f(0.8, 0.8, 0.8);
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
float SpeakerViewComponent::raycast(Speaker *speaker) {
    float t1 = (speaker->getMin().x - this->ray.getPosition().x) / this->ray.getNormal().x;
    float t2 = (speaker->getMax().x - this->ray.getPosition().x) / this->ray.getNormal().x;
    float t3 = (speaker->getMin().y - this->ray.getPosition().y) / this->ray.getNormal().y;
    float t4 = (speaker->getMax().y - this->ray.getPosition().y) / this->ray.getNormal().y;
    float t5 = (speaker->getMin().z - this->ray.getPosition().z) / this->ray.getNormal().z;
    float t6 = (speaker->getMax().z - this->ray.getPosition().z) / this->ray.getNormal().z;
    
    float tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6));
    float tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6));
    
    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
    if (tmax < 0) {
        return -1;
    }
    
    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
        return -1;
    }
    
    if (tmin < 0.0f) {
        return tmax;
    }
    return tmin;
}

//==============================================================================
bool SpeakerViewComponent::speakerNearCam(glm::vec3 speak1, glm::vec3 speak2) {
    return (sqrt( exp2(speak1.x - this->camPos.x) + exp2(speak1.y - this->camPos.y) +exp2(speak1.z - this->camPos.z) ) <=
            sqrt( exp2(speak2.x - this->camPos.x) + exp2(speak2.y - this->camPos.y) +exp2(speak2.z - this->camPos.z) ));
}
