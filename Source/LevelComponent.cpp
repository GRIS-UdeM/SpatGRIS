/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "LevelComponent.h"

#include "MainComponent.h"
#include "Speaker.h"

//==============================================================================
LevelBox::LevelBox(LevelComponent & levelComponent, SmallGrisLookAndFeel & lookAndFeel)
    : levelComponent(levelComponent)
    , lookAndFeel(lookAndFeel)
{
}

//==============================================================================
void LevelBox::setBounds(juce::Rectangle<int> const & newBounds)
{
    // TODO: this function should not be drawing or loading images.

    // LevelBox size is (22, 140)
    this->juce::Component::setBounds(newBounds); // TODO: this does not look ok!

    this->colorGrad = juce::ColourGradient{
        juce::Colour::fromRGB(255, 94, 69),    0.f,  0.f, juce::Colour::fromRGB(17, 255, 159), 0.f,
        static_cast<float>(this->getHeight()), false
    };
    this->colorGrad.addColour(0.1, juce::Colours::yellow);

    // Create vu-meter foreground image.
    this->vumeterBit = juce::Image{ juce::Image::RGB, 21, 140, true };
    juce::Graphics gf{ this->vumeterBit };
    gf.setGradientFill(this->colorGrad);
    gf.fillRect(0, 0, this->getWidth(), this->getHeight());

    // Create vu-meter background image.
    this->vumeterBackBit = juce::Image{ juce::Image::RGB, 21, 140, true };
    juce::Graphics gb{ this->vumeterBackBit };
    gb.setColour(this->lookAndFeel.getDarkColour());
    gb.fillRect(0, 0, this->getWidth(), this->getHeight());

    // Create vu-meter muted image.
    this->vumeterMutedBit = juce::Image(juce::Image::RGB, 21, 140, true);
    juce::Graphics gm{ this->vumeterMutedBit };
    gm.setColour(this->lookAndFeel.getWinBackgroundColour());
    gm.fillRect(0, 0, this->getWidth(), this->getHeight());

    // Draw ticks on images.
    gf.setColour(this->lookAndFeel.getDarkColour());
    gf.setFont(10.0f);
    gb.setColour(this->lookAndFeel.getScrollBarColour());
    gb.setFont(10.0f);
    gm.setColour(this->lookAndFeel.getScrollBarColour());
    gm.setFont(10.0f);
    int const start = this->getWidth() - 3;
    int y{};
    for (int i{ 1 }; i < 10; i++) {
        y = i * 14;
        gf.drawLine(start, y, getWidth(), y, 1);
        gb.drawLine(start, y, getWidth(), y, 1);
        gm.drawLine(start, y, getWidth(), y, 1);
        if (i % 2 == 1) {
            gf.drawText(juce::String(i * -6), start - 15, y - 5, 15, 10, juce::Justification::centred, false);
            gb.drawText(juce::String(i * -6), start - 15, y - 5, 15, 10, juce::Justification::centred, false);
            gm.drawText(juce::String(i * -6), start - 15, y - 5, 15, 10, juce::Justification::centred, false);
        }
    }
}

//==============================================================================
void LevelBox::paint(juce::Graphics & g)
{
    if (this->levelComponent.isMuted()) {
        g.drawImage(this->vumeterMutedBit, 0, 0, 22, 140, 0, 0, 22, 140);
    } else {
        float level{ this->levelComponent.getLevel() };
        if (level < MinLevelComp) {
            level = MinLevelComp;
        } else if (level > MaxLevelComp) {
            this->isClipping = true;
            level = MaxLevelComp;
        }

        int const h = static_cast<int>(level * -2.33333334f);
        int const rel = 140 - h;
        g.drawImage(this->vumeterBit, 0, h, 22, rel, 0, h, 22, rel);
        g.drawImage(this->vumeterBackBit, 0, 0, 22, h, 0, 0, 22, h);
        if (this->isClipping) {
            g.setColour(juce::Colour::fromHSV(0.0, 1, 0.75, 1));
            juce::Rectangle<float> const clipRect{ 0.5, 0.5, static_cast<float>(this->getWidth() - 1), 5 };
            g.fillRect(clipRect);
        }
    }
}

