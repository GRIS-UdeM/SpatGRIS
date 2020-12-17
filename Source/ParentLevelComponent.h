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
#include <JuceHeader.h>
ENABLE_WARNINGS

class LevelComponent;

//==============================================================================
class ParentLevelComponent
{
protected:
    //==============================================================================
    int mDirectOutChannel = 0;

public:
    //==============================================================================
    ParentLevelComponent() = default;
    virtual ~ParentLevelComponent() = default;

    ParentLevelComponent(ParentLevelComponent const &) = delete;
    ParentLevelComponent(ParentLevelComponent &&) = delete;

    ParentLevelComponent & operator=(ParentLevelComponent const &) = delete;
    ParentLevelComponent & operator=(ParentLevelComponent &&) = delete;
    //==============================================================================
    [[nodiscard]] virtual int getId() const = 0;
    [[nodiscard]] virtual int getButtonInOutNumber() const = 0;
    [[nodiscard]] virtual bool isInput() const = 0;
    [[nodiscard]] virtual float getLevel() const = 0;
    virtual void setMuted(bool mute) = 0;
    virtual void setSolo(bool solo) = 0;
    virtual void setColor(juce::Colour color, bool updateLevel = false) = 0;
    virtual void selectClick(bool select = true) = 0;

    [[nodiscard]] virtual LevelComponent * getVuMeter() = 0;
    [[nodiscard]] virtual LevelComponent const * getVuMeter() const = 0;

    virtual void changeDirectOutChannel(int chn) = 0;
    virtual void setDirectOutChannel(int chn) = 0;
    [[nodiscard]] virtual int getDirectOutChannel() const = 0;
    virtual void sendDirectOutToClient(int id, int chn) = 0;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(ParentLevelComponent)
};
