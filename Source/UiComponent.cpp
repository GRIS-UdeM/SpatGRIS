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

void Box::correctSize(unsigned int width, unsigned int height){
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




//======================================= BOX CLIENT ===========================================================================
BoxClient::BoxClient(MainContentComponent * parent, GrisLookAndFeel *feel){
    this->mainParent= parent;
    this->grisFeel = feel;
    
    tableListClient.setModel(this);
    
    tableListClient.setColour (ListBox::outlineColourId, this->grisFeel->getWinBackgroundColour());
    tableListClient.setColour(ListBox::backgroundColourId, this->grisFeel->getWinBackgroundColour());
    tableListClient.setOutlineThickness (1);
    
    tableListClient.getHeader().addColumn("Client",   1, 80, 70, 120,TableHeaderComponent::notSortable);
    
    tableListClient.getHeader().addColumn("Start",  2, 40, 35, 70,TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("End",    3, 40, 35, 70,TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("Available", 4, 50, 35, 70,TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("On/Off", 5, 45, 35, 70,TableHeaderComponent::notSortable);


    tableListClient.setMultipleSelectionEnabled (false);
    
    numRows = 0;
    tableListClient.updateContent();
    
    this->addAndMakeVisible(tableListClient);
}
BoxClient::~BoxClient(){

}

void BoxClient::buttonClicked(Button *button){
    bool connectedCli = !this->mainParent->getListClientjack()->at(button->getName().getIntValue()).connected;
    this->mainParent->connectionClientJack(this->mainParent->getListClientjack()->at(button->getName().getIntValue()).name, connectedCli);
    updateContentCli();
}

void BoxClient::setBounds(int x, int y, int width, int height)
{
    this->juce::Component::setBounds(x, y, width, height);
    tableListClient.setSize(width, height);
}

void BoxClient::updateContentCli(){
    numRows = (unsigned int)this->mainParent->getListClientjack()->size();
    tableListClient.updateContent();
    tableListClient.repaint();
}

void BoxClient::setValue (const int rowNumber, const int columnNumber,const int newRating)
{
    if (this->mainParent->getListClientjack()->size()> rowNumber)
    {
        switch(columnNumber){
            case 2 :
                this->mainParent->getListClientjack()->at(rowNumber).portStart = newRating;
                break;
            case 3 :
                this->mainParent->getListClientjack()->at(rowNumber).portEnd= newRating;
                break;
        }
    }
}

int BoxClient::getValue (const int rowNumber,const int columnNumber) const{
    if (this->mainParent->getListClientjack()->size()> rowNumber)
    {
        switch(columnNumber){
                
            case 2 :
                return this->mainParent->getListClientjack()->at(rowNumber).portStart;
                break;
            case 3 :
                return this->mainParent->getListClientjack()->at(rowNumber).portEnd;
                break;
                
        }
    }
    return -1;
}

String BoxClient::getText (const int columnNumber, const int rowNumber) const
{
    String text;
    if (this->mainParent->getListClientjack()->size()> rowNumber)
    {
        
        switch(columnNumber){
            case 1 :
                text =String(this->mainParent->getListClientjack()->at(rowNumber).name);
                break;
            case 4 :
                text =String(this->mainParent->getListClientjack()->at(rowNumber).portAvailable);
                break;
            
            default:
                text ="?";
        }
    }
    return text;
}




int BoxClient::getNumRows()
{
    return numRows;
}

void BoxClient::paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    /*if (rowIsSelected){
        g.fillAll (this->grisFeel->getOnColour());
    }
    else{*/
        if (rowNumber % 2){
            g.fillAll (this->grisFeel->getBackgroundColour().withBrightness(0.6));
        }else{
            g.fillAll (this->grisFeel->getBackgroundColour().withBrightness(0.7));
        }
    //}
}


void BoxClient::paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    g.setColour (Colours::black);
    g.setFont (12.0f);
    
    if (this->mainParent->getListClientjack()->size()> rowNumber)
    {
        if(columnId==1){
            String text = getText(columnId, rowNumber);
            g.drawText (text, 2, 0, width - 4, height, Justification::centredLeft, true);
        }
        if(columnId==4){
            String text = getText(columnId, rowNumber);
            g.drawText (text, 2, 0, width - 4, height, Justification::centred, true);
        }
    }

    g.setColour (Colours::black.withAlpha (0.2f));
    g.fillRect (width - 1, 0, 1, height);
}

Component* BoxClient::refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
                                                       Component* existingComponentToUpdate)
{
     if(columnId==1|| columnId==4){
         return existingComponentToUpdate;
     }
    
    if(columnId==5){
        TextButton* tbRemove = static_cast<TextButton*> (existingComponentToUpdate);
        if (tbRemove == nullptr){
            tbRemove = new TextButton ();
            tbRemove->setName(String(rowNumber));
            tbRemove->setBounds(4, 404, 88, 22);
            tbRemove->addListener(this);
            tbRemove->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
            tbRemove->setLookAndFeel(this->grisFeel);
        }
        if(this->mainParent->getListClientjack()->at(rowNumber).connected){
            tbRemove->setButtonText("<->");
        }else{
            tbRemove->setButtonText("<X>");
        }
        
                //tbRemove->getContent()->addAndMakeVisible(this->butAddSpeaker);
        return tbRemove;
    }

    ListIntOutComp* textLabel = static_cast<ListIntOutComp*> (existingComponentToUpdate);
    
    // same as above...
    if (textLabel == nullptr)
        textLabel = new ListIntOutComp (*this);
    
    textLabel->setRowAndColumn (rowNumber, columnId);
    
    return textLabel;
}



//======================================= Window Edit Speaker============================================================

WindowEditSpeaker::WindowEditSpeaker(const String& name, String& nameC,Colour backgroundColour, int buttonsNeeded,
                                     MainContentComponent * parent, GrisLookAndFeel * feel):
    DocumentWindow (name, backgroundColour, buttonsNeeded), font (14.0f)
{
    this->mainParent = parent;
    this->grisFeel = feel;
    
    this->boxListSpeaker = new Box(this->grisFeel, "Configuration Speakers");
    
    this->butAddSpeaker = new TextButton();
    this->butAddSpeaker->setButtonText("Add Speaker");
    this->butAddSpeaker->setBounds(4, 404, 88, 22);
    this->butAddSpeaker->addListener(this);
    this->butAddSpeaker->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butAddSpeaker->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butAddSpeaker);
    
    
    this->toggleShowSphere = new ToggleButton();
    this->toggleShowSphere->setButtonText("Show Sphere");
    this->toggleShowSphere->setBounds(4, 430, 120, 22);
    this->toggleShowSphere->addListener(this);
    this->toggleShowSphere->setToggleState(false, dontSendNotification);
    this->toggleShowSphere->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->toggleShowSphere->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->toggleShowSphere);
    
    
    this->butsaveSpeakers = new TextButton();
    this->butsaveSpeakers->setButtonText("Save");
    this->butsaveSpeakers->setBounds(100, 404, 88, 22);
    this->butsaveSpeakers->addListener(this);
    this->butsaveSpeakers->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butsaveSpeakers->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butsaveSpeakers);
    
    this->texEditNameConf = new TextEditor();
    this->texEditNameConf->setText(nameC);
    this->texEditNameConf->setBounds(190, 404, 160, 22);
    this->texEditNameConf->addListener(this);
    this->texEditNameConf->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->texEditNameConf->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->texEditNameConf);

    
    
    this->setContentComponent(this->boxListSpeaker);
    this->boxListSpeaker->getContent()->addAndMakeVisible(tableListSpeakers);

    this->boxListSpeaker->repaint();
    this->boxListSpeaker->resized();
    
}
WindowEditSpeaker::~WindowEditSpeaker(){
    this->mainParent->setShowShepre(false);
    //delete this->boxListSpeaker;
    delete this->toggleShowSphere;
    delete this->butAddSpeaker;
    delete this->butsaveSpeakers;
    delete this->texEditNameConf;
    this->mainParent->destroyWinSpeakConf();
}
void WindowEditSpeaker::initComp(){

    
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
    
    numRows = (unsigned int)this->mainParent->getListSpeaker().size();


    this->boxListSpeaker->setBounds(0, 0, getWidth(),getHeight());
    this->boxListSpeaker->correctSize(getWidth()-8, getHeight());
    tableListSpeakers.setSize(getWidth(), 400);
    
    tableListSpeakers.updateContent();
    
    
    

    
    this->boxListSpeaker->repaint();
    this->boxListSpeaker->resized();
    this->resized();
}

