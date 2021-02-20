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

#include "Speaker.h"

#include "LevelComponent.h"
#include "MainComponent.h"

//==============================================================================
template<typename T, typename U>
static T getFloatPrecision(T const value, U const precision)
{
    return std::floor((value * std::pow(static_cast<T>(10), static_cast<T>(precision)) + static_cast<T>(0.5)))
           / std::pow(static_cast<T>(10), static_cast<T>(precision));
}

//==============================================================================
Speaker::Speaker(MainContentComponent & mainContentComponent,
                 SmallGrisLookAndFeel & smallGrisLookAndFeel,
                 int const id,
                 int const outputPatch,
                 float const azimuth,
                 float const zenith,
                 float const radius)
    : mLookAndFeel(smallGrisLookAndFeel)
    , mMainContentComponent(mainContentComponent)
    , mId(id)
    , mOutputPatch(outputPatch)
    , mVuMeter(*this, mLookAndFeel, false)
{
    // Load position
    setAziZenRad(glm::vec3(azimuth, zenith, radius));
}

//==============================================================================
float Speaker::getLevel() const
{
    return mMainContentComponent.getLevelsOut(mOutputPatch - 1);
}

//==============================================================================
float Speaker::getAlpha() const
{
    float alpha;
    if (mMainContentComponent.isSpeakerLevelShown()) {
        alpha = mMainContentComponent.getSpeakerLevelsAlpha(mOutputPatch - 1);
    } else {
        alpha = 1.0f;
    }
    if (std::isnan(alpha)) {
        alpha = 0.6f;
    }
    return alpha;
}

//==============================================================================
void Speaker::setMuted(bool const mute)
{
    mMainContentComponent.muteOutput(mOutputPatch, mute);
    if (mute) {
        mMainContentComponent.soloOutput(mOutputPatch, false);
    }
}

//==============================================================================
void Speaker::setSolo(bool const solo)
{
    mMainContentComponent.soloOutput(mOutputPatch, solo);
    if (solo) {
        mMainContentComponent.muteOutput(mOutputPatch, false);
    }
}

//==============================================================================
void Speaker::setCoordinate(glm::vec3 const value)
{
    glm::vec3 newP;
    newP.x = atan2(value.z, value.x) / juce::MathConstants<float>::pi * 180.0f;
    if (newP.x < 0.0) {
        newP.x += 360.0f;
    }
    newP.y = value.y * 90.0f;
    newP.z = sqrt(value.x * value.x + value.z * value.z);
    setAziZenRad(newP);
}

//==============================================================================
void Speaker::normalizeRadius()
{
    if (!isDirectOut()) {
        glm::vec3 v = getPolarCoords();
        v.z = 1.0f;
        setAziZenRad(v);
    }
}

//==============================================================================
void Speaker::setAziZenRad(glm::vec3 value)
{
    value.z = value.z * 10.0f;
    mAziZenRad = value;
    newSphericalCoord(value);
}

//==============================================================================
void Speaker::setOutputPatch(int const value)
{
    mOutputPatch = value;
    mVuMeter.setOutputLab(juce::String(mOutputPatch));
}

//==============================================================================
void Speaker::setDirectOut(bool const value)
{
    mDirectOut = value;
    if (mDirectOut) {
        mColor = COLOR_DIRECT_OUT_SPEAKER;
    } else {
        mColor = COLOR_SPEAKER;
    }
}

//==============================================================================
glm::vec3 Speaker::getPolarCoords() const
{
    return glm::vec3(mAziZenRad.x, mAziZenRad.y, mAziZenRad.z / 10.0f);
}

//==============================================================================
bool Speaker::isValid() const
{
    return (mMin.x < mMax.x && mMin.y < mMax.y && mMin.z < mMax.z);
}

//==============================================================================
void Speaker::fix()
{
    auto const maxVec{ mMax };

    // Change new "min" to previous "max".
    if (mMin.x > mMax.x) {
        mMax.x = mMin.x;
        mMin.x = maxVec.x;
    }
    if (mMin.y > mMax.y) {
        mMax.y = mMin.y;
        mMin.y = maxVec.y;
    }
    if (mMin.z > mMax.z) {
        mMax.z = mMin.z;
        mMin.z = maxVec.z;
    }
}

