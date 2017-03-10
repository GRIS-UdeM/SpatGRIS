//
//  Speaker.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-02.
//
//

#include "Speaker.h"
#include "MainComponent.h"

Speaker::Speaker(MainContentComponent *parent, int idS){
    Speaker(parent, idS, idS, glm::vec3(0,0,0));
}

Speaker::Speaker(MainContentComponent *parent, int idS,int outP, glm::vec3 center, glm::vec3 extents) {
    this->mainParent = parent;
    this->idSpeaker = idS;
    this->outputPatch = outP;
    LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);
    
    //Load position
    this->newPosition(center, extents);

    label = new Label();
    label->setText(String(this->idSpeaker), NotificationType::dontSendNotification);
    label->setFont(mGrisFeel.getFont());
    label->setLookAndFeel(&mGrisFeel);
    label->setColour(Label::textColourId, mGrisFeel.getFontColour());
    this->addAndMakeVisible(label);
    
    
    teCenterX = new TextEditor();
    teCenterX->setText(String(this->center.x/10.0f), NotificationType::dontSendNotification);
    teCenterX->addListener(this);
    this->addAndMakeVisible(teCenterX);
    
    teCenterZ = new TextEditor();
    teCenterZ->setText(String(this->center.y/10.0f), NotificationType::dontSendNotification);
    teCenterZ->addListener(this);
    this->addAndMakeVisible(teCenterZ);
    
    teCenterY = new TextEditor();
    teCenterY->setText(String(this->center.z/10.0f), NotificationType::dontSendNotification);
    teCenterY->addListener(this);
    this->addAndMakeVisible(teCenterY);
    
    
    teAzimuth = new TextEditor();
    teAzimuth->setText(String(this->aziZenRad.x), NotificationType::dontSendNotification);
    teAzimuth->addListener(this);
    this->addAndMakeVisible(teAzimuth);
    
    teZenith = new TextEditor();
    teZenith->setText(String(this->aziZenRad.y), NotificationType::dontSendNotification);
    teZenith->addListener(this);
    this->addAndMakeVisible(teZenith);
    
    teRadius = new TextEditor();
    teRadius->setText(String(this->aziZenRad.z/10.0f), NotificationType::dontSendNotification);
    teRadius->addListener(this);
    this->addAndMakeVisible(teRadius);
    
    teOutputPatch = new TextEditor();
    teOutputPatch->setText(String(this->outputPatch), NotificationType::dontSendNotification);
    teOutputPatch->addListener(this);
    this->addAndMakeVisible(teOutputPatch);

    

}

Speaker::~Speaker(){
    delete label;
    delete teCenterX;
    delete teCenterY;
    delete teCenterZ;
    
    delete teAzimuth;
    delete teZenith;
    delete teRadius;
    
    delete teOutputPatch;
}


void Speaker::setBounds(const Rectangle<int> &newBounds){
    this->juce::Component::setBounds(newBounds);
}

glm::vec3 Speaker::getCoordinate(){
    return this->center /10.0f ;
}
glm::vec3 Speaker::getAziZenRad(){
    return glm::vec3(this->aziZenRad.x, this->aziZenRad.y, this->aziZenRad.z/10.0f);
}

void Speaker::focusOfChildComponentChanged (FocusChangeType cause){
    this->selectSpeaker();
}
void Speaker::focusLost (FocusChangeType cause){
    this->unSelectSpeaker();
}

void Speaker::textEditorFocusLost (TextEditor &textEditor) {
    textEditorReturnKeyPressed(textEditor);
}

