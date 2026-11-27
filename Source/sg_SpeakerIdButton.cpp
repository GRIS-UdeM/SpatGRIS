/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_SpeakerIdButton.hpp"

#include "sg_GrisLookAndFeel.hpp"

namespace gris
{
namespace
{
//==============================================================================
/** Content of the CallOutBox used to edit a speaker's colour: a colour selector and a "Default" button. */
class SpeakerColourEditor final
    : public juce::Component
    , private juce::ChangeListener
{
public:
    using Callback = std::function<void(tl::optional<juce::Colour>)>;

private:
    static constexpr int SELECTOR_WIDTH = 300;
    static constexpr int SELECTOR_HEIGHT = 400;
    static constexpr int BUTTON_HEIGHT = 24;
    static constexpr int PADDING = 4;

    Callback mCallback;
    juce::ColourSelector mSelector{ juce::ColourSelector::showColourAtTop | juce::ColourSelector::showSliders
                                        | juce::ColourSelector::showColourspace,
                                    4,
                                    4 };
    juce::TextButton mDefaultButton{ "Default" };

public:
    SpeakerColourEditor(juce::Colour const initialColour, Callback callback) : mCallback(std::move(callback))
    {
        mSelector.setName("background");
        mSelector.setCurrentColour(initialColour, juce::dontSendNotification);
        mSelector.setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
        mSelector.addChangeListener(this);
        addAndMakeVisible(mSelector);

        mDefaultButton.setTooltip("Reset the speaker to its default colour");
        mDefaultButton.onClick = [this] {
            mCallback(tl::nullopt);
            if (auto * callOutBox{ dynamic_cast<juce::CallOutBox *>(getParentComponent()) }) {
                callOutBox->dismiss();
            }
        };
        addAndMakeVisible(mDefaultButton);

        setSize(SELECTOR_WIDTH, SELECTOR_HEIGHT + PADDING + BUTTON_HEIGHT);
    }

    ~SpeakerColourEditor() override { mSelector.removeChangeListener(this); }

    void resized() override
    {
        auto bounds{ getLocalBounds() };
        mDefaultButton.setBounds(bounds.removeFromBottom(BUTTON_HEIGHT).reduced(PADDING * 10, 0));
        bounds.removeFromBottom(PADDING);
        mSelector.setBounds(bounds);
    }

private:
    void changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster * source) override
    {
        jassert(source == &mSelector);
        mCallback(mSelector.getCurrentColour());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpeakerColourEditor)
};
} // namespace

//==============================================================================
SpeakerIdButton::SpeakerIdButton(output_patch_t const outputPatch, Listener & listener, SmallGrisLookAndFeel & glaf)
    : mListener(listener)
    , mLookAndFeel(glaf)
    , mButton(false, juce::String{ outputPatch.get() }, "", *this, glaf)
{
#if JUCE_MAC
    const juce::String altName = "Option";
#else
    const juce::String altName = "Alt";
#endif
    mButton.setButtonTooltip("Select speaker (" + altName + "+click: change color, " + altName
                             + "+right-click: copy color to next speaker)");
    addAndMakeVisible(mButton);
}

//==============================================================================
void SpeakerIdButton::setSelected(bool const state)
{
    mIsSelected = state;
    refreshAppearance();
}

//==============================================================================
void SpeakerIdButton::setColor(tl::optional<juce::Colour> const & color)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mColour = color;
    refreshAppearance();
}

//==============================================================================
void SpeakerIdButton::refreshAppearance()
{
    if (mIsSelected) {
        mButton.setButtonColor(juce::TextButton::buttonColourId, mLookAndFeel.getOnColour());
        mButton.setLabelColour(juce::Label::textColourId, mLookAndFeel.getFontColour().contrasting(1.0f));
    } else if (mColour) {
        auto const textColour{ mColour->contrasting(1.0f) };
        mButton.setButtonColor(juce::TextButton::buttonColourId, *mColour);
        mButton.setLabelColour(juce::Label::textColourId, textColour);
    } else {
        mButton.setButtonColor(juce::TextButton::buttonColourId, mLookAndFeel.getBackgroundColour());
        mButton.setLabelColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
    }
}

//==============================================================================
void SpeakerIdButton::resized()
{
    mButton.setBounds(getLocalBounds());
}

//==============================================================================
void SpeakerIdButton::showColourEditor()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    // The button can be destroyed (e.g. slices rebuilt) while the CallOutBox is still open,
    // even if I don't really see when this can occur...
    juce::Component::SafePointer<SpeakerIdButton> safeThis{ this };
    auto callback = [safeThis](tl::optional<juce::Colour> const colour) {
        if (safeThis == nullptr) {
            return;
        }
        safeThis->mListener.speakerIdButtonColorChanged(safeThis.getComponent(), colour);
    };

    auto const initialColour{ mColour.value_or(mLookAndFeel.getBackgroundColour()) };
    auto editor{ std::make_unique<SpeakerColourEditor>(initialColour, std::move(callback)) };

    juce::CallOutBox::launchAsynchronously(std::move(editor), getScreenBounds(), nullptr);
}

//==============================================================================
void SpeakerIdButton::smallButtonClicked([[maybe_unused]] SmallToggleButton * button,
                                         bool /*state*/,
                                         bool const isLeftMouseButton)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(button == &mButton);

    auto const mods{ juce::ModifierKeys::getCurrentModifiers() };
    if (mods.isAltDown()) {
        if (isLeftMouseButton) {
            showColourEditor();
        } else {
            mListener.speakerIdButtonCopyColorToNextSpeaker(this, mColour);
        }
        return;
    }

    mListener.speakerIdButtonClicked(this);
}

} // namespace gris
