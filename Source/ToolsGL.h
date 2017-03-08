//
//  ToolsGL.h
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


#include "Speaker.h"
using namespace std;



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
        glVertex3f(this->position.x, this->position.y, this->position.z);
        glVertex3f(this->direction.x, this->direction.y, this->direction.z);
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
    static float Raycast(Ray ray, Speaker &speaker);
    static void printMatrix(glm::vec3 m);
    static bool speakerNearCam(glm::vec3 speak1, glm::vec3 speak2, glm::vec3 cam);
  
   
};

#endif /* ToolsGL_h */