void Speaker::textEditorReturnKeyPressed (TextEditor &textEditor) {
    
    if (&textEditor == teCenterX) {
        glm::vec3 newCenter = this->center;
        newCenter.x = teCenterX->getText().getFloatValue()*10.0f;
        this->newPosition(newCenter);
    }else if(&textEditor == teCenterZ) {
        glm::vec3 newCenter = this->center;
        newCenter.y = teCenterZ->getText().getFloatValue()*10.0f;
        this->newPosition(newCenter);
    }
    else if(&textEditor == teCenterY) {
        glm::vec3 newCenter = this->center;
        newCenter.z = teCenterY->getText().getFloatValue()*10.0f;
        this->newPosition(newCenter);
    }
    
    else if(&textEditor == teAzimuth) {
        glm::vec3 newAziZenRad = this->aziZenRad;
        newAziZenRad.x = teAzimuth->getText().getFloatValue();
        this->newSpheriqueCoord(newAziZenRad);
    }
    else if(&textEditor == teZenith) {
        glm::vec3 newAziZenRad = this->aziZenRad;
        newAziZenRad.y = teZenith->getText().getFloatValue();
        this->newSpheriqueCoord(newAziZenRad);
    }
    else if(&textEditor == teRadius) {
        glm::vec3 newAziZenRad = this->aziZenRad;
        newAziZenRad.z = teRadius->getText().getFloatValue()*10.0f;
        this->newSpheriqueCoord(newAziZenRad);
    }
    
    else if(&textEditor == teOutputPatch) {
       this->outputPatch = teOutputPatch->getText().getIntValue();
    }

    teCenterX->setText(String(this->center.x/10.0f), NotificationType::dontSendNotification);
    teCenterZ->setText(String(this->center.y/10.0f), NotificationType::dontSendNotification);
    teCenterY->setText(String(this->center.z/10.0f), NotificationType::dontSendNotification);

    teAzimuth->setText(String(this->aziZenRad.x), NotificationType::dontSendNotification);
    teZenith->setText(String(this->aziZenRad.y), NotificationType::dontSendNotification);
    teRadius->setText(String(this->aziZenRad.z/10.0f), NotificationType::dontSendNotification);

}
void Speaker::paint (Graphics& g)
{
    label->setBounds(2, 2, 20, getHeight());
    teCenterX->setBounds(22, 2, 66, 22);
    teCenterY->setBounds(90, 2, 66, 22);
    teCenterZ->setBounds(158, 2, 66, 22);
    
    teAzimuth->setBounds(230, 2, 66, 22);
    teZenith->setBounds(298, 2, 66, 22);
    teRadius->setBounds(366, 2, 66, 22);
    
    teOutputPatch->setBounds(440, 2, 26, 22);
    
    if(this->selected){
        Colour c = mGrisFeel.getOnColour();
        g.setColour (c.withMultipliedAlpha (0.3f));
        g.fillAll ();
        g.setColour (c);
        g.drawRect (0,0,getWidth(),getHeight(),1);
    }
}

glm::vec3 Speaker::getMin() {
    return this->min;
}

glm::vec3 Speaker::getMax() {
    return this->max;
}

glm::vec3 Speaker::getCenter(){
    return this->center;
}

bool Speaker::isValid() {
    return (this->min.x < this->max.x && this->min.y < this->max.y && this->min.z < this->max.z);
}

void Speaker::fix() {
    glm::vec3 _max = (this->max);
    //change new "min" to previous max
    if (this->min.x > this->max.x) {
        this->max.x = this->min.x;
        this->min.x = _max.x;
    }
    if (this->min.y > this->max.y) {
        this->max.y = this->min.y;
        this->min.y = _max.y;
    }
    if (this->min.z > this->max.z) {
        this->max.z = this->min.z;
        this->min.z = _max.z;
    }
}

bool Speaker::isSelected(){
    return this->selected;
}

void Speaker::selectSpeaker()
{
    this->mainParent->getLockSpeakers()->try_lock();
    for(int i = 0; i < this->mainParent->getListSpeaker().size(); ++i) {
        this->mainParent->getListSpeaker()[i]->unSelectSpeaker();
    }
    this->mainParent->getLockSpeakers()->unlock();
    
    this->color = colorSpeakerSelect;
    this->selected = true;
    this->repaint();
}

void Speaker::unSelectSpeaker()
{
    this->color = colorSpeaker;
    this->selected = false;
    this->repaint();
}


void Speaker::newPosition(glm::vec3 center, glm::vec3 extents)
{
    //min == center - extents, max == c+e
    this->min.x = center.x - extents.x;
    this->min.y = center.y - extents.y;
    this->min.z = center.z - extents.z;
    
    this->max.x = center.x + extents.x;
    this->max.y = center.y + extents.y;
    this->max.z = center.z + extents.z;
    if (!this->isValid()) {
        this->fix();
    }
    this->center = glm::vec3(this->min.x+(this->max.x - this->min.x) / 2.0f, this->min.y+(this->max.y - this->min.y) / 2.0f, this->min.z+(this->max.z - this->min.z) / 2.0f);
    
    float azimuth = ( (atan2(this->center.x, this->center.z) * 180.0f) / M_PI) +90.0f;
    float zenith = ( atan2(this->center.y, sqrt(this->center.x*this->center.x + this->center.z*this->center.z)) * 180.0f) / M_PI;
    float radius = sqrt((this->center.x*this->center.x)+(this->center.y*this->center.y)+(this->center.z*this->center.z));
    azimuth = 180.0f-azimuth;
    if(azimuth < 0.0f){
        azimuth+=360.0f;
    }
    if(zenith < 0.0f){
        zenith+=360.0f;
    }
    if(zenith >= 360.0f){
        zenith=0.0f;
    }
    this->aziZenRad = glm::vec3(azimuth, zenith, radius);
}

void Speaker::newSpheriqueCoord(glm::vec3 aziZenRad, glm::vec3 extents){
    glm::vec3 nCenter;

    aziZenRad.x = abs( (aziZenRad.x * M_PI ) / 180.0f) ;
    aziZenRad.y = abs( ((-90.0f+aziZenRad.y) * M_PI ) / 180.0f);
    
    nCenter.x = GetFloatPrecision(aziZenRad.z * sinf(aziZenRad.y)*cosf(aziZenRad.x), 6);
    nCenter.z = GetFloatPrecision(aziZenRad.z * sinf(aziZenRad.y)*sinf(aziZenRad.x), 6);
    nCenter.y = GetFloatPrecision(aziZenRad.z * cosf(aziZenRad.y), 6);
    
    this->newPosition(nCenter);
}

