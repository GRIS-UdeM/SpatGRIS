//
//  UiComponent.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-13.
//
//

#include "UiComponent.h"
#include "LevelComponent.h"
#include "MainComponent.h"

//======================================= BOX ===========================================================================
Box::Box(GrisLookAndFeel *feel, String title) {
    
    this->title = title;
    this->grisFeel = feel;
    this->bgColour = this->grisFeel->getBackgroundColour();
    
    this->content = new Component();
    this->viewport = new Viewport();
    this->viewport->setViewedComponent(this->content, false);
    this->viewport->setScrollBarsShown(true, true);
    this->viewport->setScrollBarThickness(6);
    this->viewport->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    this->viewport->getHorizontalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    
    this->viewport->setLookAndFeel(this->grisFeel);
    addAndMakeVisible(this->viewport);
}

Box::~Box(){
    this->content->deleteAllChildren();
    delete this->viewport;
    delete this->content;
}

Component * Box::getContent() {
    return this->content ? this->content : this;
}


void Box::resized() {
    if (this->viewport){
        this->viewport->setSize(getWidth(), getHeight());
    }
}

void Box::correctSize(int width, int height){
    if(this->title!=""){
        this->viewport->setTopLeftPosition(0, 20);
        this->viewport->setSize(getWidth(), getHeight()-20);
        if(width<80){
            width = 80;
        }
    }else{
        this->viewport->setTopLeftPosition(0, 0);
    }
    this->getContent()->setSize(width, height);
}


void Box::paint(Graphics &g) {
    g.setColour(this->bgColour);
    g.fillRect(getLocalBounds());
    if(this->title!=""){
        g.setColour (this->grisFeel->getWinBackgroundColour());
        g.fillRect(0,0,getWidth(),18);
        
        g.setColour (this->grisFeel->getFontColour());
        g.drawText(title, 0, 0, this->content->getWidth(), 20, juce::Justification::left);
    }
}






//======================================= Window Edit Speaker============================================================

WindowEditSpeaker::WindowEditSpeaker(const String& name, Colour backgroundColour, int buttonsNeeded,
                                     MainContentComponent * parent, GrisLookAndFeel * feel):
    DocumentWindow (name, backgroundColour, buttonsNeeded), font (14.0f)
{
    this->mainParent = parent;
    this->grisFeel = feel;
    
}
WindowEditSpeaker::~WindowEditSpeaker(){
    this->mainParent->setShowShepre(false);
    //delete this->boxListSpeaker;
    delete this->toggleShowSphere;
    delete this->butAddSpeaker;
    this->mainParent->destroyWinSpeakConf();
}
void WindowEditSpeaker::initComp(){
    this->boxListSpeaker = new Box(this->grisFeel, "Configuration Speakers");
    this->setContentComponent(this->boxListSpeaker);
    this->boxListSpeaker->getContent()->addAndMakeVisible(tableListSpeakers);
    
    tableListSpeakers.setModel(this);
    
    tableListSpeakers.setColour (ListBox::outlineColourId, this->grisFeel->getWinBackgroundColour());
    tableListSpeakers.setColour(ListBox::backgroundColourId, this->grisFeel->getWinBackgroundColour());
    tableListSpeakers.setOutlineThickness (1);
    
    tableListSpeakers.getHeader().addColumn("ID", 1, 40, 40, 60,TableHeaderComponent::defaultFlags);
    
    tableListSpeakers.getHeader().addColumn("X", 2, 70, 50, 120,TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Y", 3, 70, 50, 120,TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Z", 4, 70, 50, 120,TableHeaderComponent::defaultFlags);
    
    tableListSpeakers.getHeader().addColumn("Azimuth", 5, 70, 50, 120,TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Zenith", 6, 70, 50, 120,TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Radius", 7, 70, 50, 120,TableHeaderComponent::defaultFlags);
    
    tableListSpeakers.getHeader().addColumn("Output", 8, 40, 40, 60,TableHeaderComponent::defaultFlags);
    
    tableListSpeakers.getHeader().addColumn("delete", 9, 40, 40, 60,TableHeaderComponent::defaultFlags);
    
    tableListSpeakers.getHeader().setSortColumnId (1, true); // sort forwards by the ID column
    //tableListSpeakers.getHeader().setColumnVisible (7, false);
    
    tableListSpeakers.setMultipleSelectionEnabled (false);
    
    numRows =this->mainParent->getListSpeaker().size();


    
    this->boxListSpeaker->setBounds(0, 0, getWidth(),getHeight());
    this->boxListSpeaker->correctSize(getWidth()-8, getHeight());
    tableListSpeakers.setSize(getWidth(), 400);
    
    tableListSpeakers.updateContent();
    
    this->toggleShowSphere = new ToggleButton();
    this->toggleShowSphere->setButtonText("Show Sphere");
    this->toggleShowSphere->setBounds(4, 430, 88, 22);
    this->toggleShowSphere->addListener(this);
    this->toggleShowSphere->setToggleState(false, dontSendNotification);
    this->toggleShowSphere->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->toggleShowSphere->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->toggleShowSphere);
    
    
    this->butAddSpeaker = new TextButton();
    this->butAddSpeaker->setButtonText("Add Speaker");
    this->butAddSpeaker->setBounds(4, 404, 88, 22);
    this->butAddSpeaker->addListener(this);
    this->butAddSpeaker->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butAddSpeaker->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butAddSpeaker);
    
    
    
    this->boxListSpeaker->repaint();
    this->boxListSpeaker->resized();
}

