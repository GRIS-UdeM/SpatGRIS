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



#ifndef RAY_h
#define RAY_h


#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>

#include "../glm/glm.hpp"



class Ray {
public :
    Ray() {
        this->position = glm::vec3(0,0,0);
        this->direction = glm::vec3(0,0,0);
        this->normal = glm::vec3(0,0,0);
    }
    
    void setRay(glm::vec3 p, glm::vec3 d) {
        this->position = glm::vec3(p);
        this->direction = glm::vec3(d);
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


#endif /* ToolsGL_h */
