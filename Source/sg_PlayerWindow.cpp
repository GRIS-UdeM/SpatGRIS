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

#include "sg_PlayerWindow.hpp"

#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"
#include "sg_Player.hpp"

namespace gris
{
//==============================================================================
PlayerComponent::PlayerComponent(Player & player) : mPlayer(player)
{
    mLoadWavFilesAndSpeakerSetupButton.setButtonText("Load wav files and Speaker setup folder");
    // mLoadWavFilesAndSpeakerSetupButton.setClickingTogglesState(true);
    mLoadWavFilesAndSpeakerSetupButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mLoadWavFilesAndSpeakerSetupButton.addListener(this);
    addAndMakeVisible(mLoadWavFilesAndSpeakerSetupButton);
}

//==============================================================================
PlayerComponent::~PlayerComponent()
{
    mLoadWavFilesAndSpeakerSetupButton.removeListener(this);
}

//==============================================================================
void PlayerComponent::handleOpenWavFilesAndSpeakerSetup()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock const lock{ mLock };

    juce::File const wavFilesAndSSFolder;

    juce::FileChooser fc{ "Choose a folder to open...", wavFilesAndSSFolder, {}, true };

    if (!fc.browseForDirectory()) {
        return;
    }

    auto const chosen{ fc.getResult() };
    // maybe unused
    auto const succes{ mPlayer.loadWavFilesAndSpeakerSetup(chosen) };
}

//==============================================================================
// bool PlayerComponent::loadWavFilesAndSpeakerSetup(juce::File const & folder)
//{
//    JUCE_ASSERT_MESSAGE_THREAD;
//
//    juce::StringArray fileList;
//
//    for (const auto & filenameThatWasFound :
//         folder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*")) {
//        // filenameThatWasFound.get
//        fileList.add(filenameThatWasFound.getFileName());
//    }
//
//    for (const auto & file : fileList) {
//        DBG("File in player folder : " << file);
//    }
//
//    return false;
//}

//==============================================================================
void PlayerComponent::buttonClicked([[maybe_unused]] juce::Button * button)
{
    jassert(button == &mLoadWavFilesAndSpeakerSetupButton);
    DBG("Play button pressed.");
    handleOpenWavFilesAndSpeakerSetup();
}

//==============================================================================
void PlayerComponent::resized()
{
    mLoadWavFilesAndSpeakerSetupButton.setBounds(10, 10, 250, 30);
}

//==============================================================================
PlayerWindow::PlayerWindow(Player & player, MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel)
    : DocumentWindow("SpatGRIS Player", lookAndFeel.getBackgroundColour(), allButtons)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
    , mPlayerComponent(player)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    setResizable(true, true);
    setUsingNativeTitleBar(true);
    setContentNonOwned(&mPlayerComponent, false);
    centreWithSize(800, 400);
    DocumentWindow::setVisible(true);
}

//==============================================================================
void PlayerWindow::closeButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mMainContentComponent.closePlayerWindow();
}

} // namespace gris