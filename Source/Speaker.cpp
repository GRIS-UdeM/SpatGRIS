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
static double getFloatPrecision(double value, double precision)
{
    return floor((value * pow(10, precision) + 0.5)) / pow(10, precision);
}

//==============================================================================
Speaker::Speaker(MainContentComponent * parent, int idS, int outP, float azimuth, float zenith, float radius)
{
    this->mMainContentComponent = parent;
    this->mId = idS;
    this->mOutputPatch = outP;
    this->mDirectOut = false;
    juce::LookAndFeel::setDefaultLookAndFeel(&mLookAndFeel);

    // Load position
    this->setAziZenRad(glm::vec3(azimuth, zenith, radius));

    this->mVuMeter = new LevelComponent(*this, mLookAndFeel, false);
}

//==============================================================================
Speaker::~Speaker()
{
    delete this->mVuMeter;
}

//==============================================================================
float Speaker::getLevel() const
{
    return this->mMainContentComponent->getLevelsOut(this->mOutputPatch - 1);
}

//==============================================================================
float Speaker::getAlpha()
{
    float alpha;
    if (this->mMainContentComponent->isSpeakerLevelShown()) {
        alpha = this->mMainContentComponent->getSpeakerLevelsAlpha(this->mOutputPatch - 1);
    } else {
        alpha = 1.0f;
    }
    if (std::isnan(alpha)) {
        alpha = 0.6f;
    }
    return alpha;
}

//==============================================================================
void Speaker::setMuted(bool mute)
{
    this->mMainContentComponent->muteOutput(this->mOutputPatch, mute);
    if (mute) {
        this->mMainContentComponent->soloOutput(this->mOutputPatch, false);
    }
}

//==============================================================================
void Speaker::setSolo(bool solo)
{
    this->mMainContentComponent->soloOutput(this->mOutputPatch, solo);
    if (solo) {
        this->mMainContentComponent->muteOutput(this->mOutputPatch, false);
    }
}

//==============================================================================
void Speaker::setCoordinate(glm::vec3 value)
{
    glm::vec3 newP;
    newP.x = atan2(value.z, value.x) / M_PI * 180.0f;
    if (newP.x < 0.0) {
        newP.x += 360.0f;
    }
    newP.y = value.y * 90.0f;
    newP.z = sqrt(value.x * value.x + value.z * value.z);
    this->setAziZenRad(newP);
}

//==============================================================================
void Speaker::normalizeRadius()
{
    if (!isDirectOut()) {
        glm::vec3 v = this->getAziZenRad();
        v.z = 1.0f;
        this->setAziZenRad(v);
    }
}

//==============================================================================
void Speaker::setAziZenRad(glm::vec3 value)
{
    value.z = value.z * 10.0f;
    this->mAziZenRad = value;
    this->newSpheriqueCoord(value);
}

//==============================================================================
void Speaker::setOutputPatch(int value)
{
    this->mOutputPatch = value;
    this->mVuMeter->setOutputLab(juce::String(this->mOutputPatch));
}

//==============================================================================
void Speaker::setDirectOut(bool value)
{
    this->mDirectOut = value;
    if (this->mDirectOut) {
        this->mColor = COLOR_DIRECT_OUT_SPEAKER;
    } else {
        this->mColor = COLOR_SPEAKER;
    }
}

//==============================================================================
glm::vec3 Speaker::getAziZenRad() const
{
    return glm::vec3(this->mAziZenRad.x, this->mAziZenRad.y, this->mAziZenRad.z / 10.0f);
}

//==============================================================================
bool Speaker::isValid() const
{
    return (this->mMin.x < this->mMax.x && this->mMin.y < this->mMax.y && this->mMin.z < this->mMax.z);
}

//==============================================================================
void Speaker::fix()
{
    glm::vec3 _max = (this->mMax);

    // Change new "min" to previous "max".
    if (this->mMin.x > this->mMax.x) {
        this->mMax.x = this->mMin.x;
        this->mMin.x = _max.x;
    }
    if (this->mMin.y > this->mMax.y) {
        this->mMax.y = this->mMin.y;
        this->mMin.y = _max.y;
    }
    if (this->mMin.z > this->mMax.z) {
        this->mMax.z = this->mMin.z;
        this->mMin.z = _max.z;
    }
}

//==============================================================================
void Speaker::selectClick(bool select)
{
    if (select) {
        this->mMainContentComponent->selectSpeaker(this->mId - 1);
    } else {
        this->mMainContentComponent->selectSpeaker(-1);
    }
}