void WindowEditSpeaker::buttonClicked(Button *button){
    
    if (button == this->toggleShowSphere) {
        this->mainParent->setShowShepre(this->toggleShowSphere->getToggleState());
    }
    else if (button == this->butAddSpeaker) {
        this->mainParent->addSpeaker();
        updateWinContent();
    }
    
    if( button->getName() != "" && (button->getName().getIntValue()>=0 && button->getName().getIntValue()<= this->mainParent->getListSpeaker().size())){

        this->mainParent->removeSpeaker(button->getName().getIntValue());
        updateWinContent();
    }
    
}

void WindowEditSpeaker::updateWinContent(){
    numRows =this->mainParent->getListSpeaker().size();
    tableListSpeakers.updateContent();
}
void WindowEditSpeaker::closeButtonPressed()
{
    delete this;
}



String WindowEditSpeaker::getText (const int columnNumber, const int rowNumber) const
{
    String text = "";
    //this->mainParent->getLockSpeakers()->lock();
    if (this->mainParent->getListSpeaker().size()> rowNumber)
    {
        
        switch(columnNumber){
            case 1 :
                text =String(this->mainParent->getListSpeaker()[rowNumber]->getIdSpeaker());
                break;
                
            case 2 :
                text =String(this->mainParent->getListSpeaker()[rowNumber]->getCoordinate().x);
                break;
            case 3 :
                text =String(this->mainParent->getListSpeaker()[rowNumber]->getCoordinate().z);
                break;
            case 4 :
                text =String(this->mainParent->getListSpeaker()[rowNumber]->getCoordinate().y);
                break;
                
            case 5 :
                text =String(this->mainParent->getListSpeaker()[rowNumber]->getAziZenRad().x);
                break;
            case 6 :
                text =String(this->mainParent->getListSpeaker()[rowNumber]->getAziZenRad().y);
                break;
            case 7 :
                text =String(this->mainParent->getListSpeaker()[rowNumber]->getAziZenRad().z);
                break;
                
            case 8 :
                text =String(this->mainParent->getListSpeaker()[rowNumber]->getOutputPatch());
                break;
            default:
                text ="?";
        }
    }

    //this->mainParent->getLockSpeakers()->unlock();
    return text;
}

