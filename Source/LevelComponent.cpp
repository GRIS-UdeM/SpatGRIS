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
    juce::Component::setBounds(newBounds); // TODO: this does not look ok!

    mColorGrad
        = juce::ColourGradient{ juce::Colour::fromRGB(255, 94, 69), 0.f,  0.f, juce::Colour::fromRGB(17, 255, 159), 0.f,
                                static_cast<float>(getHeight()),    false };
    mColorGrad.addColour(0.1, juce::Colours::yellow);

    // Create vu-meter foreground image.
    mVuMeterBit = juce::Image{ juce::Image::RGB, 21, 140, true };
    juce::Graphics gf{ mVuMeterBit };
    gf.setGradientFill(mColorGrad);
    gf.fillRect(0, 0, getWidth(), getHeight());

    // Create vu-meter background image.
    mVuMeterBackBit = juce::Image{ juce::Image::RGB, 21, 140, true };
    juce::Graphics gb{ mVuMeterBackBit };
    gb.setColour(mLookAndFeel.getDarkColour());
    gb.fillRect(0, 0, getWidth(), getHeight());

    // Create vu-meter muted image.
    mVuMeterMutedBit = juce::Image(juce::Image::RGB, 21, 140, true);
    juce::Graphics gm{ mVuMeterMutedBit };
    gm.setColour(mLookAndFeel.getWinBackgroundColour());
    gm.fillRect(0, 0, getWidth(), getHeight());

    // Draw ticks on images.
    gf.setColour(mLookAndFeel.getDarkColour());
    gf.setFont(10.0f);
    gb.setColour(mLookAndFeel.getScrollBarColour());
    gb.setFont(10.0f);
    gm.setColour(mLookAndFeel.getScrollBarColour());
    gm.setFont(10.0f);
    int const start = getWidth() - 3;
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
    if (mLevelComponent.isMuted()) {
        g.drawImage(mVuMeterMutedBit, 0, 0, 22, 140, 0, 0, 22, 140);
    } else {
        float level{ mLevelComponent.getLevel() };
        if (level < MIN_LEVEL_COMP) {
            level = MIN_LEVEL_COMP;
        } else if (level > MAX_LEVEL_COMP) {
            mIsClipping = true;
            level = MAX_LEVEL_COMP;
        }

        int const h = static_cast<int>(level * -2.33333334f);
        int const rel = 140 - h;
        g.drawImage(mVuMeterBit, 0, h, 22, rel, 0, h, 22, rel);
        g.drawImage(mVuMeterBackBit, 0, 0, 22, h, 0, 0, 22, h);
        if (mIsClipping) {
            g.setColour(juce::Colour::fromHSV(0.0, 1, 0.75, 1));
            juce::Rectangle<float> const clipRect{ 0.5, 0.5, static_cast<float>(getWidth() - 1), 5 };
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
    mIsClipping = false;
    repaint();
}

//==============================================================================
LevelComponent::LevelComponent(ParentLevelComponent & parentLevelComponent,
                               SmallGrisLookAndFeel & lookAndFeel,
                               bool const colorful)
    : mParentLevelComponent(parentLevelComponent)
    , mLookAndFeel(lookAndFeel)
    , mLevelBox(*this, lookAndFeel)
    , mIsColorful(colorful)
{
    // Label
    auto const id{ mParentLevelComponent.getButtonInOutNumber() };
    mIdButton.setButtonText(juce::String{ id });
    mIdButton.setSize(22, 17);
    mIdButton.setTopLeftPosition(0, 0);
    mIdButton.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    mIdButton.setLookAndFeel(&lookAndFeel);
    mIdButton.setColour(juce::TextButton::textColourOnId, lookAndFeel.getFontColour());
    mIdButton.setColour(juce::TextButton::textColourOffId, lookAndFeel.getFontColour());
    mIdButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.getBackgroundColour());
    mIdButton.addListener(this);
    mIdButton.addMouseListener(this, true);
    addAndMakeVisible(mIdButton);

    // ToggleButton (mute)
    mMuteToggleButton.setButtonText("m");
    mMuteToggleButton.setSize(13, 15);
    mMuteToggleButton.addListener(this);
    mMuteToggleButton.setToggleState(false, juce::dontSendNotification);
    mMuteToggleButton.setLookAndFeel(&lookAndFeel);
    mMuteToggleButton.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mMuteToggleButton);

    // ToggleButton (solo)
    mSoloToggleButton.setButtonText("s");
    mSoloToggleButton.setSize(13, 15);
    mSoloToggleButton.addListener(this);
    mSoloToggleButton.setToggleState(false, juce::dontSendNotification);
    mSoloToggleButton.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    mSoloToggleButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.getBackgroundColour());
    mSoloToggleButton.setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(mSoloToggleButton);

    // ComboBox (direct out)
    if (mParentLevelComponent.isInput()) {
        mDirectOutButton.setButtonText("-");
        mDirectOutButton.setSize(22, 17);
        mDirectOutButton.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
        mDirectOutButton.setLookAndFeel(&lookAndFeel);
        mDirectOutButton.addListener(this);
        mDirectOutButton.addMouseListener(this, true);
        addAndMakeVisible(mDirectOutButton);
    }

    // Level box
    addAndMakeVisible(mLevelBox);
}

//==============================================================================
void LevelComponent::updateDirectOutMenu(std::vector<output_patch_t> directOuts)
{
    if (mParentLevelComponent.isInput()) {
        mDirectOutSpeakers = std::move(directOuts);
    }
}