//==============================================================================
void Speaker::selectSpeaker()
{
    this->mColor = COLOR_SPEAKER_SELECT;
    this->mSelected = true;
    this->mVuMeter->setSelected(this->mSelected);
}

//==============================================================================
void Speaker::unSelectSpeaker()
{
    if (this->mDirectOut) {
        this->mColor = COLOR_DIRECT_OUT_SPEAKER;
    } else {
        this->mColor = COLOR_SPEAKER;
    }
    this->mSelected = false;
    this->mVuMeter->setSelected(this->mSelected);
}

//==============================================================================
void Speaker::newPosition(glm::vec3 center, glm::vec3 extents)
{
    // min = center - extents, max = center + extents
    this->mMin.x = center.x - extents.x;
    this->mMin.y = center.y - extents.y;
    this->mMin.z = center.z - extents.z;

    this->mMax.x = center.x + extents.x;
    this->mMax.y = center.y + extents.y;
    this->mMax.z = center.z + extents.z;

    if (!this->isValid()) {
        this->fix();
    }

    this->mCenter = glm::vec3(this->mMin.x + (this->mMax.x - this->mMin.x) / 2.0f,
                              this->mMin.y + (this->mMax.y - this->mMin.y) / 2.0f,
                              this->mMin.z + (this->mMax.z - this->mMin.z) / 2.0f);
}

//==============================================================================
void Speaker::newSpheriqueCoord(glm::vec3 aziZenRad, glm::vec3 extents)
{
    glm::vec3 nCenter;

    aziZenRad.x = (aziZenRad.x * M_PI) / 180.0f;
    aziZenRad.y = abs(((-90.0f + aziZenRad.y) * M_PI) / 180.0f);

    if (this->mMainContentComponent->getModeSelected() == 1 || this->isDirectOut()) {
        nCenter.x = getFloatPrecision(aziZenRad.z * cosf(aziZenRad.x), 3);
        nCenter.z = getFloatPrecision(aziZenRad.z * sinf(aziZenRad.x), 3);
        nCenter.y = getFloatPrecision(10.f * (1.0 - aziZenRad.y / (M_PI / 2)), 3);
    } else {
        nCenter.x = getFloatPrecision(aziZenRad.z * sinf(aziZenRad.y) * cosf(aziZenRad.x), 3);
        nCenter.z = getFloatPrecision(aziZenRad.z * sinf(aziZenRad.y) * sinf(aziZenRad.x), 3);
        nCenter.y = getFloatPrecision(10.f * cosf(aziZenRad.y), 3);
    }
    this->newPosition(nCenter);
}

