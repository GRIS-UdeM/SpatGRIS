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

class VuMeterComponent;

//==============================================================================
class VuMeterModel
{
protected:
    //==============================================================================
    int mChannel;

public:
    //==============================================================================
    explicit VuMeterModel(int const channel) : mChannel(channel) {}
    virtual ~VuMeterModel() = default;
    //==============================================================================
    VuMeterModel(VuMeterModel const &) = delete;
    VuMeterModel(VuMeterModel &&) = delete;
    VuMeterModel & operator=(VuMeterModel const &) = delete;
    VuMeterModel & operator=(VuMeterModel &&) = delete;
    //==============================================================================
    [[nodiscard]] int getChannel() const { return mChannel; }
    //==============================================================================
    [[nodiscard]] virtual bool isInput() const = 0;
    [[nodiscard]] virtual dbfs_t getLevel() const = 0;
    virtual void setState(PortState state) = 0;
    virtual void setColor(juce::Colour color, bool updateLevel = false) = 0;
    virtual void setSelected(bool state) = 0;
    [[nodiscard]] virtual VuMeterComponent * getVuMeter() = 0;
    [[nodiscard]] virtual VuMeterComponent const * getVuMeter() const = 0;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(VuMeterModel)
};
