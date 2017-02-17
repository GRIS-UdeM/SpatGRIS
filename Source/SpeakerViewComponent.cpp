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



SpeakerObj::SpeakerObj(glm::vec3 pos){
    this->position = pos;
}

SpeakerObj::~SpeakerObj(){
    
}


void SpeakerObj::draw(){
    
    //glTranslatef(this->position.x, this->position.y, this->position.z);
    
    glBegin(GL_QUADS);
    
    glColor3f(this->color.x, this->color.y, this->color.z);

    
    // FRONT
    glVertex3f(this->position.x-sizex, this->position.y-sizey, this->position.z+sizez);
    glVertex3f(this->position.x+sizex, this->position.y-sizey, this->position.z+sizez);
    glVertex3f(this->position.x+sizex, this->position.y+sizey, this->position.z+sizez);
    glVertex3f(this->position.x-sizex, this->position.y+sizey, this->position.z+sizez);
    
    // BACK
    glVertex3f(this->position.x-sizex, this->position.y-sizey, this->position.z-sizez);
    glVertex3f(this->position.x-sizex, this->position.y+sizey, this->position.z-sizez);
    glVertex3f(this->position.x+sizex, this->position.y+sizey, this->position.z-sizez);
    glVertex3f(this->position.x+sizex, this->position.y-sizey, this->position.z-sizez);
    
    
    
    // LEFT
    glVertex3f(this->position.x-sizex, this->position.y-sizey, this->position.z+sizez);
    glVertex3f(this->position.x-sizex, this->position.y+sizey, this->position.z+sizez);
    glVertex3f(this->position.x-sizex, this->position.y+sizey, this->position.z-sizez);
    glVertex3f(this->position.x-sizex, this->position.y-sizey, this->position.z-sizez);
    
    // RIGHT
    glVertex3f(this->position.x+sizex, this->position.y-sizey, this->position.z-sizez);
    glVertex3f(this->position.x+sizex, this->position.y+sizey, this->position.z-sizez);
    glVertex3f(this->position.x+sizex, this->position.y+sizey, this->position.z+sizez);
    glVertex3f(this->position.x+sizex, this->position.y-sizey, this->position.z+sizez);
    
    
    
    // TOP
    glVertex3f(this->position.x-sizex, this->position.y+sizey, this->position.z+sizez);
    glVertex3f(this->position.x+sizex, this->position.y+sizey, this->position.z+sizez);
    glVertex3f(this->position.x+sizex, this->position.y+sizey, this->position.z-sizez);
    glVertex3f(this->position.x-sizex, this->position.y+sizey, this->position.z-sizez);
    
    // BOTTOM
    glVertex3f(this->position.x-sizex, this->position.y-sizey, this->position.z+sizez);
    glVertex3f(this->position.x-sizex, this->position.y-sizey, this->position.z-sizez);
    glVertex3f(this->position.x+sizex, this->position.y-sizey, this->position.z-sizez);
    glVertex3f(this->position.x+sizex, this->position.y-sizey, this->position.z+sizez);
    
    glEnd();
    
    //glTranslatef(-this->position.x, -this->position.y, -this->position.z);
}

void SpeakerObj::selectSpeaker()
{
    this->color = colorSpeakerSelect;
}
void SpeakerObj::unSelectSpeaker()
{
    this->color = colorSpeaker;
}

void SpeakerObj::setPosition(glm::vec3 pos){
    this->position = pos;
    draw();
}

glm::vec3 SpeakerObj::getPosition(){
    return  this->position;
}



