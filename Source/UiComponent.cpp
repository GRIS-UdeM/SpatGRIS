/*
 This file is part of ServerGris.
 
 Developers: Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "UiComponent.h"
#include "LevelComponent.h"
#include "MainComponent.h"

//======================================= BOX ===========================================================================
Box::Box(GrisLookAndFeel *feel, String title, bool verticalScrollbar, bool horizontalScrollbar)
{
    this->title = title;
    this->grisFeel = feel;
    this->bgColour = this->grisFeel->getBackgroundColour();
    
    this->content = new Component();
    this->viewport = new Viewport();
    this->viewport->setViewedComponent(this->content, false);
    this->viewport->setScrollBarsShown(verticalScrollbar, horizontalScrollbar);
    this->viewport->setScrollBarThickness(6);
    this->viewport->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    this->viewport->getHorizontalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    
    this->viewport->setLookAndFeel(this->grisFeel);
    addAndMakeVisible(this->viewport);
}

Box::~Box()
{
    this->content->deleteAllChildren();
    delete this->viewport;
    delete this->content;
}

Component * Box::getContent()
{
    return this->content ? this->content : this;
}


void Box::resized()
{
    if (this->viewport){
        this->viewport->setSize(getWidth(), getHeight());
    }
}

void Box::correctSize(unsigned int width, unsigned int height)
{
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


void Box::paint(Graphics &g)
{
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
BoxClient::BoxClient(MainContentComponent * parent, GrisLookAndFeel *feel)
{
    this->mainParent= parent;
    this->grisFeel = feel;
    
    tableListClient.setModel(this);
    
    tableListClient.setColour (ListBox::outlineColourId, this->grisFeel->getWinBackgroundColour());
    tableListClient.setColour(ListBox::backgroundColourId, this->grisFeel->getWinBackgroundColour());
    tableListClient.setOutlineThickness (1);
    
    tableListClient.getHeader().addColumn("Client",    1, 105, 70, 120, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("Start",     2, 45, 35, 70, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("End",       3, 45, 35, 70, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("Available", 4, 62, 35, 70, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("On/Off",    5, 41, 35, 70, TableHeaderComponent::notSortable);


    tableListClient.setMultipleSelectionEnabled (false);
    
    numRows = 0;
    tableListClient.updateContent();
    
    this->addAndMakeVisible(tableListClient);
}

BoxClient::~BoxClient(){

}

void BoxClient::buttonClicked(Button *button)
{
    this->mainParent->getLockClients()->lock();
    bool connectedCli = !this->mainParent->getListClientjack()->at(button->getName().getIntValue()).connected;
    this->mainParent->connectionClientJack(this->mainParent->getListClientjack()->at(button->getName().getIntValue()).name, connectedCli);
    updateContentCli();
    this->mainParent->getLockClients()->unlock();
}

void BoxClient::setBounds(int x, int y, int width, int height)
{
    this->juce::Component::setBounds(x, y, width, height);
    tableListClient.setSize(width, height);
}

void BoxClient::updateContentCli()
{
    numRows = (unsigned int)this->mainParent->getListClientjack()->size();
    tableListClient.updateContent();
    tableListClient.repaint();
}

void BoxClient::setValue (const int rowNumber, const int columnNumber,const int newRating)
{
    this->mainParent->getLockClients()->lock();
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
    this->mainParent->getLockClients()->unlock();
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
    String text = "?";

    if (this->mainParent->getListClientjack()->size()> rowNumber)
    {
        
        switch(columnNumber){
            case 1 :
                text =String(this->mainParent->getListClientjack()->at(rowNumber).name);
                break;
            case 4 :
                text =String(this->mainParent->getListClientjack()->at(rowNumber).portAvailable);
                break;
                
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
    if(this->mainParent->getLockClients()->try_lock()){
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
    this->mainParent->getLockClients()->unlock();
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

WindowEditSpeaker::WindowEditSpeaker(const String& name, String& nameC, Colour backgroundColour, int buttonsNeeded,
                                     MainContentComponent * parent, GrisLookAndFeel * feel):
    DocumentWindow (name, backgroundColour, buttonsNeeded), font (14.0f)
{
    this->mainParent = parent;
    this->grisFeel = feel;
    
    this->boxListSpeaker = new Box(this->grisFeel, "Configuration Speakers");
    
    this->butAddSpeaker = new TextButton();
    this->butAddSpeaker->setButtonText("Add Speaker");
    this->butAddSpeaker->setBounds(5, 404, 100, 22);
    this->butAddSpeaker->addListener(this);
    this->butAddSpeaker->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butAddSpeaker->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butAddSpeaker);

    this->butcompSpeakers = new TextButton();
    this->butcompSpeakers->setButtonText("Compute");
    this->butcompSpeakers->setBounds(110, 404, 100, 22);
    this->butcompSpeakers->addListener(this);
    this->butcompSpeakers->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butcompSpeakers->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butcompSpeakers);
    
    this->butsaveSpeakers = new TextButton();
    this->butsaveSpeakers->setButtonText("Save");
    this->butsaveSpeakers->setBounds(215, 404, 100, 22);
    this->butsaveSpeakers->addListener(this);
    this->butsaveSpeakers->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butsaveSpeakers->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butsaveSpeakers);
    

    this->texEditNameConf = new TextEditor();
    this->texEditNameConf->setText(nameC);
    this->texEditNameConf->setBounds(330, 404, 240, 22);
    this->texEditNameConf->setReadOnly(true);
    this->texEditNameConf->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->texEditNameConf->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->texEditNameConf);

    /* Generate ring of speakers */
    int wlab = 80;

    this->rNumOfSpeakersLabel = new Label();
    this->rNumOfSpeakersLabel->setText("# of speakers", NotificationType::dontSendNotification);
    this->rNumOfSpeakersLabel->setJustificationType(Justification::right);
    this->rNumOfSpeakersLabel->setFont(this->grisFeel->getFont());
    this->rNumOfSpeakersLabel->setLookAndFeel(this->grisFeel);
    this->rNumOfSpeakersLabel->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->rNumOfSpeakersLabel->setBounds(5, 435, 40, 24);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rNumOfSpeakersLabel);

    this->rNumOfSpeakers = new TextEditor();
    this->rNumOfSpeakers->setTooltip("Number of speakers in the ring");
    this->rNumOfSpeakers->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->rNumOfSpeakers->setLookAndFeel(this->grisFeel);
    this->rNumOfSpeakers->setBounds(5+wlab, 435, 40, 24);
    this->rNumOfSpeakers->addListener(this->mainParent);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rNumOfSpeakers);
    this->rNumOfSpeakers->setText("8");
    this->rNumOfSpeakers->setInputRestrictions(3, "0123456789");
    this->rNumOfSpeakers->addListener(this);
    

    this->rZenithLabel = new Label();
    this->rZenithLabel->setText("Zenith", NotificationType::dontSendNotification);
    this->rZenithLabel->setJustificationType(Justification::right);
    this->rZenithLabel->setFont(this->grisFeel->getFont());
    this->rZenithLabel->setLookAndFeel(this->grisFeel);
    this->rZenithLabel->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->rZenithLabel->setBounds(105, 435, 80, 24);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rZenithLabel);

    this->rZenith = new TextEditor();
    this->rZenith->setTooltip("Elevation angle of the ring");
    this->rZenith->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->rZenith->setLookAndFeel(this->grisFeel);
    this->rZenith->setBounds(105+wlab, 435, 60, 24);
    this->rZenith->addListener(this->mainParent);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rZenith);
    this->rZenith->setText("0.0");
    this->rZenith->setInputRestrictions(6, "0123456789.");
    this->rZenith->addListener(this);

    this->rRadiusLabel = new Label();
    this->rRadiusLabel->setText("Radius", NotificationType::dontSendNotification);
    this->rRadiusLabel->setJustificationType(Justification::right);
    this->rRadiusLabel->setFont(this->grisFeel->getFont());
    this->rRadiusLabel->setLookAndFeel(this->grisFeel);
    this->rRadiusLabel->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->rRadiusLabel->setBounds(230, 435, 80, 24);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rRadiusLabel);

    this->rRadius = new TextEditor();
    this->rRadius->setTooltip("Distance of the speakers from the center.");
    this->rRadius->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->rRadius->setLookAndFeel(this->grisFeel);
    this->rRadius->setBounds(230+wlab, 435, 60, 24);
    this->rRadius->addListener(this->mainParent);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rRadius);
    this->rRadius->setText("1.0");
    this->rRadius->setInputRestrictions(6, "0123456789.");
    this->rRadius->addListener(this);

    this->rOffsetAngleLabel = new Label();
    this->rOffsetAngleLabel->setText("Offset Angle", NotificationType::dontSendNotification);
    this->rOffsetAngleLabel->setJustificationType(Justification::right);
    this->rOffsetAngleLabel->setFont(this->grisFeel->getFont());
    this->rOffsetAngleLabel->setLookAndFeel(this->grisFeel);
    this->rOffsetAngleLabel->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->rOffsetAngleLabel->setBounds(375, 435, 80, 24);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rOffsetAngleLabel);

    this->rOffsetAngle = new TextEditor();
    this->rOffsetAngle->setTooltip("Offset angle of the first speaker.");
    this->rOffsetAngle->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->rOffsetAngle->setLookAndFeel(this->grisFeel);
    this->rOffsetAngle->setBounds(375+wlab, 435, 60, 24);
    this->rOffsetAngle->addListener(this->mainParent);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rOffsetAngle);
    this->rOffsetAngle->setText("0.0");
    this->rOffsetAngle->setInputRestrictions(6, "-0123456789.");
    this->rOffsetAngle->addListener(this);

    this->butAddRing = new TextButton();
    this->butAddRing->setButtonText("Add Ring");
    this->butAddRing->setBounds(520, 435, 100, 24);
    this->butAddRing->addListener(this);
    this->butAddRing->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butAddRing->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butAddRing);

    /* End generate ring of speakers */

    this->butClearTriplet = new TextButton();
    this->butClearTriplet->setButtonText("Clear Triplets");
    this->butClearTriplet->setBounds(5, 455, 120, 22);
    this->butClearTriplet->addListener(this);
    this->butClearTriplet->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butClearTriplet->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butClearTriplet);
    
    this->toggleShowSphere = new ToggleButton();
    this->toggleShowSphere->setButtonText("Show Sphere");
    this->toggleShowSphere->setBounds(110, 455, 120, 22);
    this->toggleShowSphere->addListener(this);
    this->toggleShowSphere->setToggleState(false, dontSendNotification);
    this->toggleShowSphere->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->toggleShowSphere->setLookAndFeel(this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->toggleShowSphere);

    this->setContentOwned(this->boxListSpeaker, false);
    this->boxListSpeaker->getContent()->addAndMakeVisible(tableListSpeakers);

    this->boxListSpeaker->repaint();
    this->boxListSpeaker->resized();
}

