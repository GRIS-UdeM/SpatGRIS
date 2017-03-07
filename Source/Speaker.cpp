//
//  Speaker.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-02.
//
//

#include "Speaker.h"

Speaker::Speaker(){

}

Speaker::Speaker(glm::vec3 center, glm::vec3 extents) {
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
    
    yAngle = ( (atan2(this->center.x, this->center.z) * 180.0f) / M_PI) +90.0f;
    zAngle = ( -atan2(this->center.y, sqrt(this->center.x*this->center.x + this->center.z*this->center.z)) * 180.0f) / M_PI;
}

Speaker::~Speaker(){
    
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
}

void Speaker::unSelectSpeaker()
{
    this->color = colorSpeaker;
    this->selected = false;
}

void Speaker::draw() {
    
    glPushMatrix();

    glTranslatef(this->center.x, this->center.y, this->center.z);
    glRotatef(yAngle, 0, 1.0, 0);
    glRotatef(zAngle, 0, 0, 1.0);
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

    
    glPopMatrix();
}