//==============================================================================
void Speaker::selectClick(bool const select)
{
    if (select) {
        mMainContentComponent.selectSpeaker(mId - 1);
    } else {
        mMainContentComponent.selectSpeaker(-1);
    }
}

//==============================================================================
void Speaker::selectSpeaker()
{
    mColor = COLOR_SPEAKER_SELECT;
    mSelected = true;
    mVuMeter.setSelected(mSelected);
}

//==============================================================================
void Speaker::unSelectSpeaker()
{
    if (mDirectOut) {
        mColor = COLOR_DIRECT_OUT_SPEAKER;
    } else {
        mColor = COLOR_SPEAKER;
    }
    mSelected = false;
    mVuMeter.setSelected(mSelected);
}

//==============================================================================
void Speaker::newPosition(glm::vec3 const center, glm::vec3 const extents)
{
    // min = center - extents, max = center + extents
    mMin.x = center.x - extents.x;
    mMin.y = center.y - extents.y;
    mMin.z = center.z - extents.z;

    mMax.x = center.x + extents.x;
    mMax.y = center.y + extents.y;
    mMax.z = center.z + extents.z;

    if (!isValid()) {
        fix();
    }

    mCenter = glm::vec3(mMin.x + (mMax.x - mMin.x) / 2.0f,
                        mMin.y + (mMax.y - mMin.y) / 2.0f,
                        mMin.z + (mMax.z - mMin.z) / 2.0f);
}

//==============================================================================
void Speaker::newSphericalCoord(glm::vec3 aziZenRad, glm::vec3 /*extents*/)
{
    glm::vec3 nCenter;

    aziZenRad.x = (aziZenRad.x * juce::MathConstants<float>::pi) / 180.0f;
    aziZenRad.y = abs(((-90.0f + aziZenRad.y) * juce::MathConstants<float>::pi) / 180.0f);

    if (mMainContentComponent.getModeSelected() == SpatModes::lbap || isDirectOut()) {
        nCenter.x = getFloatPrecision(aziZenRad.z * cosf(aziZenRad.x), 3);
        nCenter.z = getFloatPrecision(aziZenRad.z * sinf(aziZenRad.x), 3);
        nCenter.y = getFloatPrecision(10.0f * (1.0f - aziZenRad.y / (juce::MathConstants<float>::halfPi)), 3);
    } else {
        nCenter.x = getFloatPrecision(aziZenRad.z * sinf(aziZenRad.y) * cosf(aziZenRad.x), 3);
        nCenter.z = getFloatPrecision(aziZenRad.z * sinf(aziZenRad.y) * sinf(aziZenRad.x), 3);
        nCenter.y = getFloatPrecision(10.0f * cosf(aziZenRad.y), 3);
    }
    newPosition(nCenter);
}

