//
//  ParentLevelComponent.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-22.
//
//

#ifndef ParentLevelComponent_h
#define ParentLevelComponent_h

class LevelComponent;

class ParentLevelComponent{
public:
    virtual int getId() = 0;
    virtual float getLevel() = 0;
    virtual void setMuted(bool mute) = 0;
    virtual void setSolo(bool solo) = 0;
    virtual void setColor(Colour color, bool updateLevel = false) = 0;
    virtual void selectClick() = 0;
    virtual LevelComponent * getVuMeter() = 0;
    virtual ~ParentLevelComponent(){}
};

#endif /* ParentLevelComponent_h */
