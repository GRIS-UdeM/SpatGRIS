//
//  LevelComponent.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-13.
//
//

#ifndef LevelComponent_h
#define LevelComponent_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "GrisLookAndFeel.h"
#include "UiComponent.h"

static const float MinLevelComp = -60.f;
static const float MaxLevelComp = 1.f;
static const float MaxMinLevComp = MaxLevelComp - MinLevelComp;

class MainContentComponent;

//======================================= LevelBox ===================================
class LevelBox : public Component
{
public:
    LevelBox(LevelComponent* parent, GrisLookAndFeel *feel);
    ~LevelBox();
    
    void setBounds(const Rectangle<int> &newBounds);
    void paint (Graphics& g);
    
private:
    LevelComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    ColourGradient colorGrad;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelBox)
};

//====================================================================================
class LevelComponent : public Component, public ToggleButton::Listener
{
public:
    LevelComponent(MainContentComponent* parent, GrisLookAndFeel *feel, int id=-1);
    ~LevelComponent();
    
    void setOutputLab(String value) { this->indexLab->setText(value, dontSendNotification); }
    float getLevel();
    void update();
    bool isMuted();
    void setSelected(bool value);
    void buttonClicked(Button *button);
    void setBounds(const Rectangle<int> &newBounds);

private:
    MainContentComponent* mainParent;
    LevelBox *levelBox;
    Label *indexLab;
    ToggleButton *muteToggleBut;
    GrisLookAndFeel *grisFeel;
    int index;
    bool muted;
    bool selected;
    float level = -100.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelComponent)
};
#endif /* LevelComponent_h */
