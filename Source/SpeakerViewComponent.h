/*
 This file is part of spatServerGRIS.
 
 spatServerGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 spatServerGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with spatServerGRIS.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SPEAKERVIEWCOMPONENT_H_INCLUDED
#define SPEAKERVIEWCOMPONENT_H_INCLUDED


#include "../JuceLibraryCode/JuceHeader.h"
#include "Resources/WavefrontObjParser.h"
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <GLUT/glut.h>
#include "../glm/glm.hpp"


#include <math.h>

using namespace std;

const glm::vec3 colorSpeaker = glm::vec3(0.85, 0.86, 0.87);
const glm::vec3 colorSpeakerSelect = glm::vec3(1.0, 0.66, 0.67);
/*
struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};
*/
class Ray
{
public:
    Ray(const glm::vec3 orig, const glm::vec3 dir) : orig(orig), dir(dir)
    {
        invdir = 1.0f / dir;
        sign[0] = (invdir.x < 0);
        sign[1] = (invdir.y < 0);
        sign[2] = (invdir.z < 0);
    }
    glm::vec3 orig, dir;       // ray orig and dir
    glm::vec3 invdir;
    int sign[3];
};


class SpeakerObj{
public:
    SpeakerObj(glm::vec3 pos = glm::vec3(0,0,0));
    ~SpeakerObj();
    
    void draw();
    
    void selectSpeaker();
    void unSelectSpeaker();
    void setPosition(glm::vec3 pos);
    glm::vec3 getPosition();
    
    glm::vec3 getMin(){ return glm::vec3(this->position.x-0.5f,this->position.y-0.5f,this->position.z-0.5f); }
    glm::vec3 getMax(){ return glm::vec3(this->position.x+0.5f,this->position.y+0.5f,this->position.z+0.5f); }
    

private:
    glm::vec3 position;
    glm::vec3 color = colorSpeaker;
    
    const float sizex = 0.5f;
    const float sizey = 0.5f;
    const float sizez = 0.5f;
    


};

class SpeakerViewComponent : public OpenGLAppComponent {
public:
    //==============================================================================
    SpeakerViewComponent();
    
    ~SpeakerViewComponent();
    
    void initialise() override;
    
    void shutdown() override;
    

    void render() override;
    
    void paint (Graphics& g) override;
    void resized() override;
    
    void mouseDown(const MouseEvent& e)override;
    void mouseDrag (const MouseEvent& e)override;
    
    void mouseWheelMove(const MouseEvent& e,const MouseWheelDetails& wheel)override;

    

    
private:

    void drawBackground();
    void drawOriginGrid();
    void drawCube(float x, float y, float z);
    

    float camAngleX=35;
    float camAngleY=35;
    float distance = 15.0f;
    
    Ray r;

    glm::vec2 posC;
    
    std::vector<SpeakerObj> listSpeaker;
    
    ComponentDragger myDragger;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerViewComponent)
};




#endif  // SPEAKERVIEWCOMPONENT_H_INCLUDED
