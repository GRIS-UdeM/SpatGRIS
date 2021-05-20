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

#include "PrepareToRecordWindow.hpp"

#include "GrisLookAndFeel.hpp"
#include "MainComponent.hpp"

static constexpr int PADDING = 10;
static constexpr auto BUTTONS_WIDTH = 100;
static constexpr auto BUTTONS_HEIGHT = 35;
static constexpr auto WIDTH = 600;
static constexpr auto HEIGHT = PADDING * 3 + BUTTONS_HEIGHT * 2;

using flags = juce::FileBrowserComponent::FileChooserFlags;

//==============================================================================
PrepareToRecordComponent::PrepareToRecordComponent(juce::File const & recordingDirectory,
                                                   RecordingOptions const & recordingOptions,
                                                   MainContentComponent & mainContentComponent,
                                                   GrisLookAndFeel & lookAndFeel)
    : mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
    , mFileChooser("SpatGris recording", recordingDirectory, "*.wav;*.aiff", true, false, &mainContentComponent)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const validDirectory{ recordingDirectory.isDirectory()
                                   ? recordingDirectory
                                   : juce::File::getSpecialLocation(
                                       juce::File::SpecialLocationType::userDesktopDirectory) };
    auto const fileName{ juce::String{ "recording." }
                         + (recordingOptions.format == RecordingFormat::wav ? "wav" : "aiff") };
    auto const path{ validDirectory.getChildFile(fileName) };
    mPathEditor.setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    mPathEditor.setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::white);
    mPathEditor.setBorder(juce::BorderSize<int>{ 1 });
    mPathEditor.setText(path.getFullPathName());
    addAndMakeVisible(mPathEditor);

    mBrowseButton.setButtonText("Browse");
    mBrowseButton.addListener(this);
    addAndMakeVisible(mBrowseButton);

    mMonoButton.setButtonText(recordingFileTypeToString(RecordingFileType::mono));
    mMonoButton.setClickingTogglesState(true);
    mMonoButton.setRadioGroupId(PREPARE_TO_RECORD_WINDOW_FILE_TYPE_GROUP_ID, juce::dontSendNotification);
    addAndMakeVisible(mMonoButton);

    mInterleavedButton.setButtonText(recordingFileTypeToString(RecordingFileType::interleaved));
    mInterleavedButton.setClickingTogglesState(true);
    mInterleavedButton.setRadioGroupId(PREPARE_TO_RECORD_WINDOW_FILE_TYPE_GROUP_ID, juce::dontSendNotification);
    addAndMakeVisible(mInterleavedButton);

    mRecordButton.setButtonText("Start recording");
    mRecordButton.addListener(this);
    addAndMakeVisible(mRecordButton);

    if (recordingOptions.fileType == RecordingFileType::mono) {
        mMonoButton.setToggleState(true, juce::dontSendNotification);
    } else {
        jassert(recordingOptions.fileType == RecordingFileType::interleaved);
        mInterleavedButton.setToggleState(true, juce::dontSendNotification);
    }
}

//==============================================================================
void PrepareToRecordComponent::resized()
{
    auto const pathWidth{ WIDTH - PADDING * 3 - BUTTONS_WIDTH };
    auto const pathBounds{ juce::Rectangle<int>{ PADDING, PADDING, pathWidth, BUTTONS_HEIGHT }.reduced(0, 1) };
    mPathEditor.setBounds(pathBounds);
    mBrowseButton.setBounds(PADDING * 2 + pathWidth, PADDING, BUTTONS_WIDTH, BUTTONS_HEIGHT);

    auto const yOffset{ PADDING * 2 + BUTTONS_HEIGHT };

    mMonoButton.setBounds(PADDING, yOffset, BUTTONS_WIDTH, BUTTONS_HEIGHT);
    mInterleavedButton.setBounds(PADDING + BUTTONS_WIDTH + PADDING, yOffset, BUTTONS_WIDTH, BUTTONS_HEIGHT);
    mRecordButton.setBounds(WIDTH - PADDING - BUTTONS_WIDTH, yOffset, BUTTONS_WIDTH, BUTTONS_HEIGHT);
}

//==============================================================================
void PrepareToRecordComponent::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (button == &mBrowseButton) {
        if (!mFileChooser.browseForFileToSave(false)) {
            return;
        }
        auto const file{ mFileChooser.getResult() };

        mPathEditor.setText(file.getFullPathName(), juce::dontSendNotification);
        return;
    }

    if (button != &mRecordButton) {
        return;
    }

    auto const getFileType = [&]() {
        if (mMonoButton.getToggleState()) {
            return RecordingFileType::mono;
        }
        jassert(mInterleavedButton.getToggleState());
        return RecordingFileType::interleaved;
    };

    juce::File const path{ mPathEditor.getText() };

    auto const extension{ path.getFileExtension().toUpperCase().substring(1) }; // remove the dot
    auto const fileFormat{ stringToRecordingFormat(extension) };

    if (!fileFormat) {
        auto const message{ juce::String{ "Unsupported file format.\nValid file extensions are \"." }
                            + RECORDING_FORMAT_STRINGS.joinIntoString("\" \".") + "\"" };
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::WarningIcon,
                                               "Error",
                                               message,
                                               "Ok",
                                               this);
        return;
    }

    if (!path.getParentDirectory().isDirectory()) {
        auto const message{ juce::String{ "The parent directory \"" } + path.getParentDirectory().getFullPathName()
                            + "\" does not exist." };
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::WarningIcon,
                                               "Error",
                                               message,
                                               "Ok",
                                               this);
        return;
    }

    RecordingOptions const recordingOptions{ *fileFormat, getFileType() };
    mMainContentComponent.prepareAndStartRecording(path, recordingOptions);
}

//==============================================================================
PrepareToRecordWindow::PrepareToRecordWindow(juce::File const & recordingDirectory,
                                             RecordingOptions const & recordingOptions,
                                             MainContentComponent & mainContentComponent,
                                             GrisLookAndFeel & lookAndFeel)
    : DocumentWindow("Start recording", lookAndFeel.getBackgroundColour(), DocumentWindow::closeButton)
    , mMainContentComponent(mainContentComponent)
    , mContentComponent(recordingDirectory, recordingOptions, mainContentComponent, lookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    setUsingNativeTitleBar(true);
    setContentNonOwned(&mContentComponent, false);
    centreAroundComponent(&mainContentComponent, WIDTH, HEIGHT);
    mContentComponent.setSize(WIDTH, HEIGHT);
    DocumentWindow::setVisible(true);
}

//==============================================================================
void PrepareToRecordWindow::closeButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mMainContentComponent.closePrepareToRecordWindow();
}
