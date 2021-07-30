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

#include "AddRemoveSourcesWindow.hpp"

#include "GrisLookAndFeel.hpp"
#include "MainComponent.hpp"
#include "constants.hpp"

static constexpr auto COMPONENT_WIDTH = 200;
static constexpr auto ROW_HEIGHT = 23;
static constexpr auto EDITOR_WIDTH = 50;
static constexpr auto BUTTON_WIDTH = 80;
static constexpr auto PADDING = 12;

//==============================================================================
AddRemoveSourcesComponent::AddRemoveSourcesComponent(int const currentNumberOfSources,
                                                     MainContentComponent & mainContentComponent,
                                                     GrisLookAndFeel & lookAndFeel)
    : mMainContentComponent(mainContentComponent)
    , mLabel("", "Number of Sources :")
    , mApplyButton("Apply")
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mLabel.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
    mLabel.setJustificationType(juce::Justification::bottomRight);

    mNumberOfSourcesEditor.setInputRestrictions(3, "0123456789");
    mNumberOfSourcesEditor.setJustification(juce::Justification::centredLeft);
    mNumberOfSourcesEditor.setSelectAllWhenFocused(true);
    mNumberOfSourcesEditor.setText(juce::String{ currentNumberOfSources }, false);
    mNumberOfSourcesEditor.addListener(this);

    mApplyButton.addListener(this);

    addAndMakeVisible(mLabel);
    addAndMakeVisible(mNumberOfSourcesEditor);
    addAndMakeVisible(mApplyButton);
}

//==============================================================================
int AddRemoveSourcesComponent::getWidth()
{
    return COMPONENT_WIDTH;
}

//==============================================================================
int AddRemoveSourcesComponent::getHeight()
{
    return ROW_HEIGHT * 2 + PADDING * 3;
}

//==============================================================================
void AddRemoveSourcesComponent::applyAndClose() const
{
    auto const numberOfSources{ std::clamp(mNumberOfSourcesEditor.getText().getIntValue(), 1, MAX_NUM_SOURCES) };
    mMainContentComponent.numSourcesChanged(numberOfSources);
    mMainContentComponent.closeAddRemoveSourcesWindow();
}

//==============================================================================
void AddRemoveSourcesComponent::buttonClicked([[maybe_unused]] juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(button == &mApplyButton);
    applyAndClose();
}

//==============================================================================
void AddRemoveSourcesComponent::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const width{ getWidth() };
    auto const height{ getHeight() };

    auto x{ PADDING };
    auto y{ PADDING };

    auto const labelWidth{ width - PADDING * 3 - EDITOR_WIDTH };
    mLabel.setBounds(x, y, labelWidth, ROW_HEIGHT);

    x += labelWidth;
    mNumberOfSourcesEditor.setBounds(x, y, EDITOR_WIDTH, ROW_HEIGHT);

    x = (width - BUTTON_WIDTH) / 2;
    y = height - PADDING - ROW_HEIGHT;

    mApplyButton.setBounds(x, y, BUTTON_WIDTH, ROW_HEIGHT);
}

//==============================================================================
void AddRemoveSourcesComponent::textEditorReturnKeyPressed([[maybe_unused]] juce::TextEditor & editor)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(&editor == &mNumberOfSourcesEditor);
    applyAndClose();
}

//==============================================================================
AddRemoveSourcesWindow::AddRemoveSourcesWindow(int const currentNumberOfSources,
                                               MainContentComponent & mainContentComponent,
                                               GrisLookAndFeel & lookAndFeel)
    : DocumentWindow("Set sources", lookAndFeel.getBackgroundColour(), closeButton)
    , mMainContentComponent(mainContentComponent)
    , mComponent(currentNumberOfSources, mainContentComponent, lookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    setUsingNativeTitleBar(true);
    setContentNonOwned(&mComponent, false);
    DocumentWindow::setVisible(true);
    centreAroundComponent(&mainContentComponent,
                          AddRemoveSourcesComponent::getWidth(),
                          AddRemoveSourcesComponent::getHeight());
}

//==============================================================================
void AddRemoveSourcesWindow::closeButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mMainContentComponent.closeAddRemoveSourcesWindow();
}