void WindowEditSpeaker::setText (const int columnNumber, const int rowNumber, const String& newText)
{
    this->mainParent->getLockSpeakers()->lock();
    if (this->mainParent->getListSpeaker().size()> rowNumber)
    {
        glm::vec3 newP;
        switch(columnNumber){
                
            case 2 :
                newP = this->mainParent->getListSpeaker()[rowNumber]->getCoordinate();
                newP.x = newText.getFloatValue();
                this->mainParent->getListSpeaker()[rowNumber]->setCoordinate(newP);

                break;
            case 3 :
                newP = this->mainParent->getListSpeaker()[rowNumber]->getCoordinate();
                newP.z = newText.getFloatValue();
                this->mainParent->getListSpeaker()[rowNumber]->setCoordinate(newP);
                break;
            case 4 :
                newP = this->mainParent->getListSpeaker()[rowNumber]->getCoordinate();
                newP.y = newText.getFloatValue();
                this->mainParent->getListSpeaker()[rowNumber]->setCoordinate(newP);
                break;
                
            case 5 :
                newP = this->mainParent->getListSpeaker()[rowNumber]->getAziZenRad();
                newP.x = newText.getFloatValue();
                this->mainParent->getListSpeaker()[rowNumber]->setAziZenRad(newP);
                break;
            case 6 :
                newP = this->mainParent->getListSpeaker()[rowNumber]->getAziZenRad();
                newP.y = newText.getFloatValue();
                this->mainParent->getListSpeaker()[rowNumber]->setAziZenRad(newP);
                break;
            case 7 :
                newP = this->mainParent->getListSpeaker()[rowNumber]->getAziZenRad();
                newP.z = newText.getFloatValue();
                this->mainParent->getListSpeaker()[rowNumber]->setAziZenRad(newP);
                break;
                
            case 8 :
                this->mainParent->getListSpeaker()[rowNumber]->setOutputPatch(newText.getIntValue());
                this->mainParent->updateLevelComp();
                break;
                
        }
    }
    
    this->mainParent->getLockSpeakers()->unlock();
}

int WindowEditSpeaker::getNumRows()
{
    return numRows;
}

// This is overloaded from TableListBoxModel, and should fill in the background of the whole row
void WindowEditSpeaker::paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected){
        this->mainParent->getLockSpeakers()->lock();
        this->mainParent->getListSpeaker()[rowNumber]->selectSpeaker();
        this->mainParent->getLockSpeakers()->unlock();
        g.fillAll (this->grisFeel->getOnColour());
    }
    else{
        this->mainParent->getLockSpeakers()->lock();
        this->mainParent->getListSpeaker()[rowNumber]->unSelectSpeaker();
        this->mainParent->getLockSpeakers()->unlock();
        if (rowNumber % 2){
            g.fillAll (this->grisFeel->getBackgroundColour().withBrightness(0.6));
        }else{
            g.fillAll (this->grisFeel->getBackgroundColour().withBrightness(0.7));
        }
    }
}

// This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
// components.
void WindowEditSpeaker::paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    g.setColour (Colours::black);
    g.setFont (font);
    
    this->mainParent->getLockSpeakers()->lock();
    if (this->mainParent->getListSpeaker().size()> rowNumber)
    {
        String text = getText(columnId, rowNumber);
        g.drawText (text, 2, 0, width - 4, height, Justification::centredLeft, true);
        
    }
    this->mainParent->getLockSpeakers()->unlock();
    g.setColour (Colours::black.withAlpha (0.2f));
    g.fillRect (width - 1, 0, 1, height);
}

Component* WindowEditSpeaker::refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
                                    Component* existingComponentToUpdate)
{
    if(columnId==9){
        TextButton* tbRemove = static_cast<TextButton*> (existingComponentToUpdate);
        if (tbRemove == nullptr)
            tbRemove = new TextButton ();
        tbRemove->setButtonText("X");
        tbRemove->setName(String(rowNumber));
        tbRemove->setBounds(4, 404, 88, 22);
        tbRemove->addListener(this);
        tbRemove->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
        tbRemove->setLookAndFeel(this->grisFeel);
        //tbRemove->getContent()->addAndMakeVisible(this->butAddSpeaker);
        return tbRemove;
    }
    // The other columns are editable text columns, for which we use the custom Label component
    EditableTextCustomComponent* textLabel = static_cast<EditableTextCustomComponent*> (existingComponentToUpdate);
    
    // same as above...
    if (textLabel == nullptr)
        textLabel = new EditableTextCustomComponent (*this);
    
    textLabel->setRowAndColumn (rowNumber, columnId);
    if(columnId==1){    //ID Speakers
        textLabel->setEditable(false);
    }
    return textLabel;
}

