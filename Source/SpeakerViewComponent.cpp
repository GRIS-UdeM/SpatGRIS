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
    setSize(400, 400);
}

SpeakerViewComponent::~SpeakerViewComponent() {
    shutdownOpenGL();
}

void SpeakerViewComponent::initialise() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glColor3f(1.0, 1.0, 1.0);
    
    // Set the camera lens to have a 60 degree (vertical) field of view, an
    // aspect ratio of 4/3, and have everything closer than 1 unit to the
    // camera and greater than 40 units distant clipped away.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluPerspective(90.0, 16.0/9.0, 1, 40);
    
    // Position camera at (4, 6, 5) looking at (0, 0, 0) with the vector
    // <0, 1, 0> pointing upward.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    gluLookAt(4, 6, 5, 0, 0, 0, 0, 1, 0);
}

void SpeakerViewComponent::shutdown() {
}





void SpeakerViewComponent::render() {
    
    /*jassert (OpenGLHelpers::isContextActive());
    
    const float desktopScale = (float) openGLContext.getRenderingScale();
    OpenGLHelpers::clear (Colour::greyLevel (0.1f));
    
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glViewport (0, 0, roundToInt (desktopScale * getWidth()), roundToInt (desktopScale * getHeight()));
    
    DrawGrid(2);

    //call it like this
    
    mShader->use();
    DrawGrid(4);
    
    if (mUniforms->projectionMatrix != nullptr){
        mUniforms->projectionMatrix->setMatrix4 (getProjectionMatrix().mat, 1, false);
    }
    
    if (mUniforms->viewMatrix != nullptr){
        mUniforms->viewMatrix->setMatrix4 (getViewMatrix().mat, 1, false);
    }
    DrawGrid(6);
    mShape->draw (openGLContext, *mAttributes);
   DrawGrid(8);
    // Reset the element buffers so child Components draw correctly
    openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, 0);
    openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);*/
    
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Draw a white torus of outer radius 3, inner radius 0.5 with 15 stacks
    // and 30 slices.
    glColor3f(1.0, 1.0, 1.0);
    //glutWireTorus(0.5, 3, 15, 30);
    
    // Draw a red x-axis, a green y-axis, and a blue z-axis.  Each of the
    // axes are ten units long.
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(10, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 10, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 10);
    glEnd();
    
    glColor3f(1, 0, 0);
    for(int x = -25; x < 25; x++){
        glBegin(GL_LINE_LOOP);
        glVertex3f(x,0,-25);
        glVertex3f(x,0,25);
        glEnd();
    };
    
    glColor3f(0, 0, 1);
    for(int z = -25; z < 25; z++){
        glBegin(GL_LINE_LOOP);
        glVertex3f(-25,0,z);
        glVertex3f(25,0,z);
        glEnd();
    };
    
    glFlush();
    
    
/*
    OpenGLHelpers::clear(Colours::black);
    
    //  Clear screen and Z-buffer
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    // Reset transformations
    glLoadIdentity();
    
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(10, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 10, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 10);
    glEnd();
    
    // Other Transformations
    // glTranslatef( 0.1, 0.0, 0.0 );      // Not included
    // glRotatef( 180, 0.0, 1.0, 0.0 );    // Not included
    
    // Rotate when user changes rotate_x and rotate_y
    glRotatef( rotate_x, 1.0, 0.0, 0.0 );
    glRotatef( rotate_y, 0.0, 1.0, 0.0 );
    
    // Other Transformations
    // glScalef( 2.0, 2.0, 0.0 );          // Not included
    
    //Multi-colored side - FRONT
    glBegin(GL_POLYGON);
    
    glColor3f( 1.0, 0.0, 0.0 );     glVertex3f(  0.5, -0.5, -0.5 );      // P1 is red
    glColor3f( 0.0, 1.0, 0.0 );     glVertex3f(  0.5,  0.5, -0.5 );      // P2 is green
    glColor3f( 0.0, 0.0, 1.0 );     glVertex3f( -0.5,  0.5, -0.5 );      // P3 is blue
    glColor3f( 1.0, 0.0, 1.0 );     glVertex3f( -0.5, -0.5, -0.5 );      // P4 is purple
    
    glEnd();
    
    // White side - BACK
    glBegin(GL_POLYGON);
    glColor3f(   1.0,  1.0, 1.0 );
    glVertex3f(  0.5, -0.5, 0.5 );
    glVertex3f(  0.5,  0.5, 0.5 );
    glVertex3f( -0.5,  0.5, 0.5 );
    glVertex3f( -0.5, -0.5, 0.5 );
    glEnd();
    
    // Purple side - RIGHT
    glBegin(GL_POLYGON);
    glColor3f(  1.0,  0.0,  1.0 );
    glVertex3f( 0.5, -0.5, -0.5 );
    glVertex3f( 0.5,  0.5, -0.5 );
    glVertex3f( 0.5,  0.5,  0.5 );
    glVertex3f( 0.5, -0.5,  0.5 );
    glEnd();
    
    // Green side - LEFT
    glBegin(GL_POLYGON);
    glColor3f(   0.0,  1.0,  0.0 );
    glVertex3f( -0.5, -0.5,  0.5 );
    glVertex3f( -0.5,  0.5,  0.5 );
    glVertex3f( -0.5,  0.5, -0.5 );
    glVertex3f( -0.5, -0.5, -0.5 );
    glEnd();
    
    // Blue side - TOP
    glBegin(GL_POLYGON);
    glColor3f(   0.0,  0.0,  1.0 );
    glVertex3f(  0.5,  0.5,  0.5 );
    glVertex3f(  0.5,  0.5, -0.5 );
    glVertex3f( -0.5,  0.5, -0.5 );
    glVertex3f( -0.5,  0.5,  0.5 );
    glEnd();
    
    // Red side - BOTTOM
    glBegin(GL_POLYGON);
    glColor3f(   1.0,  0.0,  0.0 );
    glVertex3f(  0.5, -0.5, -0.5 );
    glVertex3f(  0.5, -0.5,  0.5 );
    glVertex3f( -0.5, -0.5,  0.5 );
    glVertex3f( -0.5, -0.5, -0.5 );
    glEnd();
    
    glFlush();

  */
    //glutSwapBuffers();
   

   /* for(int x = 0; x < 2; x++){
        glBegin(GL_LINE_LOOP);
        glVertex3f(x,0,0);
        glVertex3f(x,10,0);
        glEnd();
    };
    
    for(int y = 0; y < 10; y++){
        glBegin(GL_LINE_LOOP);
        glVertex3f(y,0,0);
        glVertex3f(y,2,0);
        glEnd();
    };
    */
    
    /*
    glEnable(GL_LINES);
    
    glColor3f(1, 0, 0);
    glVertex3f(-1000, 0, 0);
    glVertex3f(1000, 0, 0);
    glEnd();
    
    
    glEnable(GL_LINES);
    glColor3f(0, 1, 0);
    glVertex3f(0, -1000, 0);
    glVertex3f(0, 1000, 0);
    glEnd();
    
    glEnable(GL_LINES);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, -1000);
    glVertex3f(0, 0, 1000);
    
    
    glEnd();
    
    /*
    glBegin(GL_LINES);
    glColor3f(1, 0, 0);
    for(int i=-4;i<=4;i++)
    {
        glVertex3f((float)i,(float)-4,0);
        glVertex3f((float)i,(float)4,0);
        
        glVertex3f((float)-4,(float)i,0);
        glVertex3f((float)4,(float)i,0);
    }
    glEnd();*/
    
}

