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

#include "PolarVector.h"
#include "macros.h"

enum class PortState;
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

//#include "AudioProcessor.h"
#include "SpatMode.hpp"
#include "VuMeterComponent.h"
#include "VuMeterModel.h"

class GrisLookAndFeel;
class MainContentComponent;

//==============================================================================
class InputModel final : public VuMeterModel
{
    static constexpr auto SPHERE_RADIUS = 0.3f;
    static constexpr auto HALF_SPHERE_RADIUS = SPHERE_RADIUS / 2.0f;

    MainContentComponent & mMainContentComponent;
    SmallGrisLookAndFeel & mLookAndFeel;

    source_index_t mIndex;
    tl::optional<output_patch_t> mDirectOut{};

    PolarVector mVector{};
    float mAzimuthSpan{};
    float mZenithSpan{};

    glm::vec3 mCenter{};
    glm::vec3 mColor{};
    juce::Colour mColorJ{};

    VuMeterComponent mVuMeter;

public:
    //==============================================================================
    InputModel(MainContentComponent & mainContentComponent, SmallGrisLookAndFeel & lookAndFeel, source_index_t index);
    //==============================================================================
    InputModel() = delete;
    ~InputModel() override = default;

    InputModel(InputModel const &) = delete;
    InputModel(InputModel &&) = delete;

    InputModel & operator=(InputModel const &) = delete;
    InputModel & operator=(InputModel &&) = delete;
    //==============================================================================
    void setState(PortState state) override;
    void setSelected(bool state) override{};
    void setColor(juce::Colour color, bool updateLevel = false) override;
    //==============================================================================
    [[nodiscard]] MainContentComponent const & getMainContentComponent() const { return mMainContentComponent; }
    [[nodiscard]] MainContentComponent & getMainContentComponent() { return mMainContentComponent; }

    [[nodiscard]] VuMeterComponent const * getVuMeter() const override { return &mVuMeter; }
    [[nodiscard]] VuMeterComponent * getVuMeter() override { return &mVuMeter; }

    [[nodiscard]] source_index_t getIndex() const { return mIndex; }
    [[nodiscard]] dbfs_t getLevel() const override;
    [[nodiscard]] float getAlpha() const;
    [[nodiscard]] auto const & getVector() const { return mVector; }
    [[nodiscard]] float getAzimuthSpan() const { return mAzimuthSpan; }
    [[nodiscard]] float getZenithSpan() const { return mZenithSpan; }

    [[nodiscard]] glm::vec3 getCenter() const { return mCenter; }
    [[nodiscard]] glm::vec3 getColor() const { return mColor; }
    [[nodiscard]] glm::vec3 getNumberColor() const;
    [[nodiscard]] juce::Colour getColorJ() const { return mColorJ; }
    [[nodiscard]] juce::Colour getColorJWithAlpha() const;
    //==============================================================================
    [[nodiscard]] glm::vec3 polToCar(radians_t azimuth, radians_t zenith) const;
    [[nodiscard]] glm::vec3 polToCar3d(radians_t azimuth, radians_t zenith) const;
    //==============================================================================
    void resetPosition();
    void draw() const;
    void updateValues(PolarVector const & vector, float azimuthSpan, float zenithSpan, dbfs_t gain, SpatMode mode);
    void updateValuesOld(float azimuth, float zenith, float azimuthSpan, float zenithSpan, float g);
    //==============================================================================
    [[nodiscard]] bool isInput() const override { return true; }
    void setDirectOut(tl::optional<output_patch_t> directOut);
    [[nodiscard]] tl::optional<output_patch_t> getDirectOut() const { return mDirectOut; };
    // void sendDirectOutToClient(int id, output_patch_t chn) override; // TODO: what is this?

    void drawSpan() const;
    void drawSpanLbap(float x, float y, float z) const;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(InputModel)
};
