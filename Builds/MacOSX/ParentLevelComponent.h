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
    virtual float getLevel() = 0;
    virtual int getId() = 0;
    virtual LevelComponent * getVuMeter() = 0;
};

#endif /* ParentLevelComponent_h */