//==============================================================================
void Speaker::draw()
{
    float transpa = 0.75;

    glPushMatrix();

    glTranslatef(this->mCenter.x, this->mCenter.y, this->mCenter.z);

    glRotatef(180.0f - this->mAziZenRad.x, 0, 1.0, 0);
    if (this->mMainContentComponent->getModeSelected() == 1) {
        glRotatef(-this->mAziZenRad.y + this->mAziZenRad.y * this->mAziZenRad.z / 20.0, 0, 0, 1.0);
    } else {
        glRotatef(-this->mAziZenRad.y, 0, 0, 1.0);
    }
    glTranslatef(-1 * this->mCenter.x, -1 * this->mCenter.y, -1 * this->mCenter.z);

    glBegin(GL_QUADS);

    if (this->mMainContentComponent->isSpeakerLevelShown()) {
        float val = this->getAlpha();
        this->mLevelColour = val + (this->mLevelColour - val) * 0.5;
        glColor4f(this->mLevelColour, this->mLevelColour, this->mLevelColour, transpa);
    } else {
        glColor4f(this->mColor.x, this->mColor.y, this->mColor.z, transpa);
    }

    glVertex3f(this->mMin.x, this->mMin.y, this->mMax.z);
    glVertex3f(this->mMax.x, this->mMin.y, this->mMax.z);
    glVertex3f(this->mMax.x, this->mMax.y, this->mMax.z);
    glVertex3f(this->mMin.x, this->mMax.y, this->mMax.z);

    glVertex3f(this->mMax.x, this->mMin.y, this->mMax.z);
    glVertex3f(this->mMax.x, this->mMin.y, this->mMin.z);
    glVertex3f(this->mMax.x, this->mMax.y, this->mMin.z);
    glVertex3f(this->mMax.x, this->mMax.y, this->mMax.z);

    glVertex3f(this->mMin.x, this->mMax.y, this->mMax.z);
    glVertex3f(this->mMax.x, this->mMax.y, this->mMax.z);
    glVertex3f(this->mMax.x, this->mMax.y, this->mMin.z);
    glVertex3f(this->mMin.x, this->mMax.y, this->mMin.z);

    glVertex3f(this->mMin.x, this->mMin.y, this->mMin.z);
    glVertex3f(this->mMin.x, this->mMax.y, this->mMin.z);
    glVertex3f(this->mMax.x, this->mMax.y, this->mMin.z);
    glVertex3f(this->mMax.x, this->mMin.y, this->mMin.z);

    glVertex3f(this->mMin.x, this->mMin.y, this->mMin.z);
    glVertex3f(this->mMax.x, this->mMin.y, this->mMin.z);
    glVertex3f(this->mMax.x, this->mMin.y, this->mMax.z);
    glVertex3f(this->mMin.x, this->mMin.y, this->mMax.z);

    glVertex3f(this->mMin.x, this->mMin.y, this->mMin.z);
    glVertex3f(this->mMin.x, this->mMin.y, this->mMax.z);
    glVertex3f(this->mMin.x, this->mMax.y, this->mMax.z);
    glVertex3f(this->mMin.x, this->mMax.y, this->mMin.z);

    glEnd();

    if (this->mSelected) {
        glLineWidth(2);
        glBegin(GL_LINES);
        glColor4f(0, 0, 0, transpa);
        glVertex3f(this->mCenter.x + SIZE_SPEAKER.x, this->mCenter.y, this->mCenter.z);
        glVertex3f(this->mCenter.x + 1.2f, this->mCenter.y, this->mCenter.z);
        glEnd();

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(4);
        glBegin(GL_LINES);

        glVertex3f(this->mMin.x - OVER, this->mMin.y - OVER, this->mMin.z - OVER);
        glVertex3f(this->mMin.x - OVER, this->mMin.y - OVER, this->mMax.z + OVER);

        glVertex3f(this->mMax.x + OVER, this->mMin.y - OVER, this->mMin.z - OVER);
        glVertex3f(this->mMax.x + OVER, this->mMin.y - OVER, this->mMax.z + OVER);

        glVertex3f(this->mMax.x + OVER, this->mMax.y + OVER, this->mMin.z - OVER);
        glVertex3f(this->mMax.x + OVER, this->mMax.y + OVER, this->mMax.z + OVER);

        glVertex3f(this->mMin.x - OVER, this->mMax.y + OVER, this->mMin.z - OVER);
        glVertex3f(this->mMin.x - OVER, this->mMax.y + OVER, this->mMax.z + OVER);

        glVertex3f(this->mMin.x - OVER, this->mMin.y - OVER, this->mMin.z - OVER);
        glVertex3f(this->mMax.x + OVER, this->mMin.y - OVER, this->mMin.z - OVER);

        glVertex3f(this->mMin.x - OVER, this->mMin.y - OVER, this->mMax.z + OVER);
        glVertex3f(this->mMax.x + OVER, this->mMin.y - OVER, this->mMax.z + OVER);

        glVertex3f(this->mMin.x - OVER, this->mMax.y + OVER, this->mMin.z - OVER);
        glVertex3f(this->mMax.x + OVER, this->mMax.y + OVER, this->mMin.z - OVER);

        glVertex3f(this->mMin.x - OVER, this->mMax.y + OVER, this->mMax.z + OVER);
        glVertex3f(this->mMax.x + OVER, this->mMax.y + OVER, this->mMax.z + OVER);

        glVertex3f(this->mMin.x - OVER, this->mMin.y - OVER, this->mMin.z - OVER);
        glVertex3f(this->mMin.x - OVER, this->mMax.y + OVER, this->mMin.z - OVER);

        glVertex3f(this->mMin.x - OVER, this->mMin.y - OVER, this->mMax.z + OVER);
        glVertex3f(this->mMin.x - OVER, this->mMax.y + OVER, this->mMax.z + OVER);

        glVertex3f(this->mMax.x + OVER, this->mMin.y - OVER, this->mMin.z - OVER);
        glVertex3f(this->mMax.x + OVER, this->mMax.y + OVER, this->mMin.z - OVER);

        glVertex3f(this->mMax.x + OVER, this->mMin.y - OVER, this->mMax.z + OVER);
        glVertex3f(this->mMax.x + OVER, this->mMax.y + OVER, this->mMax.z + OVER);

        glEnd();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        glLineWidth(2);
        glBegin(GL_LINES);
        glColor4f(0.37, 0.37, 0.37, transpa);
        glVertex3f(this->mCenter.x + SIZE_SPEAKER.x, this->mCenter.y, this->mCenter.z);
        glVertex3f(this->mCenter.x + 1.2f, this->mCenter.y, this->mCenter.z);
        glEnd();
    }

    glPopMatrix();
}