void SpeakerViewComponent::paint (Graphics& g) {
//    // You can add your component specific drawing code here!
//    // This will draw over the top of the openGL background.
//    
//    g.setColour(Colours::white);
//    g.setFont (20);
//    g.drawText ("OpenGL Example", 25, 20, 300, 30, Justification::left);
//    g.drawLine (20, 20, 170, 20);
//    g.drawLine (20, 50, 170, 50);
}

void SpeakerViewComponent::resized() {
    //draggableOrientation.setViewport (getLocalBounds());
}



void SpeakerViewComponent::mouseDown (const MouseEvent& e) {
    rotate_x += 0.005;
    rotate_y += 5;
    

    
}

void SpeakerViewComponent::mouseDrag (const MouseEvent& e) {
    
    
    float distance = 15.0f;      // Straight line distance between the camera and look at point
    float camAngleX =e.getPosition().x;
    cout << camAngleX << endl;
    float camAngleY = e.getPosition().y;
    // Calculate the camera position using the distance and angles
    float camX = distance * sinf(camAngleX*(M_PI/180)) * cosf((camAngleY)*(M_PI/180));
    float camY = distance * sinf((camAngleY)*(M_PI/180));
    float camZ = distance * cosf((camAngleX)*(M_PI/180)) * cosf((camAngleY)*(M_PI/180));
    
    glLoadIdentity();
    gluLookAt(camX, camY, camZ, 0, 0, 0, 0,1,0);
    
  /*  rotate_x = e.getPosition().y;
    glRotatef(e.getPosition().y, 0.0f, 1.0, 0.0f);
    glRotatef(e.getPosition().x, 1.0f, 0.0f, 0.0f);

    rotate_y = e.getPosition().x;*/
    
    
    
   //myDragger.dragComponent (this, e, nullptr);
}


