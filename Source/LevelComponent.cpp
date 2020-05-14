/*
 This file is part of SpatGRIS2.
 
 Developers: Olivier Belanger, Nicolas Masson
 
 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MainComponent.h"
#include "LevelComponent.h"
#include "Speaker.h"

//========================== LevelBox =============================
LevelBox::LevelBox(LevelComponent * parent, SmallGrisLookAndFeel *feel) : mainParent(parent),
                                                                          grisFeel(feel) 
{}

LevelBox::~LevelBox() {}

void LevelBox::setBounds(const juce::Rectangle<int> &newBounds) {
    // LevelBox size is (22, 140)
    this->juce::Component::setBounds(newBounds);

    colorGrad = ColourGradient(Colour::fromRGB(255, 94, 69), 0.f, 0.f,
                               Colour::fromRGB(17, 255, 159), 0.f, getHeight(), false);
    colorGrad.addColour(0.1, Colours::yellow);

    // Create vu-meter foreground image.
    this->vumeterBit = Image(Image::RGB, 21, 140, true);
    Graphics gf (this->vumeterBit);
    gf.setGradientFill(colorGrad);
    gf.fillRect(0, 0, getWidth(), getHeight());

    // Create vu-meter background image.
    this->vumeterBackBit = Image(Image::RGB, 21, 140, true);
    Graphics gb (this->vumeterBackBit);
    gb.setColour(grisFeel->getDarkColour());
    gb.fillRect(0, 0, getWidth(), getHeight());

    // Create vu-meter muted image.
    this->vumeterMutedBit = Image(Image::RGB, 21, 140, true);
    Graphics gm (this->vumeterMutedBit);
    gm.setColour(grisFeel->getWinBackgroundColour());
    gm.fillRect(0, 0, getWidth(), getHeight());

    // Draw ticks on images.
    gf.setColour(this->grisFeel->getDarkColour());
    gf.setFont(10.0f);
    gb.setColour(this->grisFeel->getScrollBarColour());
    gb.setFont(10.0f);
    gm.setColour(this->grisFeel->getScrollBarColour());
    gm.setFont(10.0f);
    int start = getWidth() - 3;
    int y = 0;
    for (int i=1; i<10; i++) {
        y = i * 14;
        gf.drawLine(start, y, getWidth(), y, 1);
        gb.drawLine(start, y, getWidth(), y, 1);
        gm.drawLine(start, y, getWidth(), y, 1);
        if (i % 2 == 1) {
            gf.drawText(String(i*-6), start-15, y-5, 15, 10, Justification::centred, false);
            gb.drawText(String(i*-6), start-15, y-5, 15, 10, Justification::centred, false);
            gm.drawText(String(i*-6), start-15, y-5, 15, 10, Justification::centred, false);
        }
    }
}

void LevelBox::paint(Graphics& g) {
    if (this->mainParent->isMuted()) {
        g.drawImage(this->vumeterMutedBit, 0, 0, 22, 140, 0, 0, 22, 140);
    } else {
        float level = this->mainParent->getLevel();
        if (level < MinLevelComp) {
            level = MinLevelComp;
        } else if (level > MaxLevelComp) {
            isClipping = true;
            level = MaxLevelComp;
        }

        int h = (int)(level * -2.33333334);
        int rel = 140 - h;
        g.drawImage(this->vumeterBit, 0, h, 22, rel, 0, h, 22, rel);
        g.drawImage(this->vumeterBackBit, 0, 0, 22, h, 0, 0, 22, h);
        if (isClipping) {
            g.setColour(Colour::fromHSV(0.0, 1, 0.75, 1));
            juce::Rectangle<float> clipRect (0.5, 0.5, getWidth()-1, 5);
            g.fillRect(clipRect);
        }
    }
}

void LevelBox::mouseDown(const MouseEvent& e) {
    juce::Rectangle<int> hitBox (0, 0, getWidth(), 20);
    if (hitBox.contains(e.getPosition())) {
        resetClipping();
    }
}

void LevelBox::resetClipping() {
    isClipping = false;
    repaint(); 
}

//======================== LevelComponent ===========================
LevelComponent::LevelComponent(ParentLevelComponent *parent, SmallGrisLookAndFeel *feel, bool colorful)
{
    this->mainParent = parent;
    this->grisFeel = feel;
    this->isColorful = colorful;
    
    // Label
    this->idBut.reset(new TextButton());
    this->idBut->setButtonText(String(this->mainParent->getButtonInOutNumber()));
    this->idBut->setSize(22, 17);
    this->idBut->setTopLeftPosition(0, 0);
    this->idBut->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->idBut->setLookAndFeel(this->grisFeel);
    this->idBut->setColour(TextButton::textColourOnId, this->grisFeel->getFontColour());
    this->idBut->setColour(TextButton::textColourOffId, this->grisFeel->getFontColour());
    this->idBut->setColour(TextButton::buttonColourId, this->grisFeel->getBackgroundColour());
    this->idBut->addListener(this);
    this->idBut->addMouseListener(this, true);
    this->addAndMakeVisible(this->idBut.get());
    
    // ToggleButton (mute)
    this->muteToggleBut.reset(new ToggleButton());
    this->muteToggleBut->setButtonText("m");
    this->muteToggleBut->setSize(13, 15);
    this->muteToggleBut->addListener(this);
    this->muteToggleBut->setToggleState(false, dontSendNotification);
    this->muteToggleBut->setLookAndFeel(this->grisFeel);
    this->muteToggleBut->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->addAndMakeVisible(this->muteToggleBut.get());
    
    // ToggleButton (solo)
    this->soloToggleBut.reset(new ToggleButton());
    this->soloToggleBut->setButtonText("s");
    this->soloToggleBut->setSize(13, 15);
    this->soloToggleBut->addListener(this);
    this->soloToggleBut->setToggleState(false, dontSendNotification);
    this->soloToggleBut->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->soloToggleBut->setColour(TextButton::buttonColourId, this->grisFeel->getBackgroundColour());
    this->soloToggleBut->setLookAndFeel(this->grisFeel);
    this->addAndMakeVisible(this->soloToggleBut.get());

    // ComboBox (direct out)
    if (this->mainParent->isInput()) {
        this->directOut.reset(new TextButton());
        this->directOut->setButtonText("-");
        this->directOut->setSize(22, 17);
        this->directOut->setColour(Label::textColourId, this->grisFeel->getFontColour());
        this->directOut->setLookAndFeel(this->grisFeel);
        this->directOut->addListener(this);
        this->directOut->addMouseListener(this, true);
        this->addAndMakeVisible(this->directOut.get());
    }

    // Level box
    this->levelBox.reset(new LevelBox(this, this->grisFeel));
    this->addAndMakeVisible(this->levelBox.get());
}

void LevelComponent::updateDirectOutMenu(juce::OwnedArray<Speaker> & spkList) {
    if (this->mainParent->isInput()) {
        this->directOutSpeakers.clear();
        for (auto & it : spkList) {
            if (it->getDirectOut()) {
                this->directOutSpeakers.push_back(it->getOutputPatch());
            }
        }
    }
}

void LevelComponent::buttonClicked(Button *button) {
    if (button == this->muteToggleBut.get()) {
        this->mainParent->setMuted(this->muteToggleBut->getToggleState());
        if (this->muteToggleBut->getToggleState()) {
            this->soloToggleBut->setToggleState(false, dontSendNotification);
        }
        this->levelBox->repaint();
        
    } else if (button == this->soloToggleBut.get()) {
        this->mainParent->setSolo(this->soloToggleBut->getToggleState());
        if (this->soloToggleBut->getToggleState()) {
            this->muteToggleBut->setToggleState(false, dontSendNotification);
        }
        this->levelBox->repaint();
        
    } else if (button == this->idBut.get()) {
        if (this->isColorful) { //Input
            ColourSelector* colourSelector = new ColourSelector();
            colourSelector->setName("background");
            colourSelector->setCurrentColour(this->idBut->findColour(TextButton::buttonColourId));
            colourSelector->addChangeListener(this);
            colourSelector->setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
            colourSelector->setSize(300, 400);
            CallOutBox::launchAsynchronously(colourSelector, getScreenBounds(), nullptr);
        } else { //Output
            this->mainParent->selectClick(this->lastMouseButton);
        }
    } else if (button == this->directOut.get()) {
        PopupMenu menu;
        menu.addItem(1, "-");
        for (unsigned int j=0, i=2; j<this->directOutSpeakers.size(); j++, i++) {
            menu.addItem(i, String(this->directOutSpeakers[j]));
        }

        auto result = menu.show();

        int value = 0;
        if (result == 0) {
            return;
        } else if (result == 1) {
            this->directOut->setButtonText("-");
        } else {
            value = this->directOutSpeakers[result-2];
            this->directOut->setButtonText(String(value));
        }

        this->mainParent->changeDirectOutChannel(value);
        this->mainParent->sendDirectOutToClient(this->mainParent->getId(), value);
    }
}

void LevelComponent::mouseDown(const MouseEvent& e) {
    if (e.mods.isRightButtonDown()) {
        this->lastMouseButton = 0;
    } else {
        this->lastMouseButton = 1;
    }
}

void LevelComponent::changeListenerCallback(ChangeBroadcaster* source) {
    if (ColourSelector *cs = dynamic_cast<ColourSelector *> (source)) {
        this->idBut->setColour(TextButton::buttonColourId, cs->getCurrentColour());
        this->mainParent->setColor(cs->getCurrentColour());
        if (this->lastMouseButton == 0) {
            Input *input = dynamic_cast<Input *> (this->mainParent);
            for (auto&& it : input->getMainContentComponent().getListSourceInput()) {
                if (it->getId() == (this->mainParent->getId() + 1)) {
                    it->setColor(cs->getCurrentColour(), true);
                }
            }
        }
    }
}

void LevelComponent::resetClipping() {
    this->levelBox->resetClipping();
}

float LevelComponent::getLevel() {
    return level;
}

void LevelComponent::update() {
    float l = this->mainParent->getLevel();

    if (isnan(l)) { 
        return;
    }

    if (!this->muteToggleBut->getToggleState() && this->level != l) {
        this->repaint();
    }
    this->level = l;
}


bool LevelComponent::isMuted() {
    return this->muteToggleBut->getToggleState();
}

void LevelComponent::setSelected(bool value) {
    const MessageManagerLock mmLock;
    if (value) {
        this->idBut->setColour(TextButton::textColourOnId, this->grisFeel->getWinBackgroundColour());
        this->idBut->setColour(TextButton::textColourOffId, this->grisFeel->getWinBackgroundColour());
        this->idBut->setColour(TextButton::buttonColourId, this->grisFeel->getOnColour());
    } else {
        this->idBut->setColour(TextButton::textColourOnId, this->grisFeel->getFontColour());
        this->idBut->setColour(TextButton::textColourOffId, this->grisFeel->getFontColour());
        this->idBut->setColour(TextButton::buttonColourId, this->grisFeel->getBackgroundColour());
    }
}

void LevelComponent::setBounds(const juce::Rectangle<int> &newBounds) {
    int levelSize = 140;

    this->juce::Component::setBounds(newBounds);

    juce::Rectangle<int> labRect(0, 0, newBounds.getWidth(), this->idBut->getHeight());
    this->idBut->setBounds(labRect);
    this->muteToggleBut->setBounds(0, 158, this->muteToggleBut->getWidth(), this->muteToggleBut->getHeight());
    this->soloToggleBut->setBounds(this->muteToggleBut->getWidth()-2, 158,
                                   this->muteToggleBut->getWidth(), this->muteToggleBut->getHeight());
    if (this->mainParent->isInput()) {
        this->directOut->setBounds(0, getHeight()-27, newBounds.getWidth(), this->directOut->getHeight());
    }

    juce::Rectangle<int> level(0, 18, newBounds.getWidth()-WidthRect, levelSize);
    this->levelBox->setBounds(level);
}