void Speaker::draw() {
    
    glPushMatrix();

    glTranslatef(this->center.x, this->center.y, this->center.z);
    
    glRotatef(180.0f-this->aziZenRad.x, 0, 1.0, 0);
    glRotatef(-this->aziZenRad.y, 0, 0, 1.0);
    //glRotatef(0, 1.0, 0, 0); Z useless
    glTranslatef(-1*this->center.x, -1*this->center.y, -1*this->center.z);
    
    glBegin(GL_QUADS);
    
    glColor3f(this->color.x, this->color.y, this->color.z);
    
    glVertex3f(this->min.x, this->min.y, this->max.z);
    glVertex3f(this->max.x, this->min.y, this->max.z);
    glVertex3f(this->max.x, this->max.y, this->max.z);
    glVertex3f(this->min.x, this->max.y, this->max.z);
    
    glVertex3f(this->max.x, this->min.y, this->max.z);
    glVertex3f(this->max.x, this->min.y, this->min.z);
    glVertex3f(this->max.x, this->max.y, this->min.z);
    glVertex3f(this->max.x, this->max.y, this->max.z);
    
    glVertex3f(this->min.x, this->max.y, this->max.z);
    glVertex3f(this->max.x, this->max.y, this->max.z);
    glVertex3f(this->max.x, this->max.y, this->min.z);
    glVertex3f(this->min.x, this->max.y, this->min.z);
    
    glVertex3f(this->min.x, this->min.y, this->min.z);
    glVertex3f(this->min.x, this->max.y, this->min.z);
    glVertex3f(this->max.x, this->max.y, this->min.z);
    glVertex3f(this->max.x, this->min.y, this->min.z);
    
    glVertex3f(this->min.x, this->min.y, this->min.z);
    glVertex3f(this->max.x, this->min.y, this->min.z);
    glVertex3f(this->max.x, this->min.y, this->max.z);
    glVertex3f(this->min.x, this->min.y, this->max.z);
    
    glVertex3f(this->min.x, this->min.y, this->min.z);
    glVertex3f(this->min.x, this->min.y, this->max.z);
    glVertex3f(this->min.x, this->max.y, this->max.z);
    glVertex3f(this->min.x, this->max.y, this->min.z);
    
    glEnd();
    
    glLineWidth(2);
    glBegin(GL_LINES);
    
    glColor3f(0, 0, 0);
    glVertex3f(this->center.x+sizeSpeaker.x,this->center.y, this->center.z);
    glVertex3f(this->center.x+ 1.2f,this->center.y, this->center.z);
    glEnd();
    
    if(this->selected){
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // this tells it to only render lines
        
        glLineWidth(4);
        glBegin(GL_LINES);
        float over = 0.02f;
        
        glVertex3f(this->min.x-over, this->min.y-over, this->min.z-over);
        glVertex3f(this->min.x-over, this->min.y-over, this->max.z+over);
        
        glVertex3f(this->max.x+over, this->min.y-over, this->min.z-over);
        glVertex3f(this->max.x+over, this->min.y-over, this->max.z+over);
        
        glVertex3f(this->max.x+over, this->max.y+over, this->min.z-over);
        glVertex3f(this->max.x+over, this->max.y+over, this->max.z+over);
        
        glVertex3f(this->min.x-over, this->max.y+over, this->min.z-over);
        glVertex3f(this->min.x-over, this->max.y+over, this->max.z+over);
        

        glVertex3f(this->min.x-over, this->min.y-over, this->min.z-over);
        glVertex3f(this->max.x+over, this->min.y-over, this->min.z-over);
        
        glVertex3f(this->min.x-over, this->min.y-over, this->max.z+over);
        glVertex3f(this->max.x+over, this->min.y-over, this->max.z+over);
        
        glVertex3f(this->min.x-over, this->max.y+over, this->min.z-over);
        glVertex3f(this->max.x+over, this->max.y+over, this->min.z-over);
        
        glVertex3f(this->min.x-over, this->max.y+over, this->max.z+over);
        glVertex3f(this->max.x+over, this->max.y+over, this->max.z+over);
        
        
        glVertex3f(this->min.x-over, this->min.y-over, this->min.z-over);
        glVertex3f(this->min.x-over, this->max.y+over, this->min.z-over);
        
        glVertex3f(this->min.x-over, this->min.y-over, this->max.z+over);
        glVertex3f(this->min.x-over, this->max.y+over, this->max.z+over);
        
        glVertex3f(this->max.x+over, this->min.y-over, this->min.z-over);
        glVertex3f(this->max.x+over, this->max.y+over, this->min.z-over);
        
        glVertex3f(this->max.x+over, this->min.y-over, this->max.z+over);
        glVertex3f(this->max.x+over, this->max.y+over, this->max.z+over);

        
        glEnd();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glPopMatrix();
}
