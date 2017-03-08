/*
 This file is part of spatServerGRIS.
 
 spatServerGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 spatServerGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with spatServerGRIS.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "SpeakerViewComponent.h"

//==========================================================================================
// ACTUAL SPEAKER VIEW CLASS DEFS
//==========================================================================================
SpeakerViewComponent::SpeakerViewComponent() {
    //openGLContext.setMultisamplingEnabled (true);
    
    perspectivCam = glm::vec4(80.0, (16.0/9.0), 0.5, 45);
    setSize(400, 400);
    
}

SpeakerViewComponent::~SpeakerViewComponent() {
    shutdownOpenGL();
}

void SpeakerViewComponent::initialise() {
   
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glColor3f(1.0, 1.0, 1.0);

    

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(perspectivCam.x, perspectivCam.y, perspectivCam.z, perspectivCam.w);
    

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(4, 6, 5, 0, 0, 0, 0, 1, 0);
    

}

void SpeakerViewComponent::shutdown() {
    
}



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

    glEnable(GL_MULTISAMPLE_ARB);
    glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
    
    
    
    
    
    drawBackground();
    
    gluPerspective(perspectivCam.x, perspectivCam.y, perspectivCam.z, perspectivCam.w);
    glMatrixMode(GL_MODELVIEW);

    
    float camX = distance * sinf(camAngleX*(M_PI/180.f)) * cosf((camAngleY)*(M_PI/180.f));
    float camY = distance * sinf((camAngleY)*(M_PI/180.f));
    float camZ = distance * cosf((camAngleX)*(M_PI/180.f)) * cosf((camAngleY)*(M_PI/180.f));
    
    
    glLoadIdentity();
    gluLookAt(camX, camY, camZ, 0, 0, 0, 0,1,0);
    camPos = glm::vec3(camX,camY,camZ);
    
    

    drawOriginGrid();
    

    for(int i = 0; i < listSpeaker.size(); ++i) {
        listSpeaker[i]->draw();
    }


    glFlush();
}

void SpeakerViewComponent::paint (Graphics& g) {
//    // You can add your component specific drawing code here!
//    // This will draw over the top of the openGL background.
//    
    g.setColour(Colours::white);
    g.setFont (16);
    g.drawText (this->nameConfig, 25, 20, 300, 30, Justification::left);
//    g.drawLine (20, 20, 170, 20);
//    g.drawLine (20, 50, 170, 50);
}

void SpeakerViewComponent::resized() {
    //draggableOrientation.setViewport (getLocalBounds());
}


void SpeakerViewComponent::mouseDown (const MouseEvent& e) {
    
    deltaClickX = camAngleX - e.getPosition().x ;
    deltaClickY = camAngleY - e.getPosition().y;

    if(e.mods.isLeftButtonDown()){
        
        double matModelView[16], matProjection[16];
        int viewport[4];
        
        glGetDoublev( GL_MODELVIEW_MATRIX, matModelView );
        glGetDoublev( GL_PROJECTION_MATRIX, matProjection );
        glGetIntegerv( GL_VIEWPORT, viewport );
        double winX = (double)e.getPosition().x;
        double winY = viewport[3] - (double)e.getPosition().y;
        
        GLdouble xS, yS, zS;
        GLdouble xE, yE, zE;
        
        gluUnProject(winX, winY, 0.0, matModelView, matProjection,viewport, &xS, &yS,&zS);
        gluUnProject(winX, winY, 1.0, matModelView, matProjection, viewport, &xE, &yE, &zE);
        
        r = Ray(glm::vec3(xS, yS, zS),glm::vec3(xE, yE, zE));
        
        int iBestSpeaker = -1;
        for(int i = 0; i < listSpeaker.size(); ++i) {
            if (ToolsGL::Raycast(r, *listSpeaker[i]) != -1 ) {
                if(iBestSpeaker == -1){
                    iBestSpeaker = i;
                }else{
                    if(ToolsGL::speakerNearCam(listSpeaker[i]->getCenter(), listSpeaker[iBestSpeaker]->getCenter(), camPos)){
                        iBestSpeaker = i;
                    }
                }
            }
        }
        
        for(int i = 0; i < listSpeaker.size(); ++i) {
            if(i!=iBestSpeaker)
            {
                listSpeaker[i]->unSelectSpeaker();
            }else{
                listSpeaker[i]->selectSpeaker();
            }
            listSpeaker[i]->repaint();
        }
    }
}

void SpeakerViewComponent::mouseDrag (const MouseEvent& e) {
    
    if(e.mods.isRightButtonDown()){
        camAngleX = (e.getPosition().x + deltaClickX);
        camAngleY = (e.getPosition().y + deltaClickY);
    }
}

void SpeakerViewComponent::mouseWheelMove(const MouseEvent& e,const MouseWheelDetails& wheel){
    distance -= (wheel.deltaY*1.5f);
}

void SpeakerViewComponent::drawBackground()
{
    glMatrixMode(GL_PROJECTION );
    glLoadIdentity();
    glOrtho(0,1,0,1,-1,1);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //draw 2D image
    glBegin(GL_QUADS);
    glColor3f(0.48,0.55,0.63);
    glVertex2f(1.0,1.0);
    glVertex2f(-1.0,1.0);
    
    glColor3f(0.0,0.0,0.0);
    glVertex2f(0.0, 0.0);
    glVertex2f(1.0, 0.0);
    glEnd();
    
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

void SpeakerViewComponent::drawOriginGrid()
{
    
    //Draw circle------------------------
    glLineWidth(1.5f);
    
    glColor3f(0, 0, 0);
    
    /*glBegin(GL_LINE_LOOP);
    for(int i =0; i <= 180; i++){
        double angle = (2 * M_PI * i / 180);
        glVertex3f(cos(angle)*1.0f, 0.0f, sin(angle)*1.0f);
    }
    glEnd();*/
    
    glBegin(GL_LINE_LOOP);
    for(int i =0; i <= 180; i++){
        double angle = (2 * M_PI * i / 180);
        glVertex3f(cos(angle)*5.0f, 0.0f, sin(angle)*5.0f);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    for(int i =0; i <= 180; i++){
        double angle = (2 * M_PI * i / 180);
        glVertex3f(cos(angle)*10.0f, 0.0f, sin(angle)*10.0f);
    }
    glEnd();
    
    
    //3D RGB line----------------------
    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3f(0.4, 0, 0); glVertex3f(-10, 0, 0); glVertex3f(0, 0, 0);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(10, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 10, 0);
    glColor3f(0, 0, 0.4); glVertex3f(0, 0, -10); glVertex3f(0, 0, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 10);
    glEnd();

    
    //Grid-----------------------------
    glLineWidth(1);
    glColor3f(0.49, 0.49, 0.49);
    for(int x = -16; x < 17; x+=2){
        if(x != 0){
            glBegin(GL_LINE_LOOP);
            glVertex3f(x,0,-16);
            glVertex3f(x,0,16);
            glEnd();
        }
    }
    
    for(int z = -16; z < 17; z+=2){
        if(z != 0){
            glBegin(GL_LINE_LOOP);
            glVertex3f(-16,0,z);
            glVertex3f(16,0,z);
            glEnd();
        }
    }
    
    drawText("0",glm::vec3(0,0.1,0));
    drawText("X",glm::vec3(10,0.1,0));
    drawText("Y",glm::vec3(0,0.1,10));
    drawText("Z",glm::vec3(0,10,0));
}

