//
//  Speaker.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-02.
//
//

#include "Speaker.h"
#include "MainComponent.h"
#include "LevelComponent.h"

Speaker::Speaker(MainContentComponent *parent, int idS){
    Speaker(parent, idS, idS, glm::vec3(0,0,0));
}

Speaker::Speaker(MainContentComponent *parent, int idS,int outP, glm::vec3 center, glm::vec3 extents) {
    this->mainParent = parent;
    this->idSpeaker = idS;
    this->outputPatch = outP;
    LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);
    
    //Load position
    this->newPosition(center, extents);
    this->vuMeter = new LevelComponent(this, &mGrisFeel);
}

Speaker::~Speaker(){
    delete this->vuMeter;
}

float Speaker::getLevel(){
    return this->mainParent->getLevelsOut(outputPatch-1);
}
void Speaker::setMuted(bool mute){
    this->mainParent->muteOutput(this->outputPatch, mute);
}
void Speaker::setSolo(bool solo){
    this->mainParent->soloOutput(this->outputPatch, solo);
}

int Speaker::getIdSpeaker(){
    return this->idSpeaker;
}
glm::vec3 Speaker::getCoordinate(){
    return this->center /10.0f ;
}
glm::vec3 Speaker::getAziZenRad(){
    return glm::vec3(this->aziZenRad.x, this->aziZenRad.y, this->aziZenRad.z/10.0f);
}

void Speaker::setCoordinate(glm::vec3 value){
    this->newPosition(value*10.0f);
}
void Speaker::setAziZenRad(glm::vec3 value){
    value.z = value.z*10.0f;
    this->newSpheriqueCoord(value);
}

int Speaker::getOutputPatch(){
    return this->outputPatch;
}
void Speaker::setOutputPatch(int value){
    this->outputPatch = value;
    this->vuMeter->setOutputLab(String(this->outputPatch));
}

glm::vec3 Speaker::getMin() {
    return this->min;
}

glm::vec3 Speaker::getMax() {
    return this->max;
}

glm::vec3 Speaker::getCenter(){
    return this->center;
}

bool Speaker::isValid() {
    return (this->min.x < this->max.x && this->min.y < this->max.y && this->min.z < this->max.z);
}

void Speaker::fix() {
    glm::vec3 _max = (this->max);
    //change new "min" to previous max
    if (this->min.x > this->max.x) {
        this->max.x = this->min.x;
        this->min.x = _max.x;
    }
    if (this->min.y > this->max.y) {
        this->max.y = this->min.y;
        this->min.y = _max.y;
    }
    if (this->min.z > this->max.z) {
        this->max.z = this->min.z;
        this->min.z = _max.z;
    }
}

bool Speaker::isSelected(){
    return this->selected;
}

void Speaker::selectSpeaker()
{
    this->color = colorSpeakerSelect;
    this->selected = true;
    this->vuMeter->setSelected(this->selected);
}

void Speaker::unSelectSpeaker()
{
    this->color = colorSpeaker;
    this->selected = false;
    this->vuMeter->setSelected(this->selected);
}


void Speaker::newPosition(glm::vec3 center, glm::vec3 extents)
{
    //min == center - extents, max == c+e
    this->min.x = center.x - extents.x;
    this->min.y = center.y - extents.y;
    this->min.z = center.z - extents.z;
    
    this->max.x = center.x + extents.x;
    this->max.y = center.y + extents.y;
    this->max.z = center.z + extents.z;
    if (!this->isValid()) {
        this->fix();
    }
    this->center = glm::vec3(this->min.x+(this->max.x - this->min.x) / 2.0f, this->min.y+(this->max.y - this->min.y) / 2.0f, this->min.z+(this->max.z - this->min.z) / 2.0f);
    
    float azimuth = ( (atan2(this->center.x, this->center.z) * 180.0f) / M_PI) +90.0f;
    float zenith = ( atan2(this->center.y, sqrt(this->center.x*this->center.x + this->center.z*this->center.z)) * 180.0f) / M_PI;
    float radius = sqrt((this->center.x*this->center.x)+(this->center.y*this->center.y)+(this->center.z*this->center.z));
    azimuth = 180.0f-azimuth;
    if(azimuth < 0.0f){
        azimuth+=360.0f;
    }
    if(zenith < 0.0f){
        zenith+=360.0f;
    }
    if(zenith >= 360.0f){
        zenith=0.0f;
    }
    this->aziZenRad = glm::vec3(azimuth, zenith, radius);
}

void Speaker::newSpheriqueCoord(glm::vec3 aziZenRad, glm::vec3 extents){
    glm::vec3 nCenter;

    aziZenRad.x = abs( (aziZenRad.x * M_PI ) / 180.0f) ;
    aziZenRad.y = abs( ((-90.0f+aziZenRad.y) * M_PI ) / 180.0f);
    
    nCenter.x = GetFloatPrecision(aziZenRad.z * sinf(aziZenRad.y)*cosf(aziZenRad.x), 6);
    nCenter.z = GetFloatPrecision(aziZenRad.z * sinf(aziZenRad.y)*sinf(aziZenRad.x), 6);
    nCenter.y = GetFloatPrecision(aziZenRad.z * cosf(aziZenRad.y), 6);
    
    this->newPosition(nCenter);
}

