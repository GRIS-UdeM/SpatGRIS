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

juce::String const SourceVuMeterComponent::NO_DIRECT_OUT_TEXT = "-";

//==============================================================================
void LevelBox::setBounds(juce::Rectangle<int> const & newBounds)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const shouldRecomputeImages{ newBounds.getWidth() != getBounds().getWidth()
                                      || newBounds.getHeight() != getBounds().getHeight() };

    juce::Component::setBounds(newBounds);

    if (!shouldRecomputeImages) {
        return;
    }

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
    auto const start = getWidth() - 3;
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
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mIsMuted) {
        g.drawImage(mVuMeterMutedBit, 0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT);
        return;
    }

    if (mLevel <= MIN_LEVEL_COMP && !mIsClipping) {
        g.drawImage(mVuMeterBackBit, 0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT);
        return;
    }

    auto const h = static_cast<int>(mLevel.get() * -2.33333334f);
    auto const rel = HEIGHT - h;
    g.drawImage(mVuMeterBit, 0, h, WIDTH, rel, 0, h, WIDTH, rel);
    g.drawImage(mVuMeterBackBit, 0, 0, WIDTH, h, 0, 0, WIDTH, h);
    if (mIsClipping) {
        g.setColour(juce::Colour::fromHSV(0.0, 1, 0.75, 1));
        juce::Rectangle<float> const clipRect{ 0.5, 0.5, static_cast<float>(getWidth() - 1), 5 };
        g.fillRect(clipRect);
    }
}

//==============================================================================
void LevelBox::mouseDown(juce::MouseEvent const & e)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::Rectangle<int> const hitBox{ 0, 0, getWidth(), 20 };
    if (hitBox.contains(e.getPosition())) {
        resetClipping();
    }
}

//==============================================================================
void LevelBox::resetClipping()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mIsClipping = false;
    repaint();
}

//==============================================================================
void LevelBox::setLevel(dbfs_t const level)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const & clippedLevel{ std::clamp(level, MIN_LEVEL_COMP, MAX_LEVEL_COMP) };

    if (clippedLevel == mLevel) {
        return;
    }

    if (level > MAX_LEVEL_COMP) {
        mIsClipping = true;
    }
    mLevel = clippedLevel;

    repaint();
}

//==============================================================================
void LevelBox::setMuted(bool const muted)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (muted == mIsMuted) {
        return;
    }
    mIsMuted = muted;
    repaint();
}

//==============================================================================
AbstractVuMeterComponent::AbstractVuMeterComponent(int const channel, SmallGrisLookAndFeel & lookAndFeel)
    : mLookAndFeel(lookAndFeel)
    , mLevelBox(lookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    // Label
    mIdButton.setButtonText(juce::String{ channel });
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

    // Level box
    addAndMakeVisible(mLevelBox);
}

//==============================================================================
void AbstractVuMeterComponent::setState(PortState const state, bool const soloMode)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSoloToggleButton.setToggleState(state == PortState::solo, juce::dontSendNotification);
    mMuteToggleButton.setToggleState(state == PortState::muted, juce::dontSendNotification);
    mLevelBox.setMuted(soloMode ? state != PortState::solo : state == PortState::muted);

    repaint();
}

//==============================================================================
void SpeakerVuMeterComponent::setSelected(bool const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (value) {
        mIdButton.setColour(juce::TextButton::textColourOnId, mLookAndFeel.getWinBackgroundColour());
        mIdButton.setColour(juce::TextButton::textColourOffId, mLookAndFeel.getWinBackgroundColour());
        mIdButton.setColour(juce::TextButton::buttonColourId, mLookAndFeel.getOnColour());
    } else {
        mIdButton.setColour(juce::TextButton::textColourOnId, mLookAndFeel.getFontColour());
        mIdButton.setColour(juce::TextButton::textColourOffId, mLookAndFeel.getFontColour());
        mIdButton.setColour(juce::TextButton::buttonColourId, mLookAndFeel.getBackgroundColour());
    }
    repaint();
}

//==============================================================================
void SpeakerVuMeterComponent::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (button == &mMuteToggleButton) {
        auto const newState{ mMuteToggleButton.getToggleState() ? PortState::muted : PortState::normal };
        mOwner.handleSpeakerStateChanged(mOutputPatch, newState);
    } else if (button == &mSoloToggleButton) {
        auto const newState{ mSoloToggleButton.getToggleState() ? PortState::solo : PortState::normal };
        mOwner.handleSpeakerStateChanged(mOutputPatch, newState);
    } else if (button == &mIdButton) {
        mOwner.handleSpeakerSelected(mOutputPatch);
    }
}

//==============================================================================
void AbstractVuMeterComponent::setBounds(juce::Rectangle<int> const & newBounds)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static constexpr auto LEVEL_SIZE{ LevelBox::HEIGHT };

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

    juce::Rectangle<int> const levelBoxBounds{ 0, 18, newBounds.getWidth() - WIDTH_RECT, LEVEL_SIZE };
    mLevelBox.setBounds(levelBoxBounds);
}

