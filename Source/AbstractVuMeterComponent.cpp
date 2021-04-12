/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "VuMeterComponent.h"

#include "LogicStrucs.hpp"
#include "MainComponent.h"
#include "SpeakerModel.h"

//==============================================================================
VuMeterBox::VuMeterBox(VuMeterComponent & levelComponent, SmallGrisLookAndFeel & lookAndFeel)
    : mLevelComponent(levelComponent)
    , mLookAndFeel(lookAndFeel)
{
}

//==============================================================================
void VuMeterBox::setBounds(juce::Rectangle<int> const & newBounds)
{
    // TODO: move the painting stuff to paint()

    juce::Component::setBounds(newBounds);

    mColorGrad
        = juce::ColourGradient{ juce::Colour::fromRGB(255, 94, 69), 0.f,  0.f, juce::Colour::fromRGB(17, 255, 159), 0.f,
                                static_cast<float>(getHeight()),    false };
    mColorGrad.addColour(0.1, juce::Colours::yellow);

    // Create vu-meter foreground image.
    mVuMeterBit = juce::Image{ juce::Image::RGB, WIDTH - 1, HEIGHT, true };
    juce::Graphics gf{ mVuMeterBit };
    gf.setGradientFill(mColorGrad);
    gf.fillRect(0, 0, getWidth(), getHeight());
    gf.setColour(mLookAndFeel.getDarkColour());
    gf.setFont(10.0f);

    // Create vu-meter background image.
    mVuMeterBackBit = juce::Image{ juce::Image::RGB, WIDTH - 1, HEIGHT, true };
    juce::Graphics gb{ mVuMeterBackBit };
    gb.setColour(mLookAndFeel.getDarkColour());
    gb.fillRect(0, 0, getWidth(), getHeight());
    gb.setColour(mLookAndFeel.getScrollBarColour());
    gb.setFont(10.0f);

    // Create vu-meter muted image.
    mVuMeterMutedBit = juce::Image(juce::Image::RGB, WIDTH - 1, HEIGHT, true);
    juce::Graphics gm{ mVuMeterMutedBit };
    gm.setColour(mLookAndFeel.getWinBackgroundColour());
    gm.fillRect(0, 0, getWidth(), getHeight());
    gm.setColour(mLookAndFeel.getScrollBarColour());
    gm.setFont(10.0f);

    // Draw ticks on images.
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
void VuMeterBox::paint(juce::Graphics & g)
{
    if (mLevelComponent.isMuted()) {
        g.drawImage(mVuMeterMutedBit, 0, 0, WIDTH, HEIGHT, 0, 0, HEIGHT, WIDTH);
    } else {
        auto level{ std::min(mLevelComponent.getLevel(), MIN_LEVEL_COMP) };
        if (level > MAX_LEVEL_COMP) {
            mIsClipping = true;
            level = MAX_LEVEL_COMP;
        }

        auto const h = static_cast<int>(level.get() * -2.33333334f);
        auto const rel = HEIGHT - h;
        g.drawImage(mVuMeterBit, 0, h, WIDTH, rel, 0, h, WIDTH, rel);
        g.drawImage(mVuMeterBackBit, 0, 0, WIDTH, h, 0, 0, WIDTH, h);
        if (mIsClipping) {
            g.setColour(juce::Colour::fromHSV(0.0f, 1.0f, 0.75f, 1.0f));
            juce::Rectangle<float> const clipRect{ 0.5f, 0.5f, static_cast<float>(getWidth() - 1), 5.0f };
            g.fillRect(clipRect);
        }
    }
}

//==============================================================================
void VuMeterBox::mouseDown(juce::MouseEvent const & e)
{
    juce::Rectangle<int> const hitBox{ 0, 0, getWidth(), 20 };
    if (hitBox.contains(e.getPosition())) {
        resetClipping();
    }
}

//==============================================================================
void VuMeterBox::resetClipping()
{
    mIsClipping = false;
    repaint();
}

//==============================================================================
VuMeterComponent::VuMeterComponent(VuMeterModel & model,
                                   SmallGrisLookAndFeel & lookAndFeel,
                                   SpeakersData const & speakersData,
                                   bool const colorful)
    : mModel(model)
    , mLookAndFeel(lookAndFeel)
    , mContainerBox(*this, lookAndFeel)
    , mIsColorful(colorful)
    , mSpeakersData(speakersData)
{
    // Label
    mIdButton.setButtonText(juce::String{ mModel.getChannel() });
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
    if (mModel.isInput()) {
        mDirectOutButton.setButtonText("-");
        mDirectOutButton.setSize(22, 17);
        mDirectOutButton.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
        mDirectOutButton.setLookAndFeel(&lookAndFeel);
        mDirectOutButton.addListener(this);
        mDirectOutButton.addMouseListener(this, true);
        addAndMakeVisible(mDirectOutButton);
    }

    // Level box
    addAndMakeVisible(mContainerBox);
}

//==============================================================================
void VuMeterComponent::setLevel(dbfs_t const level)
{
    mLevel = level;
    repaint();
}

//==============================================================================
void VuMeterComponent::buttonClicked(juce::Button * button)
{
    if (button == &mMuteToggleButton) {
        if (mMuteToggleButton.getToggleState()) {
            mModel.setState(PortState::muted);
            mSoloToggleButton.setToggleState(false, juce::dontSendNotification);
        } else {
            mModel.setState(PortState::normal);
        }
        mContainerBox.repaint();

    } else if (button == &mSoloToggleButton) {
        if (mSoloToggleButton.getToggleState()) {
            mModel.setState(PortState::solo);
        }
        mContainerBox.repaint();

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
            auto const selected{ mLastMouseButton == MouseButton::left };
            mModel.setSelected(selected);
        }
    } else if (button == &mDirectOutButton) {
        juce::PopupMenu menu{};
        static constexpr auto CHOICE_NOT_DIRECT_OUT = std::numeric_limits<int>::min();
        static constexpr auto CHOICE_CANCELED = 0;
        juce::Array<output_patch_t> directOutSpeakers{};
        juce::Array<output_patch_t> nonDirectOutSpeakers{};
        for (auto const speaker : mSpeakersData) {
            auto & destination{ speaker.value->isDirectOutOnly ? directOutSpeakers : nonDirectOutSpeakers };
            destination.add(speaker.key);
        }
        menu.addItem(CHOICE_NOT_DIRECT_OUT, "-");
        for (auto const outputPatch : directOutSpeakers) {
            menu.addItem(outputPatch.get(), juce::String{ outputPatch.get() });
        }
        menu.addItem(CHOICE_NOT_DIRECT_OUT, "-");
        for (auto const outputPatch : nonDirectOutSpeakers) {
            menu.addItem(outputPatch.get(), juce::String{ outputPatch.get() });
        }

        auto const result{ menu.show() };

        if (result == CHOICE_CANCELED) {
            return;
        }

        tl::optional<output_patch_t> newOutputPatch{};
        if (result != CHOICE_NOT_DIRECT_OUT) {
            newOutputPatch = output_patch_t{ result };
        }

        mModel.

            if (result == 1)
        {
            mDirectOutButton.setButtonText("-");
        }
        else
        {
            value = mDirectOutSpeakers[result - 2];
            mDirectOutButton.setButtonText(juce::String{ value.get() });
        }

        mModel.changeDirectOutChannel(value);
        mModel.sendDirectOutToClient(mModel.getId(), value);
    }
}

