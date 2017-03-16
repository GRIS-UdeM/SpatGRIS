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
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>

#include "../glm/glm.hpp"

class Input
{

public :
    Input(int id = 0);
    ~Input();
    
    void draw();
    void updateValues(float az, float ze, float azS, float zeS, float g);
    
private:
    int idChannel;
    float azimuth;
    float zenith;
    float azimSpan;
    float zeniSpan;
    float gain;
    glm::vec3 position;
};
#endif /* Input_h */
