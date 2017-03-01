//
//  ToolsGL.hpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-01.
//
//

#ifndef ToolsGL_h
#define ToolsGL_h

#include <iostream>
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>

#include "../glm/glm.hpp"

using namespace std;

const glm::vec3 colorSpeaker = glm::vec3(0.85, 0.86, 0.87);
const glm::vec3 colorSpeakerSelect = glm::vec3(1.0, 0.66, 0.67);
const glm::vec3 sizeSpeaker = glm::vec3(0.5, 0.5, 0.5);

class SpeakerObj {
public:
    
    SpeakerObj() {
        min = glm::vec3(-1, -1, -1);
        max = glm::vec3(1, 1, 1);
    }
    
    
    SpeakerObj(glm::vec3 center, glm::vec3 extents = sizeSpeaker) {
        //min == center - extents, max == c+e
        min.x = center.x - extents.x;
        min.y = center.y - extents.y;
        min.z = center.z - extents.z;
        
        max.x = center.x + extents.x;
        max.y = center.y + extents.y;
        max.z = center.z + extents.z;
        if (!isValid()) {
            fix();
        }
    }

    glm::vec3 getMin() {
        return this->min;
    }

    glm::vec3 getMax() {
        return this->max;
    }

    glm::vec3 center() {
        return  glm::vec3(min.x+(max.x - min.x) / 2.0f, min.y+(max.y - min.y) / 2.0f, min.z+(max.z - min.z) / 2.0f);
    }
    
    glm::vec3 extents() {
        return  glm::vec3((max.x - center().x), (max.y - center().y), (max.z - center().z));
    }
    
    bool isValid() {
        return (min.x < max.x && min.y < max.y && min.z < max.z);
    }
    
    void fix() {
        glm::vec3 _max = (max);
        //change new "min" to previous max
        if (min.x > max.x) {
            max.x = min.x;
            min.x = _max.x;
        }
        if (min.y > max.y) {
            max.y = min.y;
            min.y = _max.y;
        }
        if (min.z > max.z) {
            max.z = min.z;
            min.z = _max.z;
        }
    }
    
    bool isSelected(){
        return this->selected;
    }
    void selectSpeaker()
    {
        this->color = colorSpeakerSelect;
        this->selected = true;
    }
    void unSelectSpeaker()
    {
        this->color = colorSpeaker;
        this->selected = false;
    }
    
    void draw() {
        glBegin(GL_QUADS);
        
        glColor3f(this->color.x, this->color.y, this->color.z);
        glVertex3f(min.x, min.y, max.z);
        glVertex3f(max.x, min.y, max.z);
        glVertex3f(max.x, max.y, max.z);
        glVertex3f(min.x, max.y, max.z);
        
        glVertex3f(max.x, min.y, max.z);
        glVertex3f(max.x, min.y, min.z);
        glVertex3f(max.x, max.y, min.z);
        glVertex3f(max.x, max.y, max.z);
        
        glVertex3f(min.x, max.y, max.z);
        glVertex3f(max.x, max.y, max.z);
        glVertex3f(max.x, max.y, min.z);
        glVertex3f(min.x, max.y, min.z);
        
        glVertex3f(min.x, min.y, min.z);
        glVertex3f(min.x, max.y, min.z);
        glVertex3f(max.x, max.y, min.z);
        glVertex3f(max.x, min.y, min.z);
        
        glVertex3f(min.x, min.y, min.z);
        glVertex3f(max.x, min.y, min.z);
        glVertex3f(max.x, min.y, max.z);
        glVertex3f(min.x, min.y, max.z);
        
        glVertex3f(min.x, min.y, min.z);
        glVertex3f(min.x, min.y, max.z);
        glVertex3f(min.x, max.y, max.z);
        glVertex3f(min.x, max.y, min.z);
        
        glEnd();
    }
    
private :
    glm::vec3 min = glm::vec3(0,0,0);
    glm::vec3 max = glm::vec3(0,0,0);
    glm::vec3 color = colorSpeaker;
    
    bool selected = false;
    
};

class Ray {
public :
    Ray() {
        this->position = glm::vec3(0,0,0);
        this->direction = glm::vec3(0,0,0);
        this->normal = glm::vec3(0,0,0);
    }
    
    Ray(glm::vec3 p, glm::vec3 d) {
        this->position = p;//glm::vec3(p.x, p.y, p.z);
        this->direction = d;
        this->normal = (this->direction - this->position)/5000.0f;//normalize(glm::vec3(d.x, d.y, d.z));
    }
    
    glm::vec3 getNormal() {
        return this->normal;
    }
    glm::vec3 getPosition() {
        return this->position;
    }
    
    glm::vec3 getDirection() {
        return this->direction;
    }
    
    void draw() {
        glBegin(GL_LINES);
        glColor3f(1, 0, 0);
        //glm::vec3 end = position + normal * 5000.0f;
        glVertex3f(position.x, position.y, position.z);
        glVertex3f(direction.x, direction.y, direction.z);
        glEnd();
    }
    
    
private:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 normal;
    
};

class ToolsGL
{
public:
    static float Raycast(Ray ray, SpeakerObj speaker);
    static void printMatrix(glm::vec3 m);
};

#endif /* ToolsGL_h */
