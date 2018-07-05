/*
 This file is part of ServerGris.
 
 Developers: Olivier Belanger, Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Speaker.h"
#include "MainComponent.h"
#include "LevelComponent.h"

static double GetFloatPrecision(double value, double precision) {
    return floor((value * pow(10, precision) + 0.5)) / pow(10, precision);
}

Speaker::Speaker(MainContentComponent *parent, int idS) {
    Speaker(parent, idS, idS, glm::vec3(0,0,0));
}

Speaker::Speaker(MainContentComponent *parent, int idS, int outP,
                 glm::vec3 center, glm::vec3 extents) {
    this->mainParent = parent;
    this->idSpeaker = idS;
    this->outputPatch = outP;
    this->directOut = false;
    LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);
    
    // Load position
    this->newPosition(center, extents);
    this->vuMeter = new LevelComponent(this, &mGrisFeel, false);
}

Speaker::~Speaker() {
    delete this->vuMeter;
}

float Speaker::getLevel() {
    return this->mainParent->getLevelsOut(this->outputPatch-1);
}

float Speaker::getAlpha() {
    float alpha;
    if (this->mainParent->isSpeakerLevelShown) {
        alpha = this->mainParent->getSpeakerLevelsAlpha(this->outputPatch-1);
    } else {
        alpha = 1.0f;
    }
    if (isnan(alpha)) {
        alpha = 0.6f;
    }
    return alpha;
}

void Speaker::setMuted(bool mute) {
    this->mainParent->muteOutput(this->outputPatch, mute);
    if (mute) {
        this->mainParent->soloOutput(this->outputPatch, false);
    }
}

void Speaker::setSolo(bool solo) {
    this->mainParent->soloOutput(this->outputPatch, solo);
    if (solo) {
        this->mainParent->muteOutput(this->outputPatch, false);
    }
}

void Speaker::setColor(Colour color, bool updateLevel) {}

int Speaker::getIdSpeaker() {
    return this->idSpeaker;
}

glm::vec3 Speaker::getCoordinate() {
    return this->center / 10.0f ;
}

glm::vec3 Speaker::getAziZenRad() {
    return glm::vec3(this->aziZenRad.x, this->aziZenRad.y, this->aziZenRad.z / 10.0f);
}

void Speaker::setCoordinate(glm::vec3 value) {
    this->newPosition(value * 10.0f);
}
void Speaker::setAziZenRad(glm::vec3 value) {
    value.z = value.z * 10.0f;
    this->newSpheriqueCoord(value);
}

int Speaker::getOutputPatch() {
    return this->outputPatch;
}

void Speaker::setOutputPatch(int value) {
    this->outputPatch = value;
    this->vuMeter->setOutputLab(String(this->outputPatch));
}

float Speaker::getGain() {
    return this->gain;
}

void Speaker::setGain(float value) {
    this->gain = value;
}

float Speaker::getHighPassCutoff() {
    return this->hpCutoff;
}

void Speaker::setHighPassCutoff(float value) {
    this->hpCutoff = value;
}

bool Speaker::getDirectOut() {
    return this->directOut;
}

void Speaker::setDirectOut(bool value) {
    this->directOut = value;
    if (this->directOut) {
        this->color = ColorDirectOutSpeaker;
    } else {
        this->color = ColorSpeaker;
    }
}

glm::vec3 Speaker::getMin() {
    return this->min;
}

glm::vec3 Speaker::getMax() {
    return this->max;
}

glm::vec3 Speaker::getCenter() {
    return this->center;
}

bool Speaker::isValid() {
    return (this->min.x < this->max.x && this->min.y < this->max.y && this->min.z < this->max.z);
}

void Speaker::fix() {
    glm::vec3 _max = (this->max);

    // Change new "min" to previous "max".
    if (this->min.x > this->max.x) {
        this->max.x = this->min.x;
        this->min.x = _max.x;
    }
    if (this->min.y > this->max.y) {
        this->max.y = this->min.y;
        this->min.y = _max.y;
    }
    if (this->min.z > this->max.z) {
        this->max.z = this->min.z;
        this->min.z = _max.z;
    }
}

void Speaker::selectClick(bool select) {
    if (select) {
        this->mainParent->selectSpeaker(this->idSpeaker-1);
    } else {
        this->mainParent->selectSpeaker(-1);
    }
}

bool Speaker::isSelected() {
    return this->selected;
}

void Speaker::selectSpeaker() {
    this->color = ColorSpeakerSelect;
    this->selected = true;
    this->vuMeter->setSelected(this->selected);
}

void Speaker::unSelectSpeaker() {
    if (this->directOut) {
        this->color = ColorDirectOutSpeaker;
    } else {
        this->color = ColorSpeaker;
    }
    this->selected = false;
    this->vuMeter->setSelected(this->selected);
}


void Speaker::newPosition(glm::vec3 center, glm::vec3 extents) {
    // min = center - extents, max = center + extents
    this->min.x = center.x - extents.x;
    this->min.y = center.y - extents.y;
    this->min.z = center.z - extents.z;
    
    this->max.x = center.x + extents.x;
    this->max.y = center.y + extents.y;
    this->max.z = center.z + extents.z;

    if (!this->isValid()) {
        this->fix();
    }

    this->center = glm::vec3(this->min.x + (this->max.x - this->min.x) / 2.0f,
                             this->min.y + (this->max.y - this->min.y) / 2.0f,
                             this->min.z + (this->max.z - this->min.z) / 2.0f);
    
    float azimuth = ((atan2(this->center.x, this->center.z) * 180.0f) / M_PI) + 90.0f;
    float plane = sqrt(this->center.x*this->center.x + this->center.z*this->center.z);
    float zenith = atan2(this->center.y, plane) * 180.0f / M_PI;
    float radius = sqrt(this->center.x * this->center.x +
                        this->center.y * this->center.y +
                        this->center.z * this->center.z);
    azimuth = 180.0f - azimuth;

    if (azimuth < 0.0f) {
        azimuth += 360.0f;
    }
    if (zenith < -90.0f) {
        zenith = -90.0f;
    } else if (zenith > 90.0f) {
        zenith = 90.0f;
    }
    azimuth = GetFloatPrecision(azimuth, 2);
    if (azimuth == -0.0f) {
        azimuth = 0.0f;
    }
    zenith = GetFloatPrecision(zenith, 2);
    if (zenith == -0.0f) {
        zenith = 0.0f;
    }

    radius = GetFloatPrecision(radius, 2);

    this->aziZenRad = glm::vec3(azimuth, zenith, radius);
}

void Speaker::newSpheriqueCoord(glm::vec3 aziZenRad, glm::vec3 extents) {
    glm::vec3 nCenter;

    aziZenRad.x = abs((aziZenRad.x * M_PI) / 180.0f);
    aziZenRad.y = abs(((-90.0f + aziZenRad.y) * M_PI) / 180.0f);

    nCenter.x = GetFloatPrecision(aziZenRad.z * sinf(aziZenRad.y) * cosf(aziZenRad.x), 3);
    nCenter.z = GetFloatPrecision(aziZenRad.z * sinf(aziZenRad.y) * sinf(aziZenRad.x), 3);
    nCenter.y = GetFloatPrecision(aziZenRad.z * cosf(aziZenRad.y), 3);
    
    this->newPosition(nCenter);
}

void Speaker::draw() {
    float transpa = 0.75;

    glPushMatrix();

    glTranslatef(this->center.x, this->center.y, this->center.z);

    glRotatef(180.0f-this->aziZenRad.x, 0, 1.0, 0);
    glRotatef(-this->aziZenRad.y, 0, 0, 1.0);
    glTranslatef(-1*this->center.x, -1*this->center.y, -1*this->center.z);

    glBegin(GL_QUADS);

    if (this->mainParent->isSpeakerLevelShown) {
        float val = this->getAlpha();
        this->levelColour = val + (this->levelColour - val) * 0.5;
        glColor4f(this->levelColour, this->levelColour, this->levelColour, transpa);
    } else {
        glColor4f(this->color.x, this->color.y, this->color.z, transpa);
    }

    glVertex3f(this->min.x, this->min.y, this->max.z);
    glVertex3f(this->max.x, this->min.y, this->max.z);
    glVertex3f(this->max.x, this->max.y, this->max.z);
    glVertex3f(this->min.x, this->max.y, this->max.z);
    
    glVertex3f(this->max.x, this->min.y, this->max.z);
    glVertex3f(this->max.x, this->min.y, this->min.z);
    glVertex3f(this->max.x, this->max.y, this->min.z);
    glVertex3f(this->max.x, this->max.y, this->max.z);
    
    glVertex3f(this->min.x, this->max.y, this->max.z);
    glVertex3f(this->max.x, this->max.y, this->max.z);
    glVertex3f(this->max.x, this->max.y, this->min.z);
    glVertex3f(this->min.x, this->max.y, this->min.z);
    
    glVertex3f(this->min.x, this->min.y, this->min.z);
    glVertex3f(this->min.x, this->max.y, this->min.z);
    glVertex3f(this->max.x, this->max.y, this->min.z);
    glVertex3f(this->max.x, this->min.y, this->min.z);
    
    glVertex3f(this->min.x, this->min.y, this->min.z);
    glVertex3f(this->max.x, this->min.y, this->min.z);
    glVertex3f(this->max.x, this->min.y, this->max.z);
    glVertex3f(this->min.x, this->min.y, this->max.z);
    
    glVertex3f(this->min.x, this->min.y, this->min.z);
    glVertex3f(this->min.x, this->min.y, this->max.z);
    glVertex3f(this->min.x, this->max.y, this->max.z);
    glVertex3f(this->min.x, this->max.y, this->min.z);
    
    glEnd();
    
    if (this->selected) {
        glLineWidth(2);
        glBegin(GL_LINES);
        glColor4f(0, 0, 0, transpa);
        glVertex3f(this->center.x+SizeSpeaker.x, this->center.y, this->center.z);
        glVertex3f(this->center.x + 1.2f, this->center.y, this->center.z);
        glEnd();

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(4);
        glBegin(GL_LINES);

        glVertex3f(this->min.x-Over, this->min.y-Over, this->min.z-Over);
        glVertex3f(this->min.x-Over, this->min.y-Over, this->max.z+Over);
        
        glVertex3f(this->max.x+Over, this->min.y-Over, this->min.z-Over);
        glVertex3f(this->max.x+Over, this->min.y-Over, this->max.z+Over);
        
        glVertex3f(this->max.x+Over, this->max.y+Over, this->min.z-Over);
        glVertex3f(this->max.x+Over, this->max.y+Over, this->max.z+Over);
        
        glVertex3f(this->min.x-Over, this->max.y+Over, this->min.z-Over);
        glVertex3f(this->min.x-Over, this->max.y+Over, this->max.z+Over);

        glVertex3f(this->min.x-Over, this->min.y-Over, this->min.z-Over);
        glVertex3f(this->max.x+Over, this->min.y-Over, this->min.z-Over);
        
        glVertex3f(this->min.x-Over, this->min.y-Over, this->max.z+Over);
        glVertex3f(this->max.x+Over, this->min.y-Over, this->max.z+Over);
        
        glVertex3f(this->min.x-Over, this->max.y+Over, this->min.z-Over);
        glVertex3f(this->max.x+Over, this->max.y+Over, this->min.z-Over);
        
        glVertex3f(this->min.x-Over, this->max.y+Over, this->max.z+Over);
        glVertex3f(this->max.x+Over, this->max.y+Over, this->max.z+Over);

        glVertex3f(this->min.x-Over, this->min.y-Over, this->min.z-Over);
        glVertex3f(this->min.x-Over, this->max.y+Over, this->min.z-Over);
        
        glVertex3f(this->min.x-Over, this->min.y-Over, this->max.z+Over);
        glVertex3f(this->min.x-Over, this->max.y+Over, this->max.z+Over);
        
        glVertex3f(this->max.x+Over, this->min.y-Over, this->min.z-Over);
        glVertex3f(this->max.x+Over, this->max.y+Over, this->min.z-Over);
        
        glVertex3f(this->max.x+Over, this->min.y-Over, this->max.z+Over);
        glVertex3f(this->max.x+Over, this->max.y+Over, this->max.z+Over);
        
        glEnd();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        glLineWidth(2);
        glBegin(GL_LINES);
        glColor4f(0.37, 0.37, 0.37, transpa);
        glVertex3f(this->center.x+SizeSpeaker.x, this->center.y, this->center.z);
        glVertex3f(this->center.x + 1.2f, this->center.y, this->center.z);
        glEnd();
    }

    glPopMatrix();
}