//==============================================================================
void LevelComponent::buttonClicked(juce::Button * button)
{
    if (button == &mMuteToggleButton) {
        mParentLevelComponent.setMuted(mMuteToggleButton.getToggleState());
        if (mMuteToggleButton.getToggleState()) {
            mSoloToggleButton.setToggleState(false, juce::dontSendNotification);
        }
        mLevelBox.repaint();

    } else if (button == &mSoloToggleButton) {
        mParentLevelComponent.setSolo(mSoloToggleButton.getToggleState());
        if (mSoloToggleButton.getToggleState()) {
            mMuteToggleButton.setToggleState(false, juce::dontSendNotification);
        }
        mLevelBox.repaint();

    } else if (button == &mIdButton) {
        if (mIsColorful) { // Input
            auto * colourSelector{ new juce::ColourSelector{} };
            colourSelector->setName("background");
            colourSelector->setCurrentColour(mIdButton.findColour(juce::TextButton::buttonColourId));
            colourSelector->addChangeListener(this);
            colourSelector->setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
            colourSelector->setSize(300, 400);
            std::unique_ptr<juce::Component> component{ colourSelector };
            juce::CallOutBox::launchAsynchronously(std::move(component), getScreenBounds(), nullptr);
        } else { // Output
            mParentLevelComponent.selectClick(mLastMouseButton);
        }
    } else if (button == &mDirectOutButton) {
        juce::PopupMenu menu{};
        menu.addItem(1, "-");
        for (size_t i{}; i < mDirectOutSpeakers.size(); ++i) {
            menu.addItem(narrow<int>(i) + 2, juce::String{ mDirectOutSpeakers[i].get() });
        }

        auto const result{ menu.show() };

        output_patch_t value{};
        if (result != 0) {
            if (result == 1) {
                mDirectOutButton.setButtonText("-");
            } else {
                value = mDirectOutSpeakers[result - 2];
                mDirectOutButton.setButtonText(juce::String{ value.get() });
            }

            mParentLevelComponent.changeDirectOutChannel(value);
            mParentLevelComponent.sendDirectOutToClient(mParentLevelComponent.getId(), value);
        }
    }
}

//==============================================================================
void LevelComponent::mouseDown(juce::MouseEvent const & e)
{
    if (e.mods.isRightButtonDown()) {
        mLastMouseButton = 0;
    } else {
        mLastMouseButton = 1;
    }
}

//==============================================================================
void LevelComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    juce::ColourSelector * cs{ dynamic_cast<juce::ColourSelector *>(source) };
    if (cs != nullptr) {
        mIdButton.setColour(juce::TextButton::buttonColourId, cs->getCurrentColour());
        mParentLevelComponent.setColor(cs->getCurrentColour());
        if (mLastMouseButton == 0) {
            Input * input = dynamic_cast<Input *>(&mParentLevelComponent);
            jassert(input != nullptr);
            for (auto * it : input->getMainContentComponent().getSourceInputs()) {
                if (it->getId() == mParentLevelComponent.getId() + 1) {
                    it->setColor(cs->getCurrentColour(), true);
                }
            }
        }
    }
}

//==============================================================================
void LevelComponent::setColor(juce::Colour const color)
{
    mIdButton.setColour(juce::TextButton::buttonColourId, color);
    repaint();
}

//==============================================================================
void LevelComponent::update()
{
    auto const l = mParentLevelComponent.getLevel();

    if (!std::isnan(l)) {
        if (!mMuteToggleButton.getToggleState() && mLevel != l) {
            repaint();
        }
        mLevel = l;
    }
}

//==============================================================================
void LevelComponent::setSelected(bool const value)
{
    juce::MessageManagerLock const mmLock{};
    if (value) {
        mIdButton.setColour(juce::TextButton::textColourOnId, mLookAndFeel.getWinBackgroundColour());
        mIdButton.setColour(juce::TextButton::textColourOffId, mLookAndFeel.getWinBackgroundColour());
        mIdButton.setColour(juce::TextButton::buttonColourId, mLookAndFeel.getOnColour());
    } else {
        mIdButton.setColour(juce::TextButton::textColourOnId, mLookAndFeel.getFontColour());
        mIdButton.setColour(juce::TextButton::textColourOffId, mLookAndFeel.getFontColour());
        mIdButton.setColour(juce::TextButton::buttonColourId, mLookAndFeel.getBackgroundColour());
    }
}

//==============================================================================
void LevelComponent::setBounds(juce::Rectangle<int> const & newBounds)
{
    int const levelSize{ 140 };

    juce::Component::setBounds(newBounds); // TODO: this does not look ok!

    juce::Rectangle<int> const labRect{ 0, 0, newBounds.getWidth(), mIdButton.getHeight() };
    mIdButton.setBounds(labRect);
    mMuteToggleButton.setBounds(0, 158, mMuteToggleButton.getWidth(), mMuteToggleButton.getHeight());
    mSoloToggleButton.setBounds(mMuteToggleButton.getWidth() - 2,
                                158,
                                mMuteToggleButton.getWidth(),
                                mMuteToggleButton.getHeight());
    if (mParentLevelComponent.isInput()) {
        mDirectOutButton.setBounds(0, getHeight() - 27, newBounds.getWidth(), mDirectOutButton.getHeight());
    }

    juce::Rectangle<int> const level{ 0, 18, newBounds.getWidth() - WIDTH_RECT, levelSize };
    mLevelBox.setBounds(level);
}