void WindowEditSpeaker::buttonClicked(Button *button){
    
    if (button == this->toggleShowSphere) {
        this->mainParent->setShowShepre(this->toggleShowSphere->getToggleState());
    }
    else if (button == this->butAddSpeaker) {
        this->mainParent->addSpeaker();
        updateWinContent();
    }
    else if (button == this->butsaveSpeakers) {
        FileChooser fc ("Choose a file to save...",File::getCurrentWorkingDirectory(), "*.xml", true);
        if (fc.browseForFileToSave (true))
        {
            String chosen = fc.getResults().getReference(0).getFullPathName();
            bool r = AlertWindow::showOkCancelBox (AlertWindow::InfoIcon,"Save preset","Save to : " + chosen);
            //Save preset speaker
            if(r){
                this->mainParent->savePresetSpeakers(chosen);
            }
        }
    }
    
    if( button->getName() != "" && (button->getName().getIntValue()>=0 && button->getName().getIntValue()<= this->mainParent->getListSpeaker().size())){

        this->mainParent->removeSpeaker(button->getName().getIntValue());
        updateWinContent();
    }
    
}

void WindowEditSpeaker::updateWinContent(){
    numRows = (unsigned int)this->mainParent->getListSpeaker().size();
    tableListSpeakers.updateContent();
}
void WindowEditSpeaker::selectedRow(int value){
    MessageManagerLock mmlock;
    this->tableListSpeakers.selectRow(value);
    this->repaint();
}
void WindowEditSpeaker::closeButtonPressed()
{
    delete this;
}

