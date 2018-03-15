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

#ifndef ParentLevelComponent_h
#define ParentLevelComponent_h

class LevelComponent;

class ParentLevelComponent{
public:
    virtual int getId() = 0;
    virtual bool isInput() = 0;
    virtual float getLevel() = 0;
    virtual void setMuted(bool mute) = 0;
    virtual void setSolo(bool solo) = 0;
    virtual void setColor(Colour color, bool updateLevel = false) = 0;
    virtual void selectClick(bool select = true) = 0;
    virtual LevelComponent * getVuMeter() = 0;
    virtual void changeDirectOutChannel(int chn) = 0;
    virtual void setDirectOutChannel(int chn) = 0;
    virtual int getDirectOutChannel() = 0;
    virtual void sendDirectOutToClient(int id, int chn) = 0;
    virtual ~ParentLevelComponent(){}

    int directOutChannel = 0;
};

#endif /* ParentLevelComponent_h */
