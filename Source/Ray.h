/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#if defined(__linux__)
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glut.h>
#elif defined(__APPLE__)
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGl/glu.h>
#endif

#include "../glm/glm.hpp"

#include <JuceHeader.h>
ENABLE_WARNINGS

//==============================================================================
class Ray
{
    glm::vec3 mPosition{ 0.0f, 0.0f, 0.0f };
    glm::vec3 mDirection{ 0.0f, 0.0f, 0.0f };
    glm::vec3 mNormal{ 0.0f, 0.0f, 0.0f };

public:
    //==============================================================================
    void setRay(glm::vec3 const & p, glm::vec3 const & d);
    //==============================================================================
    glm::vec3 const & getNormal() const { return this->mNormal; }
    glm::vec3 const & getPosition() const { return this->mPosition; }
    glm::vec3 const & getDirection() const { return this->mDirection; }
    //==============================================================================
    void draw() const;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Ray)
};
