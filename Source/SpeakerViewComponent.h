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
struct Ray{
    
    glm::vec3 orig;
    
    glm::vec3 dir;
    glm::vec3 invdir;
    
};
*/
class Ray {
    
public :
    Ray() {
        this->position = glm::vec3(0,0,0);
        this->direction = glm::vec3(0,0,0);
        this->normal = glm::vec3(0,0,0);
    }
    
    Ray(glm::vec3 p, glm::vec3 d) {
        this->position = (p);//glm::vec3(p.x, p.y, p.z);
        this->direction = d;
        this->normal = normalize(glm::vec3(d.x, d.y, d.z));
    }
    
    glm::vec3 Normal() {
        return this->normal;
    }
    glm::vec3 orig() {
        return this->position;
    }
    
    glm::vec3 dir() {
        return this->direction;
    }
    


    
private:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 normal;
    
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
    
    glm::vec3 getMin(){
        
        /*return glm::vec3(min( this->position.x+0.5f, this->position.x-0.5f ),
        min( this->position.y+0.5f, this->position.y-0.5f ),
        min( this->position.z+0.5f, this->position.z-0.5f ));*/
        
        return (glm::vec3(this->position.x-0.5f,this->position.y-0.5f,this->position.z-0.5f));
    }
    glm::vec3 getMax(){
        /*return glm::vec3(max( this->position.x+0.5f, this->position.x-0.5f ),
                         max( this->position.y+0.5f, this->position.y-0.5f ),
                         max( this->position.z+0.5f, this->position.z-0.5f ));*/

        return (glm::vec3(this->position.x+0.5f,this->position.y+0.5f,this->position.z+0.5f));
    }
    

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
    void printMatrix(glm::vec3 m){
        cout << "["<< m.x << " . "<<  m.y << " . "<<  m.z << "]"<<endl;
    }
    glm::vec4 trace(glm::vec3 origin, glm::vec3 dir);
    

    void drawBackground();
    void drawOriginGrid();
    void drawCube(float x, float y, float z);
    

    float camAngleX=35;
    float camAngleY=35;
    float distance = 15.0f;
    
    Ray r;

    glm::vec2 posC;
     glm::vec3 t1, t2;
    std::vector<SpeakerObj> listSpeaker;
    
    ComponentDragger myDragger;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerViewComponent)
};




#endif  // SPEAKERVIEWCOMPONENT_H_INCLUDED