//==============================================================================
void Speaker::draw()
{
    static auto constexpr ALPHA = 0.75f;

    glPushMatrix();

    glTranslatef(mCenter.x, mCenter.y, mCenter.z);

    glRotatef(180.0f - mAziZenRad.x, 0.0f, 1.0f, 0.0f);
    if (mMainContentComponent.getModeSelected() == SpatModes::lbap) {
        glRotatef(-mAziZenRad.y + mAziZenRad.y * mAziZenRad.z / 20.0f, 0.0f, 0.0f, 1.0f);
    } else {
        glRotatef(-mAziZenRad.y, 0.0f, 0.0f, 1.0f);
    }
    glTranslatef(-1.0f * mCenter.x, -1.0f * mCenter.y, -1.0f * mCenter.z);

    glBegin(GL_QUADS);

    if (mMainContentComponent.isSpeakerLevelShown()) {
        auto const alpha{ getAlpha() };
        mLevelColour = alpha + (mLevelColour - alpha) * 0.5f;
        glColor4f(mLevelColour, mLevelColour, mLevelColour, ALPHA);
    } else {
        glColor4f(mColor.x, mColor.y, mColor.z, ALPHA);
    }

    glVertex3f(mMin.x, mMin.y, mMax.z);
    glVertex3f(mMax.x, mMin.y, mMax.z);
    glVertex3f(mMax.x, mMax.y, mMax.z);
    glVertex3f(mMin.x, mMax.y, mMax.z);

    glVertex3f(mMax.x, mMin.y, mMax.z);
    glVertex3f(mMax.x, mMin.y, mMin.z);
    glVertex3f(mMax.x, mMax.y, mMin.z);
    glVertex3f(mMax.x, mMax.y, mMax.z);

    glVertex3f(mMin.x, mMax.y, mMax.z);
    glVertex3f(mMax.x, mMax.y, mMax.z);
    glVertex3f(mMax.x, mMax.y, mMin.z);
    glVertex3f(mMin.x, mMax.y, mMin.z);

    glVertex3f(mMin.x, mMin.y, mMin.z);
    glVertex3f(mMin.x, mMax.y, mMin.z);
    glVertex3f(mMax.x, mMax.y, mMin.z);
    glVertex3f(mMax.x, mMin.y, mMin.z);

    glVertex3f(mMin.x, mMin.y, mMin.z);
    glVertex3f(mMax.x, mMin.y, mMin.z);
    glVertex3f(mMax.x, mMin.y, mMax.z);
    glVertex3f(mMin.x, mMin.y, mMax.z);

    glVertex3f(mMin.x, mMin.y, mMin.z);
    glVertex3f(mMin.x, mMin.y, mMax.z);
    glVertex3f(mMin.x, mMax.y, mMax.z);
    glVertex3f(mMin.x, mMax.y, mMin.z);

    glEnd();

    if (mSelected) {
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glColor4f(0.0f, 0.0f, 0.0f, ALPHA);
        glVertex3f(mCenter.x + SIZE_SPEAKER.x, mCenter.y, mCenter.z);
        glVertex3f(mCenter.x + 1.2f, mCenter.y, mCenter.z);
        glEnd();

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(4.0f);
        glBegin(GL_LINES);

        glVertex3f(mMin.x - OVER, mMin.y - OVER, mMin.z - OVER);
        glVertex3f(mMin.x - OVER, mMin.y - OVER, mMax.z + OVER);

        glVertex3f(mMax.x + OVER, mMin.y - OVER, mMin.z - OVER);
        glVertex3f(mMax.x + OVER, mMin.y - OVER, mMax.z + OVER);

        glVertex3f(mMax.x + OVER, mMax.y + OVER, mMin.z - OVER);
        glVertex3f(mMax.x + OVER, mMax.y + OVER, mMax.z + OVER);

        glVertex3f(mMin.x - OVER, mMax.y + OVER, mMin.z - OVER);
        glVertex3f(mMin.x - OVER, mMax.y + OVER, mMax.z + OVER);

        glVertex3f(mMin.x - OVER, mMin.y - OVER, mMin.z - OVER);
        glVertex3f(mMax.x + OVER, mMin.y - OVER, mMin.z - OVER);

        glVertex3f(mMin.x - OVER, mMin.y - OVER, mMax.z + OVER);
        glVertex3f(mMax.x + OVER, mMin.y - OVER, mMax.z + OVER);

        glVertex3f(mMin.x - OVER, mMax.y + OVER, mMin.z - OVER);
        glVertex3f(mMax.x + OVER, mMax.y + OVER, mMin.z - OVER);

        glVertex3f(mMin.x - OVER, mMax.y + OVER, mMax.z + OVER);
        glVertex3f(mMax.x + OVER, mMax.y + OVER, mMax.z + OVER);

        glVertex3f(mMin.x - OVER, mMin.y - OVER, mMin.z - OVER);
        glVertex3f(mMin.x - OVER, mMax.y + OVER, mMin.z - OVER);

        glVertex3f(mMin.x - OVER, mMin.y - OVER, mMax.z + OVER);
        glVertex3f(mMin.x - OVER, mMax.y + OVER, mMax.z + OVER);

        glVertex3f(mMax.x + OVER, mMin.y - OVER, mMin.z - OVER);
        glVertex3f(mMax.x + OVER, mMax.y + OVER, mMin.z - OVER);

        glVertex3f(mMax.x + OVER, mMin.y - OVER, mMax.z + OVER);
        glVertex3f(mMax.x + OVER, mMax.y + OVER, mMax.z + OVER);

        glEnd();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glColor4f(0.37f, 0.37f, 0.37f, ALPHA);
        glVertex3f(mCenter.x + SIZE_SPEAKER.x, mCenter.y, mCenter.z);
        glVertex3f(mCenter.x + 1.2f, mCenter.y, mCenter.z);
        glEnd();
    }

    glPopMatrix();
}
