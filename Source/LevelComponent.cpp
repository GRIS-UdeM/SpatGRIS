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
    : mLevelComponent(levelComponent)
    , mLookAndFeel(lookAndFeel)
{
}

//==============================================================================
void LevelBox::setBounds(juce::Rectangle<int> const & newBounds)
{
    // TODO: this function should not be drawing or loading images.

    // LevelBox size is (22, 140)
    this->juce::Component::setBounds(newBounds); // TODO: this does not look ok!

    this->mColorGrad = juce::ColourGradient{
        juce::Colour::fromRGB(255, 94, 69),    0.f,  0.f, juce::Colour::fromRGB(17, 255, 159), 0.f,
        static_cast<float>(this->getHeight()), false
    };
    this->mColorGrad.addColour(0.1, juce::Colours::yellow);

    // Create vu-meter foreground image.
    this->mVuMeterBit = juce::Image{ juce::Image::RGB, 21, 140, true };
    juce::Graphics gf{ this->mVuMeterBit };
    gf.setGradientFill(this->mColorGrad);
    gf.fillRect(0, 0, this->getWidth(), this->getHeight());

    // Create vu-meter background image.
    this->mVuMeterBackBit = juce::Image{ juce::Image::RGB, 21, 140, true };
    juce::Graphics gb{ this->mVuMeterBackBit };
    gb.setColour(this->mLookAndFeel.getDarkColour());
    gb.fillRect(0, 0, this->getWidth(), this->getHeight());

    // Create vu-meter muted image.
    this->mVuMeterMutedBit = juce::Image(juce::Image::RGB, 21, 140, true);
    juce::Graphics gm{ this->mVuMeterMutedBit };
    gm.setColour(this->mLookAndFeel.getWinBackgroundColour());
    gm.fillRect(0, 0, this->getWidth(), this->getHeight());

    // Draw ticks on images.
    gf.setColour(this->mLookAndFeel.getDarkColour());
    gf.setFont(10.0f);
    gb.setColour(this->mLookAndFeel.getScrollBarColour());
    gb.setFont(10.0f);
    gm.setColour(this->mLookAndFeel.getScrollBarColour());
    gm.setFont(10.0f);
    int const start = this->getWidth() - 3;
    for (int i{ 1 }; i < 10; i++) {
        auto const y = i * 14;
        auto const y_f{ static_cast<float>(y) };
        auto const start_f{ static_cast<float>(start) };
        auto const with_f{ static_cast<float>(getWidth()) };

        gf.drawLine(start_f, y_f, with_f, y_f, 1.0f);
        gb.drawLine(start_f, y_f, with_f, y_f, 1.0f);
        gm.drawLine(start_f, y_f, with_f, y_f, 1.0f);
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
    if (this->mLevelComponent.isMuted()) {
        g.drawImage(this->mVuMeterMutedBit, 0, 0, 22, 140, 0, 0, 22, 140);
    } else {
        float level{ this->mLevelComponent.getLevel() };
        if (level < MIN_LEVEL_COMP) {
            level = MIN_LEVEL_COMP;
        } else if (level > MAX_LEVEL_COMP) {
            this->mIsClipping = true;
            level = MAX_LEVEL_COMP;
        }

        int const h = static_cast<int>(level * -2.33333334f);
        int const rel = 140 - h;
        g.drawImage(this->mVuMeterBit, 0, h, 22, rel, 0, h, 22, rel);
        g.drawImage(this->mVuMeterBackBit, 0, 0, 22, h, 0, 0, 22, h);
        if (this->mIsClipping) {
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
    this->mIsClipping = false;
    this->repaint();
}

//==============================================================================
LevelComponent::LevelComponent(ParentLevelComponent & parentLevelComponent,
                               SmallGrisLookAndFeel & lookAndFeel,
                               bool const colorful)
    : mParentLevelComponent(parentLevelComponent)
    , mLookAndFeel(lookAndFeel)
    , mIsColorful(colorful)
    , mLevelBox(*this, lookAndFeel)
{
    // Label
    this->mIdButton.setButtonText(juce::String{ this->mParentLevelComponent.getButtonInOutNumber() });
    this->mIdButton.setSize(22, 17);
    this->mIdButton.setTopLeftPosition(0, 0);
    this->mIdButton.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    this->mIdButton.setLookAndFeel(&lookAndFeel);
    this->mIdButton.setColour(juce::TextButton::textColourOnId, lookAndFeel.getFontColour());
    this->mIdButton.setColour(juce::TextButton::textColourOffId, lookAndFeel.getFontColour());
    this->mIdButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.getBackgroundColour());
    this->mIdButton.addListener(this);
    this->mIdButton.addMouseListener(this, true);
    this->addAndMakeVisible(this->mIdButton);

    // ToggleButton (mute)
    this->mMuteToggleButton.setButtonText("m");
    this->mMuteToggleButton.setSize(13, 15);
    this->mMuteToggleButton.addListener(this);
    this->mMuteToggleButton.setToggleState(false, juce::dontSendNotification);
    this->mMuteToggleButton.setLookAndFeel(&lookAndFeel);
    this->mMuteToggleButton.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    this->addAndMakeVisible(this->mMuteToggleButton);

    // ToggleButton (solo)
    this->mSoloToggleButton.setButtonText("s");
    this->mSoloToggleButton.setSize(13, 15);
    this->mSoloToggleButton.addListener(this);
    this->mSoloToggleButton.setToggleState(false, juce::dontSendNotification);
    this->mSoloToggleButton.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    this->mSoloToggleButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.getBackgroundColour());
    this->mSoloToggleButton.setLookAndFeel(&lookAndFeel);
    this->addAndMakeVisible(this->mSoloToggleButton);

    // ComboBox (direct out)
    if (this->mParentLevelComponent.isInput()) {
        this->directOut.setButtonText("-");
        this->directOut.setSize(22, 17);
        this->directOut.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
        this->directOut.setLookAndFeel(&lookAndFeel);
        this->directOut.addListener(this);
        this->directOut.addMouseListener(this, true);
        this->addAndMakeVisible(this->directOut);
    }

    // Level box
    this->addAndMakeVisible(this->mLevelBox);
}

//==============================================================================
void LevelComponent::updateDirectOutMenu(juce::OwnedArray<Speaker> const & spkList)
{
    if (this->mParentLevelComponent.isInput()) {
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
    if (button == &this->mMuteToggleButton) {
        this->mParentLevelComponent.setMuted(this->mMuteToggleButton.getToggleState());
        if (this->mMuteToggleButton.getToggleState()) {
            this->mSoloToggleButton.setToggleState(false, juce::dontSendNotification);
        }
        this->mLevelBox.repaint();

    } else if (button == &this->mSoloToggleButton) {
        this->mParentLevelComponent.setSolo(this->mSoloToggleButton.getToggleState());
        if (this->mSoloToggleButton.getToggleState()) {
            this->mMuteToggleButton.setToggleState(false, juce::dontSendNotification);
        }
        this->mLevelBox.repaint();

    } else if (button == &this->mIdButton) {
        if (this->mIsColorful) { // Input
            auto * colourSelector{ new juce::ColourSelector{} };
            colourSelector->setName("background");
            colourSelector->setCurrentColour(this->mIdButton.findColour(juce::TextButton::buttonColourId));
            colourSelector->addChangeListener(this);
            colourSelector->setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
            colourSelector->setSize(300, 400);
            std::unique_ptr<juce::Component> component{ colourSelector };
            juce::CallOutBox::launchAsynchronously(std::move(component), getScreenBounds(), nullptr);
        } else { // Output
            this->mParentLevelComponent.selectClick(this->mLastMouseButton);
        }
    } else if (button == &this->directOut) {
        juce::PopupMenu menu{};
        menu.addItem(1, "-");
        for (size_t i{}; i < this->directOutSpeakers.size(); ++i) {
            menu.addItem(static_cast<int>(i) + 2, juce::String{ this->directOutSpeakers[i] });
        }

        auto const result{ menu.show() };

        int value{};
        if (result != 0) {
            if (result == 1) {
                this->directOut.setButtonText("-");
            } else {
                value = this->directOutSpeakers[result - 2];
                this->directOut.setButtonText(juce::String{ value });
            }

            this->mParentLevelComponent.changeDirectOutChannel(value);
            this->mParentLevelComponent.sendDirectOutToClient(this->mParentLevelComponent.getId(), value);
        }
    }
}

//==============================================================================
void LevelComponent::mouseDown(juce::MouseEvent const & e)
{
    if (e.mods.isRightButtonDown()) {
        this->mLastMouseButton = 0;
    } else {
        this->mLastMouseButton = 1;
    }
}

//==============================================================================
void LevelComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    juce::ColourSelector * cs{ dynamic_cast<juce::ColourSelector *>(source) };
    if (cs != nullptr) {
        this->mIdButton.setColour(juce::TextButton::buttonColourId, cs->getCurrentColour());
        this->mParentLevelComponent.setColor(cs->getCurrentColour());
        if (this->mLastMouseButton == 0) {
            Input * input = dynamic_cast<Input *>(&this->mParentLevelComponent);
            jassert(input != nullptr);
            for (auto * it : input->getMainContentComponent().getSourceInputs()) {
                if (it->getId() == this->mParentLevelComponent.getId() + 1) {
                    it->setColor(cs->getCurrentColour(), true);
                }
            }
        }
    }
}

//==============================================================================
void LevelComponent::setColor(juce::Colour const color)
{
    this->mIdButton.setColour(juce::TextButton::buttonColourId, color);
    this->repaint();
}

//==============================================================================
void LevelComponent::update()
{
    auto const l = this->mParentLevelComponent.getLevel();

    if (!std::isnan(l)) {
        if (!this->mMuteToggleButton.getToggleState() && this->mLevel != l) {
            this->repaint();
        }
        this->mLevel = l;
    }
}

//==============================================================================
void LevelComponent::setSelected(bool const value)
{
    juce::MessageManagerLock const mmLock{};
    if (value) {
        this->mIdButton.setColour(juce::TextButton::textColourOnId, this->mLookAndFeel.getWinBackgroundColour());
        this->mIdButton.setColour(juce::TextButton::textColourOffId, this->mLookAndFeel.getWinBackgroundColour());
        this->mIdButton.setColour(juce::TextButton::buttonColourId, this->mLookAndFeel.getOnColour());
    } else {
        this->mIdButton.setColour(juce::TextButton::textColourOnId, this->mLookAndFeel.getFontColour());
        this->mIdButton.setColour(juce::TextButton::textColourOffId, this->mLookAndFeel.getFontColour());
        this->mIdButton.setColour(juce::TextButton::buttonColourId, this->mLookAndFeel.getBackgroundColour());
    }
}

//==============================================================================
void LevelComponent::setBounds(juce::Rectangle<int> const & newBounds)
{
    int const levelSize{ 140 };

    this->juce::Component::setBounds(newBounds); // TODO: this does not look ok!

    juce::Rectangle<int> const labRect{ 0, 0, newBounds.getWidth(), this->mIdButton.getHeight() };
    this->mIdButton.setBounds(labRect);
    this->mMuteToggleButton.setBounds(0, 158, this->mMuteToggleButton.getWidth(), this->mMuteToggleButton.getHeight());
    this->mSoloToggleButton.setBounds(this->mMuteToggleButton.getWidth() - 2,
                                      158,
                                      this->mMuteToggleButton.getWidth(),
                                      this->mMuteToggleButton.getHeight());
    if (this->mParentLevelComponent.isInput()) {
        this->directOut.setBounds(0, getHeight() - 27, newBounds.getWidth(), this->directOut.getHeight());
    }

    juce::Rectangle<int> const level{ 0, 18, newBounds.getWidth() - WIDTH_RECT, levelSize };
    this->mLevelBox.setBounds(level);
}