WindowEditSpeaker::~WindowEditSpeaker()
{
    this->mainParent->setShowShepre(false);
    //delete this->boxListSpeaker;
    delete this->toggleShowSphere;
    delete this->butAddSpeaker;
    delete this->butcompSpeakers;
    delete this->butsaveSpeakers;
    delete this->texEditNameConf;
    delete this->butClearTriplet;
    delete this->rNumOfSpeakers;
    delete this->rNumOfSpeakersLabel;
    delete this->rZenith;
    delete this->rZenithLabel;
    delete this->rRadius;
    delete this->rRadiusLabel;
    delete this->rOffsetAngle;
    delete this->rOffsetAngleLabel;
    delete this->butAddRing;
    this->mainParent->destroyWinSpeakConf();
}

void WindowEditSpeaker::setNameConfig(String name) {
    this->texEditNameConf->setText(name);
}

void WindowEditSpeaker::initComp()
{
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
    
    tableListSpeakers.getHeader().addColumn("Output", 8, 70, 50, 120,TableHeaderComponent::defaultFlags);

    tableListSpeakers.getHeader().addColumn("Direct", 9, 40, 40, 60,TableHeaderComponent::defaultFlags);
    
    tableListSpeakers.getHeader().addColumn("delete", 10, 40, 40, 60,TableHeaderComponent::defaultFlags);
    
    tableListSpeakers.getHeader().setSortColumnId (1, true); // sort forwards by the ID column
    
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

//void WindowEditSpeaker::sortOrderChanged(int newSortColumnId, bool isForwards) {
//    tableListSpeakers.getHeader().setSortColumnId(newSortColumnId, isForwards);
//   tableListSpeakers.getHeader().reSortTable();
    //tableListSpeakers.updateContent();
//}

void WindowEditSpeaker::buttonClicked(Button *button)
{
    if (button == this->toggleShowSphere) {
        this->mainParent->setShowShepre(this->toggleShowSphere->getToggleState());
    }
    else if (button == this->butAddSpeaker) {
        this->mainParent->addSpeaker();
        updateWinContent();
        this->tableListSpeakers.selectRow(this->getNumRows()-1);
    }
    else if (button == this->butcompSpeakers) {
        // TODO: Should return a value to tell if there is an error or not.
        this->mainParent->updateLevelComp();
    }
    else if (button == this->butsaveSpeakers) {
        this->mainParent->updateLevelComp();
        // TODO: We should save a xml file only if the previous call passed.
        String dir = this->mainParent->applicationProperties.getUserSettings()->getValue("lastSpeakerPresetDirectory");
        if (! File(dir).isDirectory()) {
            dir = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
        }
        String filename = File(this->mainParent->getCurrentFileSpeakerPath()).getFileName();

#ifdef __linux__
        FileChooser fc ("Choose a file to save...", dir + "/" + filename, "*.xml", false);
#else
        FileChooser fc ("Choose a file to save...", dir + "/" + filename, "*.xml", true);
#endif
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
    else if (button == this->butAddRing) {
        for (int i = 0; i < this->rNumOfSpeakers->getText().getIntValue(); i++) {
            this->mainParent->addSpeaker();
            float azimuth = 360.0f / this->rNumOfSpeakers->getText().getIntValue() * i + this->rOffsetAngle->getText().getFloatValue();
            if (azimuth > 360.0f) {
                azimuth -= 360.0f;
            } else if (azimuth < 0.0f) {
                azimuth += 360.0f;
            }
            float zenith = this->rZenith->getText().getFloatValue();
            float radius = this->rRadius->getText().getFloatValue();
            this->mainParent->getListSpeaker().back()->setAziZenRad(glm::vec3(azimuth, zenith, radius));
            
        }
        updateWinContent();
        this->tableListSpeakers.selectRow(this->getNumRows()-1);

    }
    else if (button == this->butClearTriplet) {
        this->mainParent->clearListTriplet();
    }
    else if (button->getName() != "" && (button->getName().getIntValue() >= 0 &&
        button->getName().getIntValue() <= this->mainParent->getListSpeaker().size())) {
        this->mainParent->removeSpeaker(button->getName().getIntValue());
        updateWinContent();
    }
    else {
        int row = button->getName().getIntValue() - 1000;
        if (button->getToggleState()) {
            this->mainParent->getListSpeaker()[row]->setDirectOut(true);
        }
        else {
            this->mainParent->getListSpeaker()[row]->setDirectOut(false);
        }
        updateWinContent();
    }
}

void WindowEditSpeaker::textEditorTextChanged(TextEditor& editor) {
    float value;
    String test;
    if (&editor == this->rNumOfSpeakers) {} 
    else if (&editor == this->rZenith) {
        test = this->rZenith->getText().retainCharacters(".");
        if (test.length() > 1) {
            this->rZenith->setText(this->rZenith->getText().dropLastCharacters(1), false);
        }
        value = this->rZenith->getText().getFloatValue();
        if (value > 90.0f) {
            this->rZenith->setText(String(90.0f), false);
        }
    } else if (&editor == this->rRadius) {
        test = this->rRadius->getText().retainCharacters(".");
        if (test.length() > 1) {
            this->rRadius->setText(this->rRadius->getText().dropLastCharacters(1), false);
        }
        value = this->rRadius->getText().getFloatValue();
        if (value > 1.0f) {
            this->rRadius->setText(String(1.0f), false);
        }
    } else if (&editor == this->rOffsetAngle) {
        test = this->rOffsetAngle->getText().retainCharacters(".");
        if (test.length() > 1) {
            this->rOffsetAngle->setText(this->rOffsetAngle->getText().dropLastCharacters(1), false);
        }
        value = this->rOffsetAngle->getText().getFloatValue();
        if (value < -180.0f) {
            this->rOffsetAngle->setText(String(-180.0f), false);
        } else if (value > 180.0f) {
            this->rOffsetAngle->setText(String(180.0f), false);
        }
    }
}

void WindowEditSpeaker::updateWinContent()
{
    this->numRows = (unsigned int)this->mainParent->getListSpeaker().size();
    this->tableListSpeakers.updateContent();
}

void WindowEditSpeaker::selectedRow(int value)
{
    MessageManagerLock mmlock;
    this->tableListSpeakers.selectRow(value);
    this->repaint();
}

void WindowEditSpeaker::closeButtonPressed()
{
    delete this;
}

void WindowEditSpeaker::resized()
{
    this->juce::DocumentWindow::resized();
    
    tableListSpeakers.setSize(getWidth(), getHeight()-145);
    
    this->boxListSpeaker->setSize(getWidth(), getHeight());
    this->boxListSpeaker->correctSize(getWidth()-10, getHeight()-30);

    this->butAddSpeaker->setBounds(5, getHeight()-130, 100, 22);
    this->butClearTriplet->setBounds(5, getHeight()-55, 100, 22);
    this->toggleShowSphere->setBounds(110, getHeight()-55, 120, 22);
    this->butcompSpeakers->setBounds(110, getHeight()-130, 100, 22);
    this->butsaveSpeakers->setBounds(215, getHeight()-130, 100, 22);
    this->texEditNameConf->setBounds(330, getHeight()-130, 240, 22);

    this->rNumOfSpeakersLabel->setBounds(5, getHeight()-95, 80, 24);
    this->rNumOfSpeakers->setBounds(5+80, getHeight()-95, 40, 24);
    this->rZenithLabel->setBounds(100, getHeight()-95, 80, 24);
    this->rZenith->setBounds(100+80, getHeight()-95, 60, 24);
    this->rRadiusLabel->setBounds(215, getHeight()-95, 80, 24);
    this->rRadius->setBounds(215+80, getHeight()-95, 60, 24);
    this->rOffsetAngleLabel->setBounds(360, getHeight()-95, 80, 24);
    this->rOffsetAngle->setBounds(360+80, getHeight()-95, 60, 24);
    this->butAddRing->setBounds(520, getHeight()-95, 100, 24);
}

String WindowEditSpeaker::getText (const int columnNumber, const int rowNumber) const
{
    String text = "";
    //this->mainParent->getLockSpeakers()->lock();
    if (this->mainParent->getListSpeaker().size() > rowNumber) {
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
            case 9:
                text =String(this->mainParent->getListSpeaker()[rowNumber]->getDirectOut());
                break;
            default:
                text ="?";
        }
    }

    //this->mainParent->getLockSpeakers()->unlock();
    return text;
}

void WindowEditSpeaker::setText(const int columnNumber, const int rowNumber, const String& newText)
{
    if(this->mainParent->getLockSpeakers()->try_lock()) {
        if (this->mainParent->getListSpeaker().size() > rowNumber) {
            glm::vec3 newP;
            switch(columnNumber) {
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
                case 8:
                    this->mainParent->getListSpeaker()[rowNumber]->setOutputPatch(newText.getIntValue());
                    break;
                case 9 :
                    this->mainParent->getListSpeaker()[rowNumber]->setDirectOut(newText.getIntValue());
                    break;
            }
        }
        this->mainParent->getLockSpeakers()->unlock();
    }
}

int WindowEditSpeaker::getNumRows()
{
    return numRows;
}

// This is overloaded from TableListBoxModel, and should fill in the background of the whole row
void WindowEditSpeaker::paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected){
        if(this->mainParent->getLockSpeakers()->try_lock()){
        this->mainParent->getListSpeaker()[rowNumber]->selectSpeaker();
        this->mainParent->getLockSpeakers()->unlock();
        }
        g.fillAll (this->grisFeel->getHighlightColour());
    }
    else{
        if(this->mainParent->getLockSpeakers()->try_lock()){
        this->mainParent->getListSpeaker()[rowNumber]->unSelectSpeaker();
        this->mainParent->getLockSpeakers()->unlock();
        }
        if (rowNumber % 2){
            g.fillAll (this->grisFeel->getBackgroundColour().withBrightness(0.6));
        }else{
            g.fillAll (this->grisFeel->getBackgroundColour().withBrightness(0.7));
        }
    }
}

