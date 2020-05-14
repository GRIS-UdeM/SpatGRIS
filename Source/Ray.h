/*
 This file is part of SpatGRIS2.
 
 Developers: Nicolas Masson
 
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

#ifndef RAY_H
#define RAY_H

#if defined(__linux__)
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#elif defined(WIN32) || defined(_WIN64)
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>
#endif

#include "../glm/glm.hpp"

//==============================================================================
class Ray {
public :
    Ray() {
        this->position = glm::vec3(0,0,0);
        this->direction = glm::vec3(0,0,0);
        this->normal = glm::vec3(0,0,0);
    }
    //==============================================================================
    void setRay(glm::vec3 p, glm::vec3 d) {
        this->position = glm::vec3(p);
        this->direction = glm::vec3(d);
        this->normal = (this->direction - this->position) / 5000.0f;
    }
    //==============================================================================
    glm::vec3 getNormal() {
        return this->normal;
    }
    //==============================================================================
    glm::vec3 getPosition() {
        return this->position;
    }
    //==============================================================================
    glm::vec3 getDirection() {
        return this->direction;
    }
    //==============================================================================
    void draw() {
        glBegin(GL_LINES);
        glColor3f(1, 0, 0);
        glVertex3f(this->position.x, this->position.y, this->position.z);
        glVertex3f(this->direction.x, this->direction.y, this->direction.z);
        glEnd();
    }
private:
    //==============================================================================
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 normal;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Ray);
};

#endif /* RAY_H */
