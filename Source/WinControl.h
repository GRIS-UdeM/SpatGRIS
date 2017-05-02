//
//  WinControl.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-05-02.
//
//

#ifndef WinControl_h
#define WinControl_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "GrisLookAndFeel.h"

class MainContentComponent;
class Input;

using namespace std;

static const float RadiusMax        = 2.f;
static const float  SourceRadius    = 10.f;
static const float  SourceDiameter  = SourceRadius * 2.f;
typedef Point<float> FPoint;

static float DegreeToRadian (float degree){
    return ((degree * M_PI ) / 180.0f) ;
}

static FPoint GetSourceAzimElev(FPoint pXY, bool bUseCosElev = false)
{
    //calculate azim in range [0,1], and negate it because zirkonium wants -1 on right side
    float fAzim = -atan2f(pXY.x, pXY.y)/M_PI;
    
    //calculate xy distance from origin, and clamp it to 2 (ie ignore outside of circle)
    float hypo = hypotf(pXY.x, pXY.y);
    if (hypo > RadiusMax){
        hypo = RadiusMax;
    }
    
    float fElev;
    if (bUseCosElev){
        fElev = acosf(hypo/RadiusMax);   //fElev is elevation in radian, [0,pi/2)
        fElev /= (M_PI/2.f);                      //making range [0,1]
        fElev /= 2.f;                            //making range [0,.5] because that's what the zirkonium wants
    } else {
        fElev = (RadiusMax-hypo)/4.0f;
    }
    
    return FPoint(fAzim, fElev);
}

static FPoint DegreeToXy (FPoint p, int FieldWidth)
{
    float x,y;
    x = -((FieldWidth - SourceDiameter)/2) * sinf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    y = -((FieldWidth - SourceDiameter)/2) * cosf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    return FPoint(x, y);
}


class WinControl :   public DocumentWindow,
private Timer

{
public:
    WinControl(const String& name, Colour backgroundColour, int buttonsNeeded,MainContentComponent * parent, GrisLookAndFeel * feel);
    ~WinControl();
    
    void timerCallback() override;
    void paint (Graphics& g) override;
    void resized() override;
    void closeButtonPressed() override;
    
private:
    void drawAzimElevSource(Graphics &g,  Input *  it, const int fieldWH, const int fieldCenter);
    
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WinControl)
};

#endif /* WinControl_h */