//==============================================================================
void LevelBox::mouseDown(juce::MouseEvent const & e)
{
    juce::Rectangle<int> const hitBox{ 0, 0, getWidth(), 20 };
    if (hitBox.contains(e.getPosition())) {
        resetClipping();
    }
}

//==============================================================================
void LevelBox::resetClipping()
{
    this->isClipping = false;
    this->repaint();
}

//==============================================================================
LevelComponent::LevelComponent(ParentLevelComponent & parentLevelComponent,
                               SmallGrisLookAndFeel & lookAndFeel,
                               bool const colorful)
    : parentLevelComponent(parentLevelComponent)
    , lookAndFeel(lookAndFeel)
    , isColorful(colorful)
    , levelBox(*this, lookAndFeel)
{
    // Label
    this->idBut.setButtonText(juce::String{ this->parentLevelComponent.getButtonInOutNumber() });
    this->idBut.setSize(22, 17);
    this->idBut.setTopLeftPosition(0, 0);
    this->idBut.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    this->idBut.setLookAndFeel(&lookAndFeel);
    this->idBut.setColour(juce::TextButton::textColourOnId, lookAndFeel.getFontColour());
    this->idBut.setColour(juce::TextButton::textColourOffId, lookAndFeel.getFontColour());
    this->idBut.setColour(juce::TextButton::buttonColourId, lookAndFeel.getBackgroundColour());
    this->idBut.addListener(this);
    this->idBut.addMouseListener(this, true);
    this->addAndMakeVisible(this->idBut);

    // ToggleButton (mute)
    this->muteToggleBut.setButtonText("m");
    this->muteToggleBut.setSize(13, 15);
    this->muteToggleBut.addListener(this);
    this->muteToggleBut.setToggleState(false, juce::dontSendNotification);
    this->muteToggleBut.setLookAndFeel(&lookAndFeel);
    this->muteToggleBut.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    this->addAndMakeVisible(this->muteToggleBut);

    // ToggleButton (solo)
    this->soloToggleBut.setButtonText("s");
    this->soloToggleBut.setSize(13, 15);
    this->soloToggleBut.addListener(this);
    this->soloToggleBut.setToggleState(false, juce::dontSendNotification);
    this->soloToggleBut.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    this->soloToggleBut.setColour(juce::TextButton::buttonColourId, lookAndFeel.getBackgroundColour());
    this->soloToggleBut.setLookAndFeel(&lookAndFeel);
    this->addAndMakeVisible(this->soloToggleBut);

    // ComboBox (direct out)
    if (this->parentLevelComponent.isInput()) {
        this->directOut.setButtonText("-");
        this->directOut.setSize(22, 17);
        this->directOut.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
        this->directOut.setLookAndFeel(&lookAndFeel);
        this->directOut.addListener(this);
        this->directOut.addMouseListener(this, true);
        this->addAndMakeVisible(this->directOut);
    }

    // Level box
    this->addAndMakeVisible(this->levelBox);
}

//==============================================================================
void LevelComponent::updateDirectOutMenu(juce::OwnedArray<Speaker> const & spkList)
{
    if (this->parentLevelComponent.isInput()) {
        this->directOutSpeakers.clear();
        for (auto const speaker : spkList) {
            if (speaker->isDirectOut()) {
                this->directOutSpeakers.push_back(speaker->getOutputPatch());
            }
        }
    }
}