//==========================================================================================
// ACTUAL SPEAKER VIEW CLASS DEFS
//==========================================================================================
SpeakerViewComponent::SpeakerViewComponent() {
    setSize(400, 400);
    
    this->listSpeaker = std::vector<SpeakerObj>();
    this->listSpeaker.push_back(SpeakerObj(glm::vec3(4,0,2)));
    //this->listSpeaker.push_back(SpeakerObj(glm::vec3(10,0,0)));
    
    /*this->listSpeaker.push_back(SpeakerObj(glm::vec3(-8,0,-5)));
    this->listSpeaker.push_back(SpeakerObj(glm::vec3(8,0,-5)));
    
    this->listSpeaker.push_back(SpeakerObj(glm::vec3(-8,0,5)));
    this->listSpeaker.push_back(SpeakerObj(glm::vec3(8,0,5)));*/

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


void SpeakerViewComponent::drawBackground()
{
    glMatrixMode(GL_PROJECTION );
    glLoadIdentity();
    glOrtho(0,1,0,1,-1,1);
    //glDisable(GL_DEPTH_TEST);
    //glDisable(GL_LIGHTING);
    //glDepthMask(GL_FALSE);
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
    
    //glDepthMask(GL_TRUE);
    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

}
void SpeakerViewComponent::drawOriginGrid()
{
    glColor3f(0.49, 0.49, 0.49);
    for(int x = -25; x < 25; x++){
        glBegin(GL_LINE_LOOP);
        glVertex3f(x,0,-25);
        glVertex3f(x,0,25);
        glEnd();
    };
    
    
    for(int z = -25; z < 25; z++){
        glBegin(GL_LINE_LOOP);
        glVertex3f(-25,0,z);
        glVertex3f(25,0,z);
        glEnd();
    };
    
    
    
    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3f(0, 0, 0);
    glVertex3f(-25, 0, 0);glVertex3f(25, 0, 0);
    glVertex3f(0, 0, -25); glVertex3f(0, 0, 25);
    glEnd();
    
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(10, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 10, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 10);
    glEnd();

    glLineWidth(1);

}


void SpeakerViewComponent::render() {
    
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    drawBackground();
    
    gluPerspective(90.0, 16.0/9.0, 1, 40);
    glMatrixMode(GL_MODELVIEW);
    
    
    
    float camX = distance * sinf(camAngleX*(M_PI/180)) * cosf((camAngleY)*(M_PI/180));
    float camY = distance * sinf((camAngleY)*(M_PI/180));
    float camZ = distance * cosf((camAngleX)*(M_PI/180)) * cosf((camAngleY)*(M_PI/180));
    
    glLoadIdentity();
    gluLookAt(camX, camY, camZ, 0, 0, 0, 0,1,0);
    
    drawOriginGrid();
    
    
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(r.origin.x, r.origin.y, r.origin.z); glVertex3f(r.dir.x, r.dir.y, r.dir.z);
    glEnd();

    for(int i = 0; i < this->listSpeaker.size(); ++i) {
        this->listSpeaker[i].draw();
    }
    
    
   
    
    glFlush();
  
    
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

    double matModelView[16], matProjection[16];
    int viewport[4];
    glGetDoublev( GL_MODELVIEW_MATRIX, matModelView );
    glGetDoublev( GL_PROJECTION_MATRIX, matProjection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    double winX = (double)e.getPosition().x;
    double winY = viewport[3] - (double)e.getPosition().y;
	
    GLdouble xS, yS, zS;
    GLdouble xE, yE, zE;

    gluUnProject(winX, winY, 0.0, matModelView, matProjection,viewport, &xS,&yS,&zS);
    gluUnProject(winX, winY, 1.0, matModelView, matProjection, viewport, &xE, &yE, &zE);
    
   
    listSpeaker[0].unSelectSpeaker();
    r.orig = glm::vec3(xS, yS, zS);
    r.dir = glm::vec3(xE, yE, zE);
    
    float tmin, tmax, tymin, tymax, tzmin, tzmax;
    
    tmin = (bounds[r.sign[0]].x - r.orig.x) * r.invdir.x;
    tmax = (bounds[1-r.sign[0]].x - r.orig.x) * r.invdir.x;
    tymin = (bounds[r.sign[1]].y - r.orig.y) * r.invdir.y;
    tymax = (bounds[1-r.sign[1]].y - r.orig.y) * r.invdir.y;
    
    if ((tmin > tymax) || (tymin > tmax)){
        cout << "false" << endl;
        return;
    }
    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;
    
    tzmin = (bounds[r.sign[2]].z - r.orig.z) * r.invdir.z;
    tzmax = (bounds[1-r.sign[2]].z - r.orig.z) * r.invdir.z;
    
    if ((tmin > tzmax) || (tzmin > tmax)){
        cout << "false" << endl;
        return;
    }
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;
    
    listSpeaker[0].selectSpeaker();
    cout << "true" << endl;
    
    /*
    float tmin = (listSpeaker[0].getMin().x - r.origin.x) / r.dir.x;
    float tmax = (listSpeaker[0].getMax().x - r.origin.x) / r.dir.x;
    
    if (tmin > tmax) swap(tmin, tmax);
    
    float tymin = (listSpeaker[0].getMin().y - r.origin.y) / r.dir.y;
    float tymax = (listSpeaker[0].getMax().y - r.origin.y) / r.dir.y;
    
    if (tymin > tymax) swap(tymin, tymax);
    
    if ((tmin > tymax) || (tymin > tmax)){
         cout << "false" << endl;
        return;
    }
    
    if (tymin > tmin)
        tmin = tymin;
    
    if (tymax < tmax)
        tmax = tymax;
    
    float tzmin = (listSpeaker[0].getMin().z - r.origin.z) / r.dir.z;
    float tzmax = (listSpeaker[0].getMax().z - r.origin.z) / r.dir.z;
    
    if (tzmin > tzmax) swap(tzmin, tzmax);
    
    if ((tmin > tzmax) || (tzmin > tmax)){
        cout << "false" << endl;
        return;
    }
    
    
    if (tzmin > tmin)
        tmin = tzmin;
    
    if (tzmax < tmax)
        tmax = tzmax;
    listSpeaker[0].selectSpeaker();
    cout << "true" << endl;

/*
    glm::vec3 tMin = (listSpeaker[0].getMin() - r.origin) / r.dir;
    glm::vec3 tMax = (listSpeaker[0].getMax() - r.origin) / r.dir;
    glm::vec3 t1 = min(tMin, tMax);
    glm::vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    
    posC =  glm::vec2(tNear, tFar);
    

    float smallest = 1000.0;
    bool found = false;

    
    //cout << (posC.x > 0.0) << " * "<< (posC.x < posC.y) << " * "<< (posC.x < smallest) << endl;

    if (posC.x > 0.0 && posC.x < posC.y && posC.x < smallest) {
        smallest = posC.x;
        found = true;
        listSpeaker[0].selectSpeaker();
    }
    cout << found << endl;*/

 
}

void SpeakerViewComponent::mouseDrag (const MouseEvent& e) {
    if(e.mods.isRightButtonDown()){
        camAngleX =e.getPosition().x;
        
        camAngleY = e.getPosition().y;
    }
}

void SpeakerViewComponent::mouseWheelMove(const MouseEvent& e,const MouseWheelDetails& wheel){
        distance -= (wheel.deltaY*1.5f);
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