//==============================================================================
SourceVuMeterComponent::SourceVuMeterComponent(source_index_t const sourceIndex,
                                               tl::optional<output_patch_t> const directOut,
                                               juce::Colour const colour,
                                               Owner & owner,
                                               SmallGrisLookAndFeel & lookAndFeel)
    : AbstractVuMeterComponent(sourceIndex.get(), lookAndFeel)
    , mSourceIndex(sourceIndex)
    , mOwner(owner)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mDirectOutButton.setSize(22, 17);
    mDirectOutButton.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    mDirectOutButton.setLookAndFeel(&lookAndFeel);
    mDirectOutButton.addListener(this);
    mDirectOutButton.addMouseListener(this, true);
    setDirectOut(directOut);
    addAndMakeVisible(mDirectOutButton);

    setSourceColour(colour);
}

//==============================================================================
void SourceVuMeterComponent::setDirectOut(tl::optional<output_patch_t> const outputPatch)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    // TODO : the following code always results in some app-crashing invalid string when the optional holds a value. Why
    // is that ? Is it possible that the perfect-forwarding of a juce::String does something weird ?

    /*static auto const PATCH_TO_STRING
        = [](output_patch_t const outputPatch) -> juce::String { return juce::String{ outputPatch.get() }; };

    auto const newText{ outputPatch.map_or(PATCH_TO_STRING, NO_DIRECT_OUT_TEXT) };*/

    auto const newText{ outputPatch ? juce::String{ outputPatch->get() } : NO_DIRECT_OUT_TEXT };
    mDirectOutButton.setButtonText(newText);
}

//==============================================================================
void SourceVuMeterComponent::setSourceColour(juce::Colour const colour)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mIdButton.setColour(juce::TextButton::buttonColourId, colour);
}

//==============================================================================
void SourceVuMeterComponent::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (button == &mMuteToggleButton) {
        muteButtonClicked();
    } else if (button == &mSoloToggleButton) {
        // SOLO
        soloButtonClicked();
    } else if (button == &mIdButton) {
        colorSelectorButtonClicked();
    } else if (button == &mDirectOutButton) {
        directOutButtonClicked();
    }
}

//==============================================================================
void SourceVuMeterComponent::setBounds(const juce::Rectangle<int> & newBounds)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    AbstractVuMeterComponent::setBounds(newBounds);

    juce::Rectangle<int> const directOutButtonBounds{ 0,
                                                      getHeight() - 27,
                                                      newBounds.getWidth(),
                                                      mDirectOutButton.getHeight() };
    mDirectOutButton.setBounds(directOutButtonBounds);
}

//==============================================================================
void SourceVuMeterComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto * colorSelector{ dynamic_cast<juce::ColourSelector *>(source) };
    jassert(colorSelector);
    if (colorSelector != nullptr) {
        mOwner.handleSourceColorChanged(mSourceIndex, colorSelector->getCurrentColour());
    }
}

//==============================================================================
void SourceVuMeterComponent::muteButtonClicked() const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const newState{ mMuteToggleButton.getToggleState() ? PortState::muted : PortState::normal };
    mOwner.handleSourceStateChanged(mSourceIndex, newState);
}

//==============================================================================
void SourceVuMeterComponent::soloButtonClicked() const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const newState{ mSoloToggleButton.getToggleState() ? PortState::solo : PortState::normal };
    mOwner.handleSourceStateChanged(mSourceIndex, newState);
}

//==============================================================================
void SourceVuMeterComponent::colorSelectorButtonClicked()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto * colourSelector{ new juce::ColourSelector{} };
    colourSelector->setName("background");
    colourSelector->setCurrentColour(mIdButton.findColour(juce::TextButton::buttonColourId));
    colourSelector->addChangeListener(this);
    colourSelector->setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
    colourSelector->setSize(300, 400);
    std::unique_ptr<juce::Component> component{ colourSelector };
    juce::CallOutBox::launchAsynchronously(std::move(component), getScreenBounds(), nullptr);
}

//==============================================================================
void SourceVuMeterComponent::directOutButtonClicked() const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::PopupMenu menu{};
    static constexpr auto CHOICE_NOT_DIRECT_OUT = std::numeric_limits<int>::min();
    static constexpr auto CHOICE_CANCELED = 0;
    juce::Array<output_patch_t> directOutSpeakers{};
    juce::Array<output_patch_t> nonDirectOutSpeakers{};
    for (auto const speaker : mOwner.getSpeakersData()) {
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

    mOwner.handleSourceDirectOutChanged(mSourceIndex, newOutputPatch);
}

//==============================================================================
SpeakerVuMeterComponent::SpeakerVuMeterComponent(output_patch_t const outputPatch,
                                                 Owner & owner,
                                                 SmallGrisLookAndFeel & lookAndFeel)
    : AbstractVuMeterComponent(outputPatch.get(), lookAndFeel)
    , mOutputPatch(outputPatch)
    , mOwner(owner)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    setSelected(false);
}
