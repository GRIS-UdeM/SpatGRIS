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
#include <math.h>

using namespace std;
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
    

    
private:


    double rotate_y=0;
    double rotate_x=0;
    
     ComponentDragger myDragger;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerViewComponent)
};




#endif  // SPEAKERVIEWCOMPONENT_H_INCLUDED
