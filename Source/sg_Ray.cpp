/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_Ray.hpp"

namespace gris
{
//==============================================================================
void Ray::setRay(CartesianVector const & p, CartesianVector const & d)
{
    this->mPosition = p;
    this->mDirection = d;
    this->mNormal = (this->mDirection - this->mPosition) / 5000.0f;
}

//==============================================================================
void Ray::draw() const
{
    juce::gl::glBegin(juce::gl::GL_LINES);
    juce::gl::glColor3f(1.0f, 0.0f, 0.0f);
    juce::gl::glVertex3f(this->mPosition.x, this->mPosition.y, this->mPosition.z);
    juce::gl::glVertex3f(this->mDirection.x, this->mDirection.y, this->mDirection.z);
    juce::gl::glEnd();
}

} // namespace gris
