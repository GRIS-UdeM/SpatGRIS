/*
 ==============================================================================
 
 This file was auto-generated!
 
 ==============================================================================
 */

#ifndef SPEAKERVIEWCOMPONENT_H_INCLUDED
#define SPEAKERVIEWCOMPONENT_H_INCLUDED


#include "../JuceLibraryCode/JuceHeader.h"
#include "Resources/WavefrontObjParser.h"


class Shape;
class Attributes;
struct Uniforms;

class SpeakerViewComponent : public OpenGLAppComponent {
public:
    //==============================================================================
    SpeakerViewComponent();
    
    ~SpeakerViewComponent();
    
    void initialise() override;
    
    void shutdown() override;
    
    Matrix3D<float> getProjectionMatrix() const;
    
    Matrix3D<float> getViewMatrix() const;
    
    void render() override;
    
    void paint (Graphics& g) override;
    void resized() override;
    
    void mouseDown (const MouseEvent& e) override;
    
    void mouseDrag (const MouseEvent& e) override;
    
    void createShaders();
    
    Draggable3DOrientation draggableOrientation;
    
    
private:
    
    const char* vertexShader;
    const char* fragmentShader;
    
    ScopedPointer<OpenGLShaderProgram> shader;
    ScopedPointer<Shape> shape;
    ScopedPointer<Attributes> attributes;
    ScopedPointer<Uniforms> uniforms;
    
    String newVertexShader, newFragmentShader;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerViewComponent)
};




#endif  // SPEAKERVIEWCOMPONENT_H_INCLUDED