// This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
// components.
void WindowEditSpeaker::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    g.setColour (Colours::black);
    g.setFont (font);
    
    if (this->mainParent->getLockSpeakers()->try_lock()) {
        if (this->mainParent->getListSpeaker().size() > rowNumber) {
            String text = getText(columnId, rowNumber);
            g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
        }
        this->mainParent->getLockSpeakers()->unlock();
    }
    g.setColour (Colours::black.withAlpha (0.2f));
    g.fillRect (width - 1, 0, 1, height);
}

Component* WindowEditSpeaker::refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/,
                                                      Component* existingComponentToUpdate)
{
    if (columnId == 9){
        ToggleButton* tbDirect = static_cast<ToggleButton*> (existingComponentToUpdate);
        if (tbDirect == nullptr)
            tbDirect = new ToggleButton ();
        tbDirect->setName(String(rowNumber  + 1000));
        tbDirect->setClickingTogglesState(true);
        tbDirect->setBounds(4, 404, 88, 22);
        tbDirect->addListener(this);
        // TODO: ToggleButton->setToggleState is deprecated, need an alternative.
        tbDirect->setToggleState(this->mainParent->getListSpeaker()[rowNumber]->getDirectOut(), false);
        tbDirect->setLookAndFeel(this->grisFeel);
        return tbDirect;
    }
    if (columnId == 10){
        TextButton* tbRemove = static_cast<TextButton*> (existingComponentToUpdate);
        if (tbRemove == nullptr)
            tbRemove = new TextButton ();
        tbRemove->setButtonText("X");
        tbRemove->setName(String(rowNumber));
        tbRemove->setBounds(4, 404, 88, 22);
        tbRemove->addListener(this);
        tbRemove->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
        tbRemove->setLookAndFeel(this->grisFeel);
        return tbRemove;
    }
    // The other columns are editable text columns, for which we use the custom Label component
    EditableTextCustomComponent* textLabel = static_cast<EditableTextCustomComponent*> (existingComponentToUpdate);
    if (textLabel == nullptr)
        textLabel = new EditableTextCustomComponent (*this);
    
    textLabel->setRowAndColumn (rowNumber, columnId);
    if(columnId==1){    //ID Speakers
        textLabel->setEditable(false);
    }
    return textLabel;
}