void WindowEditSpeaker::textEditorFocusLost (TextEditor &textEditor){
    textEditorReturnKeyPressed(textEditor);
}
void WindowEditSpeaker::textEditorReturnKeyPressed (TextEditor &textEditor){
    if(&textEditor == this->texEditNameConf){
        this->mainParent->setNameConfig(this->texEditNameConf->getTextValue().toString());
    }
}

void WindowEditSpeaker::resized(){
    this->juce::DocumentWindow::resized();
    
    tableListSpeakers.setSize(getWidth(), getHeight()-120);
    
    this->boxListSpeaker->setSize(getWidth(), getHeight());
    this->boxListSpeaker->correctSize(getWidth()-10, getHeight()-30);

    this->butAddSpeaker->setBounds(4, getHeight()-110, 88, 22);
    this->toggleShowSphere->setBounds(4, getHeight()-86, 120, 22);
    this->butsaveSpeakers->setBounds(100, getHeight()-110, 88, 22);
    this->texEditNameConf->setBounds(190, getHeight()-110, 160, 22);
    
    //this->boxListSpeaker->correctSize((this->listSourceInput.size()*(SizeWidthLevelComp))+4, 210);
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




//======================================= WinJackSettings ===========================

WindowJackSetting::WindowJackSetting(const String& name, Colour backgroundColour, int buttonsNeeded,MainContentComponent * parent, GrisLookAndFeel * feel, int indR, int indB):
    DocumentWindow (name, backgroundColour, buttonsNeeded)
{
    this->mainParent = parent;
    this->grisFeel = feel;
    
    this->labRate = new Label();
    this->labRate->setText("Rate (Hz) :", NotificationType::dontSendNotification);
    this->labRate->setJustificationType(Justification::right);
    this->labRate->setBounds(10, 20, 80, 22);
    this->labRate->setFont(this->grisFeel->getFont());
    this->labRate->setLookAndFeel(this->grisFeel);
    this->labRate->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->juce::Component::addAndMakeVisible(this->labRate);
    
    this->labBuff = new Label();
    this->labBuff->setText("Buffer (spls) :", NotificationType::dontSendNotification);
    this->labBuff->setJustificationType(Justification::right);
    this->labBuff->setBounds(10, 50, 80, 22);
    this->labBuff->setFont(this->grisFeel->getFont());
    this->labBuff->setLookAndFeel(this->grisFeel);
    this->labBuff->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->juce::Component::addAndMakeVisible(this->labBuff);

    
    this->cobRate = new ComboBox();
    this->cobRate->addItemList(RateValues, 1);
    this->cobRate->setSelectedItemIndex(indR);
    this->cobRate->setBounds(90, 20, 120, 22);
    this->cobRate->setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->cobRate);
    
    this->cobBuffer = new ComboBox();
    this->cobBuffer->addItemList(BufferSize, 1);
    this->cobBuffer->setSelectedItemIndex(indB);
    this->cobBuffer->setBounds(90, 50, 120, 22);
    this->cobBuffer->setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->cobBuffer);

    
    this->butValidSettings = new TextButton();
    this->butValidSettings->setButtonText("Save");
    this->butValidSettings->setBounds(122, 80, 88, 22);
    this->butValidSettings->addListener(this);
    this->butValidSettings->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butValidSettings->setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->butValidSettings);

}

WindowJackSetting::~WindowJackSetting(){
    delete this->labRate;
    delete this->labBuff;
    delete this->cobRate;
    delete this->cobBuffer;
    delete this->butValidSettings;
    this->mainParent->destroyWinJackSetting();
}

void WindowJackSetting::closeButtonPressed()
{
    delete this;
}


void WindowJackSetting::buttonClicked(Button *button)
{
    if( button == this->butValidSettings){
        this->mainParent->saveJackSettings(this->cobRate->getText().getIntValue(), this->cobBuffer->getText().getIntValue());
        delete this;
    }
}


