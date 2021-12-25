/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "sg_CartesianVector.hpp"

#include "sg_Warnings.hpp"

#include <JuceHeader.h>

DISABLE_WARNING_PUSH
DISABLE_WARNING_NAMELESS_STRUCT
DISABLE_WARNING_UNREFERENCED_FUNCTION
#if defined(__linux__)
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glut.h>
#elif defined(__APPLE__)
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGl/glu.h>
#endif

DISABLE_WARNING_POP

namespace gris
{
//==============================================================================
class Ray
{
    CartesianVector mPosition{ 0.0f, 0.0f, 0.0f };
    CartesianVector mDirection{ 0.0f, 0.0f, 0.0f };
    CartesianVector mNormal{ 0.0f, 0.0f, 0.0f };

public:
    //==============================================================================
    void setRay(CartesianVector const & p, CartesianVector const & d);
    //==============================================================================
    [[nodiscard]] CartesianVector const & getNormal() const { return this->mNormal; }
    [[nodiscard]] CartesianVector const & getPosition() const { return this->mPosition; }
    [[nodiscard]] CartesianVector const & getDirection() const { return this->mDirection; }
    //==============================================================================
    void draw() const;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Ray)
};

} // namespace gris