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

#include <JuceHeader.h>

#if defined(WIN32)
    #include <GL/freeglut.h>
#endif

// For some unknown reason, glutSolidSphere() crashes on Windows (and also maybe on Linux). This might be due to a
// conflict between glm, freeglut and juce's OpenGL implementations.
//
// This fix avoids calling glutSolidSphere().
static auto inline drawSphere = [](float const r) {
    static auto constexpr LATS = 8;
    static auto constexpr LONGS = 8;

    ASSERT_OPEN_GL_THREAD;

    for (int i{}; i <= LATS; ++i) {
        auto const lat0
            = juce::MathConstants<float>::pi * (-0.5f + static_cast<float>(i - 1) / static_cast<float>(LATS));
        auto const z0 = std::sin(lat0);
        auto const zr0 = std::cos(lat0);

        auto const lat1 = juce::MathConstants<float>::pi * (-0.5f + static_cast<float>(i) / static_cast<float>(LATS));
        auto const z1 = std::sin(lat1);
        auto const zr1 = std::cos(lat1);

        glBegin(GL_QUAD_STRIP);
        for (int j{}; j <= LONGS; ++j) {
            auto const lng
                = 2.0f * juce::MathConstants<float>::pi * static_cast<float>(j - 1) / static_cast<float>(LONGS);
            auto const x = std::cos(lng);
            auto const y = std::sin(lng);

            glNormal3f(x * zr0, y * zr0, z0);
            glVertex3f(r * x * zr0, r * y * zr0, r * z0);
            glNormal3f(x * zr1, y * zr1, z1);
            glVertex3f(r * x * zr1, r * y * zr1, r * z1);
        }
        glEnd();
    }
};