void SpeakerViewComponent::drawText(string val, glm::vec3 position, bool camLock){
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    
    //glRotatef(-90.0f, 1, 1, 0);
    if(camLock){
        glRotatef((camAngleX), 0, 1, 0);
        if(camAngleY < 0 && camAngleY > 90.f){
            glRotatef(-camAngleY, 1, 0, 0);
        }

        
    }

    float s = 0.005f;
    glScalef( s, s, s );
    glColor3f( 1, 1, 1 );
    for (char & c : val)
    {
        glutStrokeCharacter( GLUT_STROKE_MONO_ROMAN, c );
    }
   
    glTranslatef(-1*position.x, -1*position.y, -1*position.z);
    glPopMatrix();
}


void SpeakerViewComponent::drawCube(float x, float y, float z)

{
    const float sizex = 0.5f;
    const float sizey = 0.5f;
    const float sizez = 0.5f;
    
    glTranslatef(x, y, z);
    
    glBegin(GL_QUADS);
    
    glColor3f(0.85, 0.86, 0.87);
    
    // FRONT
    glVertex3f(-sizex, -sizey, sizez);
    glVertex3f(sizex, -sizey, sizez);
    glVertex3f(sizex, sizey, sizez);
    glVertex3f(-sizex, sizey, sizez);
    
    // BACK
    glVertex3f(-sizex, -sizey, -sizez);
    glVertex3f(-sizex, sizey, -sizez);
    glVertex3f(sizex, sizey, -sizez);
    glVertex3f(sizex, -sizey, -sizez);
    
    // LEFT
    glVertex3f(-sizex, -sizey, sizez);
    glVertex3f(-sizex, sizey, sizez);
    glVertex3f(-sizex, sizey, -sizez);
    glVertex3f(-sizex, -sizey, -sizez);
    
    // RIGHT
    glVertex3f(sizex, -sizey, -sizez);
    glVertex3f(sizex, sizey, -sizez);
    glVertex3f(sizex, sizey, sizez);
    glVertex3f(sizex, -sizey, sizez);
    
    // TOP
    glVertex3f(-sizex, sizey, sizez);
    glVertex3f(sizex, sizey, sizez);
    glVertex3f(sizex, sizey, -sizez);
    glVertex3f(-sizex, sizey, -sizez);
    
    // BOTTOM
    glVertex3f(-sizex, -sizey, sizez);
    glVertex3f(-sizex, -sizey, -sizez);
    glVertex3f(sizex, -sizey, -sizez);
    glVertex3f(sizex, -sizey, sizez);
    
    glEnd();
    
    glTranslatef(-x, -y, -z);
}