//==============================================================================
void LevelComponent::buttonClicked(juce::Button * button)
{
    if (button == &this->muteToggleBut) {
        this->parentLevelComponent.setMuted(this->muteToggleBut.getToggleState());
        if (this->muteToggleBut.getToggleState()) {
            this->soloToggleBut.setToggleState(false, juce::dontSendNotification);
        }
        this->levelBox.repaint();

    } else if (button == &this->soloToggleBut) {
        this->parentLevelComponent.setSolo(this->soloToggleBut.getToggleState());
        if (this->soloToggleBut.getToggleState()) {
            this->muteToggleBut.setToggleState(false, juce::dontSendNotification);
        }
        this->levelBox.repaint();

    } else if (button == &this->idBut) {
        if (this->isColorful) { // Input
            auto * colourSelector{ new juce::ColourSelector{} };
            colourSelector->setName("background");
            colourSelector->setCurrentColour(this->idBut.findColour(juce::TextButton::buttonColourId));
            colourSelector->addChangeListener(this);
            colourSelector->setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
            colourSelector->setSize(300, 400);
            juce::CallOutBox::launchAsynchronously(colourSelector, getScreenBounds(), nullptr);
        } else { // Output
            this->parentLevelComponent.selectClick(this->lastMouseButton);
        }
    } else if (button == &this->directOut) {
        juce::PopupMenu menu{};
        menu.addItem(1, "-");
        for (size_t i{}; i < this->directOutSpeakers.size(); ++i) {
            menu.addItem(i + 2, juce::String{ this->directOutSpeakers[i] });
        }

        auto result{ menu.show() };

        int value{};
        if (result != 0) {
            if (result == 1) {
                this->directOut.setButtonText("-");
            } else {
                value = this->directOutSpeakers[result - 2];
                this->directOut.setButtonText(juce::String{ value });
            }

            this->parentLevelComponent.changeDirectOutChannel(value);
            this->parentLevelComponent.sendDirectOutToClient(this->parentLevelComponent.getId(), value);
        }
    }
}

//==============================================================================
void LevelComponent::mouseDown(juce::MouseEvent const & e)
{
    if (e.mods.isRightButtonDown()) {
        this->lastMouseButton = 0;
    } else {
        this->lastMouseButton = 1;
    }
}

//==============================================================================
void LevelComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    juce::ColourSelector * cs{ dynamic_cast<juce::ColourSelector *>(source) };
    if (cs != nullptr) {
        this->idBut.setColour(juce::TextButton::buttonColourId, cs->getCurrentColour());
        this->parentLevelComponent.setColor(cs->getCurrentColour());
        if (this->lastMouseButton == 0) {
            Input * input = dynamic_cast<Input *>(&this->parentLevelComponent);
            jassert(input != nullptr);
            for (auto * it : input->getMainContentComponent().getListSourceInput()) {
                if (it->getId() == this->parentLevelComponent.getId() + 1) {
                    it->setColor(cs->getCurrentColour(), true);
                }
            }
        }
    }
}

//==============================================================================
void LevelComponent::update()
{
    auto const l = this->parentLevelComponent.getLevel();

    if (!std::isnan(l)) {
        if (!this->muteToggleBut.getToggleState() && this->level != l) {
            this->repaint();
        }
        this->level = l;
    }
}

//==============================================================================
void LevelComponent::setSelected(bool const value)
{
    juce::MessageManagerLock const mmLock{};
    if (value) {
        this->idBut.setColour(juce::TextButton::textColourOnId, this->lookAndFeel.getWinBackgroundColour());
        this->idBut.setColour(juce::TextButton::textColourOffId, this->lookAndFeel.getWinBackgroundColour());
        this->idBut.setColour(juce::TextButton::buttonColourId, this->lookAndFeel.getOnColour());
    } else {
        this->idBut.setColour(juce::TextButton::textColourOnId, this->lookAndFeel.getFontColour());
        this->idBut.setColour(juce::TextButton::textColourOffId, this->lookAndFeel.getFontColour());
        this->idBut.setColour(juce::TextButton::buttonColourId, this->lookAndFeel.getBackgroundColour());
    }
}

//==============================================================================
void LevelComponent::setBounds(juce::Rectangle<int> const & newBounds)
{
    int const levelSize{ 140 };

    this->juce::Component::setBounds(newBounds); // TODO: this does not look ok!

    juce::Rectangle<int> const labRect{ 0, 0, newBounds.getWidth(), this->idBut.getHeight() };
    this->idBut.setBounds(labRect);
    this->muteToggleBut.setBounds(0, 158, this->muteToggleBut.getWidth(), this->muteToggleBut.getHeight());
    this->soloToggleBut.setBounds(this->muteToggleBut.getWidth() - 2,
                                  158,
                                  this->muteToggleBut.getWidth(),
                                  this->muteToggleBut.getHeight());
    if (this->parentLevelComponent.isInput()) {
        this->directOut.setBounds(0, getHeight() - 27, newBounds.getWidth(), this->directOut.getHeight());
    }

    juce::Rectangle<int> const level{ 0, 18, newBounds.getWidth() - WidthRect, levelSize };
    this->levelBox.setBounds(level);
}
