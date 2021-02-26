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

#include "AudioProcessor.h"
#include "LevelComponent.h"
#include "ParentLevelComponent.h"

class GrisLookAndFeel;
class MainContentComponent;

//==============================================================================
class Input final : public ParentLevelComponent
{
    static constexpr auto SPHERE_RADIUS = 0.3f;
    static constexpr auto HALF_SPHERE_RADIUS = SPHERE_RADIUS / 2.0f;

    MainContentComponent & mMainContentComponent;
    SmallGrisLookAndFeel & mLookAndFeel;

    int mIdChannel;
    output_patch_t mDirectOutChannel{};

    float mAzimuth{};
    float mZenith{};
    float mRadius{};

    float mAzimuthSpan{};
    float mZenithSpan{};
    float mGain{ -1.0f };

    glm::vec3 mCenter{};
    glm::vec3 mColor{};
    juce::Colour mColorJ{};

    LevelComponent mVuMeter;

public:
    //==============================================================================
    Input(MainContentComponent & mainContentComponent, SmallGrisLookAndFeel & lookAndFeel, int id = 0);
    //==============================================================================
    Input() = delete;
    ~Input() override = default;

    Input(Input const &) = delete;
    Input(Input &&) = delete;

    Input & operator=(Input const &) = delete;
    Input & operator=(Input &&) = delete;
    //==============================================================================
    void setMuted(bool mute) override;
    void setSolo(bool solo) override;
    void selectClick(bool /*select = true*/) override{};
    void setColor(juce::Colour color, bool updateLevel = false) override;
    //==============================================================================
    [[nodiscard]] MainContentComponent const & getMainContentComponent() const { return mMainContentComponent; }
    [[nodiscard]] MainContentComponent & getMainContentComponent() { return mMainContentComponent; }

    [[nodiscard]] LevelComponent const * getVuMeter() const override { return &mVuMeter; }
    [[nodiscard]] LevelComponent * getVuMeter() override { return &mVuMeter; }

    [[nodiscard]] int getId() const override { return mIdChannel; }
    [[nodiscard]] int getButtonInOutNumber() const override { return mIdChannel; }
    [[nodiscard]] float getLevel() const override;
    [[nodiscard]] float getAlpha() const;
    [[nodiscard]] float getAzimuth() const { return mAzimuth; }
    [[nodiscard]] float getZenith() const { return mZenith; }
    [[nodiscard]] float getRadius() const { return mRadius; }
    [[nodiscard]] float getAzimuthSpan() const { return mAzimuthSpan; }
    [[nodiscard]] float getZenithSpan() const { return mZenithSpan; }
    [[nodiscard]] float getGain() const { return mGain; }

    [[nodiscard]] glm::vec3 getCenter() const { return mCenter; }
    [[nodiscard]] glm::vec3 getColor() const { return mColor; }
    [[nodiscard]] glm::vec3 getNumberColor() const;
    [[nodiscard]] juce::Colour getColorJ() const { return mColorJ; }
    [[nodiscard]] juce::Colour getColorJWithAlpha() const;
    //==============================================================================
    [[nodiscard]] glm::vec3 polToCar(float azimuth, float zenith) const;
    [[nodiscard]] glm::vec3 polToCar3d(float azimuth, float zenith) const;
    //==============================================================================
    void resetPosition();
    void draw() const;
    void updateValues(float azimuth,
                      float zenith,
                      float azimuthSpan,
                      float zenithSpan,
                      float radius,
                      float gain,
                      SpatModes mode);
    void updateValuesOld(float azimuth, float zenith, float azimuthSpan, float zenithSpan, float g);
    //==============================================================================
    [[nodiscard]] bool isInput() const override { return true; }
    void changeDirectOutChannel(output_patch_t chn) override { mDirectOutChannel = chn; }
    void setDirectOutChannel(output_patch_t chn) override;
    [[nodiscard]] output_patch_t getDirectOutChannel() const override { return mDirectOutChannel; };
    void sendDirectOutToClient(int id, output_patch_t chn) override;

    void drawSpan() const;
    void drawSpanLbap(float x, float y, float z) const;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Input)
};