//======================================= WinJackSettings ===========================

WindowJackSetting::WindowJackSetting(const String& name, Colour backgroundColour, int buttonsNeeded,
                                     MainContentComponent * parent, GrisLookAndFeel * feel, int indR, 
                                    int indB, int indFF):
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

    this->labRecFormat = new Label();
    this->labRecFormat->setText("File Format :", NotificationType::dontSendNotification);
    this->labRecFormat->setJustificationType(Justification::right);
    this->labRecFormat->setBounds(10, 80, 80, 22);
    this->labRecFormat->setFont(this->grisFeel->getFont());
    this->labRecFormat->setLookAndFeel(this->grisFeel);
    this->labRecFormat->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->juce::Component::addAndMakeVisible(this->labRecFormat);
    
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

    this->recordFormat = new ComboBox();
    this->recordFormat->addItemList(FileFormats, 1);
    this->recordFormat->setSelectedItemIndex(indFF, dontSendNotification);
    this->recordFormat->setBounds(90, 80, 120, 22);
    this->recordFormat->setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->recordFormat);
    
    this->butValidSettings = new TextButton();
    this->butValidSettings->setButtonText("Save");
    this->butValidSettings->setBounds(122, 110, 88, 22);
    this->butValidSettings->addListener(this);
    this->butValidSettings->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butValidSettings->setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->butValidSettings);
}

WindowJackSetting::~WindowJackSetting() {
    delete this->labRate;
    delete this->labBuff;
    delete this->labRecFormat;
    delete this->cobRate;
    delete this->cobBuffer;
    delete this->recordFormat;
    delete this->butValidSettings;
    this->mainParent->destroyWinJackSetting();
}

void WindowJackSetting::closeButtonPressed()
{
    delete this;
}


void WindowJackSetting::buttonClicked(Button *button)
{
    if( button == this->butValidSettings) {
        this->mainParent->saveJackSettings(this->cobRate->getText().getIntValue(), this->cobBuffer->getText().getIntValue(),
                                           this->recordFormat->getSelectedItemIndex());
        delete this;
    }
}