void Speaker::draw() {
    
    glPushMatrix();

    glTranslatef(this->center.x, this->center.y, this->center.z);
    
    glRotatef(180.0f-this->aziZenRad.x, 0, 1.0, 0);
    glRotatef(-this->aziZenRad.y, 0, 0, 1.0);
    //glRotatef(0, 1.0, 0, 0); Z useless
    glTranslatef(-1*this->center.x, -1*this->center.y, -1*this->center.z);
    
    glBegin(GL_QUADS);
    
    glColor3f(this->color.x, this->color.y, this->color.z);
    
    glVertex3f(this->min.x, this->min.y, this->max.z);
    glVertex3f(this->max.x, this->min.y, this->max.z);
    glVertex3f(this->max.x, this->max.y, this->max.z);
    glVertex3f(this->min.x, this->max.y, this->max.z);
    
    glVertex3f(this->max.x, this->min.y, this->max.z);
    glVertex3f(this->max.x, this->min.y, this->min.z);
    glVertex3f(this->max.x, this->max.y, this->min.z);
    glVertex3f(this->max.x, this->max.y, this->max.z);
    
    glVertex3f(this->min.x, this->max.y, this->max.z);
    glVertex3f(this->max.x, this->max.y, this->max.z);
    glVertex3f(this->max.x, this->max.y, this->min.z);
    glVertex3f(this->min.x, this->max.y, this->min.z);
    
    glVertex3f(this->min.x, this->min.y, this->min.z);
    glVertex3f(this->min.x, this->max.y, this->min.z);
    glVertex3f(this->max.x, this->max.y, this->min.z);
    glVertex3f(this->max.x, this->min.y, this->min.z);
    
    glVertex3f(this->min.x, this->min.y, this->min.z);
    glVertex3f(this->max.x, this->min.y, this->min.z);
    glVertex3f(this->max.x, this->min.y, this->max.z);
    glVertex3f(this->min.x, this->min.y, this->max.z);
    
    glVertex3f(this->min.x, this->min.y, this->min.z);
    glVertex3f(this->min.x, this->min.y, this->max.z);
    glVertex3f(this->min.x, this->max.y, this->max.z);
    glVertex3f(this->min.x, this->max.y, this->min.z);
    
    glEnd();
    
    glLineWidth(2);
    glBegin(GL_LINES);
    
    glColor3f(0, 0, 0);
    glVertex3f(this->center.x+sizeSpeaker.x,this->center.y, this->center.z);
    glVertex3f(this->center.x+ 1.2f,this->center.y, this->center.z);
    glEnd();
    
    if(this->selected){
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // this tells it to only render lines
        
        glLineWidth(4);
        glBegin(GL_LINES);
        float over = 0.02f;
        
        glVertex3f(this->min.x-over, this->min.y-over, this->min.z-over);
        glVertex3f(this->min.x-over, this->min.y-over, this->max.z+over);
        
        glVertex3f(this->max.x+over, this->min.y-over, this->min.z-over);
        glVertex3f(this->max.x+over, this->min.y-over, this->max.z+over);
        
        glVertex3f(this->max.x+over, this->max.y+over, this->min.z-over);
        glVertex3f(this->max.x+over, this->max.y+over, this->max.z+over);
        
        glVertex3f(this->min.x-over, this->max.y+over, this->min.z-over);
        glVertex3f(this->min.x-over, this->max.y+over, this->max.z+over);
        

        glVertex3f(this->min.x-over, this->min.y-over, this->min.z-over);
        glVertex3f(this->max.x+over, this->min.y-over, this->min.z-over);
        
        glVertex3f(this->min.x-over, this->min.y-over, this->max.z+over);
        glVertex3f(this->max.x+over, this->min.y-over, this->max.z+over);
        
        glVertex3f(this->min.x-over, this->max.y+over, this->min.z-over);
        glVertex3f(this->max.x+over, this->max.y+over, this->min.z-over);
        
        glVertex3f(this->min.x-over, this->max.y+over, this->max.z+over);
        glVertex3f(this->max.x+over, this->max.y+over, this->max.z+over);
        
        
        glVertex3f(this->min.x-over, this->min.y-over, this->min.z-over);
        glVertex3f(this->min.x-over, this->max.y+over, this->min.z-over);
        
        glVertex3f(this->min.x-over, this->min.y-over, this->max.z+over);
        glVertex3f(this->min.x-over, this->max.y+over, this->max.z+over);
        
        glVertex3f(this->max.x+over, this->min.y-over, this->min.z-over);
        glVertex3f(this->max.x+over, this->max.y+over, this->min.z-over);
        
        glVertex3f(this->max.x+over, this->min.y-over, this->max.z+over);
        glVertex3f(this->max.x+over, this->max.y+over, this->max.z+over);

        
        glEnd();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glPopMatrix();
}
