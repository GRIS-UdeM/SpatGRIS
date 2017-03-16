//
//  Input.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-16.
//
//

#ifndef Input_h
#define Input_h

#include <stdio.h>
#include <iostream>

#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>

#include "../glm/glm.hpp"

using namespace std;
class Input
{

public :
    Input(int id = 0);
    ~Input();
    
    int getId();
    glm::vec3 getCenter();
    void draw();
    void updateValues(float az, float ze, float azS, float zeS, float g);
    
private:
    int idChannel;
    
    float azimuth;
    float zenith;
    float azimSpan;
    float zeniSpan;
    float gain;
    
    float sizeT = 0.5f;
    glm::vec3 center;
};
#endif /* Input_h */
