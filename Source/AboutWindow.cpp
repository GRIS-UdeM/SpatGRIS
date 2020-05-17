/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Nicolas Masson

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

//==============================================================================
AboutComponent::AboutComponent(AboutWindow & aboutWindow, GrisLookAndFeel & lookAndFeel) : mParentWindow(aboutWindow)
{
    this->setSize(400, 500);

    juce::File fs{ ServerGrisIconSmallFilePath };
    if (fs.exists()) {
        juce::Image img = juce::ImageFileFormat::loadFrom(fs);
        this->mLogoImage.setImage(img);
        this->mLogoImage.setBounds(136, 5, 128, 127);
        this->addAndMakeVisible(this->mLogoImage);
    }

    this->mTitleLabel.setText("SpatGRIS v2 - Sound Spatialization Tool\n\n", NotificationType::dontSendNotification);
    this->mTitleLabel.setJustificationType(Justification::horizontallyCentred);
    this->mTitleLabel.setBounds(5, 150, 390, 50);
    this->mTitleLabel.setLookAndFeel(&lookAndFeel);
    this->mTitleLabel.setColour(Label::textColourId, lookAndFeel.getFontColour());
    this->addAndMakeVisible(this->mTitleLabel);

    String version_num = STRING(JUCE_APP_VERSION);
    this->mVersionLabel.setText("Version " + version_num + "\n\n\n", NotificationType::dontSendNotification);
    this->mVersionLabel.setJustificationType(Justification::horizontallyCentred);
    this->mVersionLabel.setBounds(5, 180, 390, 50);
    this->mVersionLabel.setLookAndFeel(&lookAndFeel);
    this->mVersionLabel.setColour(Label::textColourId, lookAndFeel.getFontColour());
    this->addAndMakeVisible(this->mVersionLabel);

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

    this->mInfosLabel.setText(infos, NotificationType::dontSendNotification);
    this->mInfosLabel.setJustificationType(Justification::left);
    this->mInfosLabel.setBounds(5, 230, 390, 250);
    this->mInfosLabel.setFont(lookAndFeel.getFont());
    this->mInfosLabel.setLookAndFeel(&lookAndFeel);
    this->mInfosLabel.setColour(Label::textColourId, lookAndFeel.getFontColour());
    this->addAndMakeVisible(this->mInfosLabel);

    this->mWebsiteHyperlink.setButtonText("GRIS Web Site");
    this->mWebsiteHyperlink.setURL(juce::URL{ "http://gris.musique.umontreal.ca/" });
    this->mWebsiteHyperlink.setColour(HyperlinkButton::textColourId, lookAndFeel.getFontColour());
    this->mWebsiteHyperlink.setBounds(20, 470, 150, 22);
    this->mWebsiteHyperlink.setColour(ToggleButton::textColourId, lookAndFeel.getFontColour());
    this->mWebsiteHyperlink.setLookAndFeel(&lookAndFeel);
    this->mWebsiteHyperlink.addListener(this);
    this->addAndMakeVisible(this->mWebsiteHyperlink);

    this->mCloseButton.setButtonText("Close");
    this->mCloseButton.setBounds(250, 470, 100, 22);
    this->mCloseButton.setColour(ToggleButton::textColourId, lookAndFeel.getFontColour());
    this->mCloseButton.setLookAndFeel(&lookAndFeel);
    this->mCloseButton.addListener(this);
    this->addAndMakeVisible(mCloseButton);
}

//==============================================================================
AboutWindow::AboutWindow(juce::String const &   name,
                         GrisLookAndFeel &      lookAndFeel,
                         MainContentComponent & mainContentComponent)
    : juce::DocumentWindow(name, lookAndFeel.getBackgroundColour(), DocumentWindow::closeButton)
    , mMainContentComponent(mainContentComponent)
{
    this->setContentOwned(new AboutComponent{ *this, lookAndFeel }, true);

    this->centreWithSize(this->getContentComponent()->getWidth(), this->getContentComponent()->getHeight());
    this->setUsingNativeTitleBar(true);
    this->setResizable(false, false);
    this->setVisible(true);
}

//==============================================================================
void AboutWindow::closeButtonPressed()
{
    this->mMainContentComponent.closeAboutWindow();
}