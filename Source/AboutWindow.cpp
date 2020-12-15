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

#include "AboutWindow.h"

#include "GrisLookAndFeel.h"
#include "MainComponent.h"
#include "constants.hpp"

//==============================================================================
AboutComponent::AboutComponent(AboutWindow & parentWindow, GrisLookAndFeel & lookAndFeel) : mParentWindow(parentWindow)
{
    setSize(400, 500);

    auto const & icon{ SERVER_GRIS_ICON_SMALL_FILE };
    if (icon.exists()) {
        auto const img{ juce::ImageFileFormat::loadFrom(icon) };
        mLogoImage.setImage(img);
        mLogoImage.setBounds(136, 5, 128, 127);
        addAndMakeVisible(mLogoImage);
    }

    mTitleLabel.setText("SpatGRIS v2 - Sound Spatialization Tool\n\n", juce::NotificationType::dontSendNotification);
    mTitleLabel.setJustificationType(juce::Justification::horizontallyCentred);
    mTitleLabel.setBounds(5, 150, 390, 50);
    mTitleLabel.setLookAndFeel(&lookAndFeel);
    mTitleLabel.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mTitleLabel);

    juce::String const version{ STRING(JUCE_APP_VERSION) };
    mVersionLabel.setText("Version " + version + "\n\n\n", juce::NotificationType::dontSendNotification);
    mVersionLabel.setJustificationType(juce::Justification::horizontallyCentred);
    mVersionLabel.setBounds(5, 180, 390, 50);
    mVersionLabel.setLookAndFeel(&lookAndFeel);
    mVersionLabel.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mVersionLabel);

    juce::String infos{};
    infos << "Developed by the G.R.I.S. at Université de Montréal\n\n";
    infos << "(Groupe de Recherche en Immersion Spatiale)\n\n\n";
    infos << "Director:\n\n";
    infos << "Robert NORMANDEAU\n\n\n";
    infos << "Programmers:\n\n";
    infos << "Actual: Samuel BÉLAND\n\n";
    infos << "Former: Olivier BÉLANGER, Vincent BERTHIAUME, Nicolas MASSON, Antoine MISSOUT\n\n\n";
    infos << "Assistants:\n\n";
    infos << "David LEDOUX, Christophe LENGELÉ, Nicola GIANNINI\n\n";

    mInfosLabel.setText(infos, juce::NotificationType::dontSendNotification);
    mInfosLabel.setJustificationType(juce::Justification::left);
    mInfosLabel.setBounds(5, 230, 390, 250);
    mInfosLabel.setFont(lookAndFeel.getFont());
    mInfosLabel.setLookAndFeel(&lookAndFeel);
    mInfosLabel.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mInfosLabel);

    mWebsiteHyperlink.setButtonText("GRIS Web Site");
    mWebsiteHyperlink.setURL(juce::URL{ "http://gris.musique.umontreal.ca/" });
    mWebsiteHyperlink.setColour(juce::HyperlinkButton::textColourId, lookAndFeel.getFontColour());
    mWebsiteHyperlink.setBounds(20, 470, 150, 22);
    mWebsiteHyperlink.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    mWebsiteHyperlink.setLookAndFeel(&lookAndFeel);
    mWebsiteHyperlink.addListener(this);
    addAndMakeVisible(mWebsiteHyperlink);

    mCloseButton.setButtonText("Close");
    mCloseButton.setBounds(250, 470, 100, 22);
    mCloseButton.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    mCloseButton.setLookAndFeel(&lookAndFeel);
    mCloseButton.addListener(this);
    addAndMakeVisible(mCloseButton);
}

//==============================================================================
AboutWindow::AboutWindow(juce::String const & name,
                         GrisLookAndFeel & lookAndFeel,
                         MainContentComponent & mainContentComponent)
    : juce::DocumentWindow(name, lookAndFeel.getBackgroundColour(), DocumentWindow::closeButton)
    , mMainContentComponent(mainContentComponent)
{
    setContentOwned(new AboutComponent{ *this, lookAndFeel }, true);

    centreWithSize(getContentComponent()->getWidth(), getContentComponent()->getHeight());
    setUsingNativeTitleBar(true);
    setResizable(false, false);
    setVisible(true);
}

//==============================================================================
void AboutWindow::closeButtonPressed()
{
    mMainContentComponent.closeAboutWindow();
}