//==============================================================================
void VuMeterComponent::mouseDown(juce::MouseEvent const & e)
{
    mLastMouseButton = (e.mods.isRightButtonDown() ? MouseButton::right : MouseButton::left);
}

//==============================================================================
void VuMeterComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    auto * colorSelector{ dynamic_cast<juce::ColourSelector *>(source) };
    if (colorSelector != nullptr) {
        mIdButton.setColour(juce::TextButton::buttonColourId, colorSelector->getCurrentColour());
        mModel.setColor(colorSelector->getCurrentColour());
        if (mLastMouseButton == MouseButton::right) {
            auto * input = dynamic_cast<InputModel *>(&mModel);
            jassert(input != nullptr);
            for (auto * it : input->getMainContentComponent().getSourceInputs()) {
                if (it->getChannel() == mModel.getChannel() + 1) {
                    it->setColor(colorSelector->getCurrentColour(), true);
                }
            }
        }
    }
}

//==============================================================================
void VuMeterComponent::setColor(juce::Colour const color)
{
    mIdButton.setColour(juce::TextButton::buttonColourId, color);
    repaint();
}

//==============================================================================
void VuMeterComponent::update()
{
    auto const level{ mModel.getLevel() };

    if (!std::isnan(level)) {
        if (!mMuteToggleButton.getToggleState() && mLevel != level) {
            repaint();
        }
        mLevel = level;
    }
}

//==============================================================================
void VuMeterComponent::setSelected(bool const value)
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
void VuMeterComponent::setBounds(juce::Rectangle<int> const & newBounds)
{
    static constexpr auto LEVEL_SIZE{ 140 };

    juce::Component::setBounds(newBounds);

    juce::Rectangle<int> const idBounds{ 0, 0, newBounds.getWidth(), mIdButton.getHeight() };
    mIdButton.setBounds(idBounds);

    juce::Rectangle<int> const muteButtonBounds{ 0, 158, mMuteToggleButton.getWidth(), mMuteToggleButton.getHeight() };
    mMuteToggleButton.setBounds(muteButtonBounds);

    juce::Rectangle<int> const soloButtonBounds{ mMuteToggleButton.getWidth() - 2,
                                                 158,
                                                 mMuteToggleButton.getWidth(),
                                                 mMuteToggleButton.getHeight() };
    mSoloToggleButton.setBounds(soloButtonBounds);

    if (mModel.isInput()) {
        juce::Rectangle<int> const directOutButtonBounds{ 0,
                                                          getHeight() - 27,
                                                          newBounds.getWidth(),
                                                          mDirectOutButton.getHeight() };
        mDirectOutButton.setBounds(directOutButtonBounds);
    }

    juce::Rectangle<int> const levelBoxBounds{ 0, 18, newBounds.getWidth() - WIDTH_RECT, LEVEL_SIZE };
    mContainerBox.setBounds(levelBoxBounds);
}
