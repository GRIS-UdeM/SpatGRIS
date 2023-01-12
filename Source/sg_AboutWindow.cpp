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

#include "sg_AboutWindow.hpp"

#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"
#include "sg_constants.hpp"

namespace gris
{
//==============================================================================
AboutComponent::AboutComponent(AboutWindow & parentWindow, GrisLookAndFeel & lookAndFeel) : mParentWindow(parentWindow)
{
    setSize(400, 550);

    auto const & icon{ ICON_SMALL_FILE };
    if (icon.exists()) {
        auto const img{ juce::ImageFileFormat::loadFrom(icon) };
        mLogoImage.setImage(img);
        mLogoImage.setBounds(136, 5, 128, 127);
        addAndMakeVisible(mLogoImage);
    }

    mTitleLabel.setText("SpatGRIS 3 - Sound Spatialization Tool", juce::NotificationType::dontSendNotification);
    mTitleLabel.setJustificationType(juce::Justification::horizontallyCentred);
    mTitleLabel.setBounds(5, 150, 390, 50);
    mTitleLabel.setLookAndFeel(&lookAndFeel);
    mTitleLabel.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mTitleLabel);

    auto const version{ juce::JUCEApplication::getInstance()->getApplicationVersion() };
    mVersionLabel.setText("Version " + version, juce::NotificationType::dontSendNotification);
    mVersionLabel.setJustificationType(juce::Justification::horizontallyCentred);
    mVersionLabel.setBounds(5, 180, 390, 50);
    mVersionLabel.setLookAndFeel(&lookAndFeel);
    mVersionLabel.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mVersionLabel);

    juce::String infos{};
    infos << "Developed by the G.R.I.S. at Université de Montréal\n";
    infos << "(Groupe de Recherche en Immersion Spatiale)\n\n";
    infos << "Director:\n";
    infos << "Robert NORMANDEAU\n\n";
    infos << "Programmers:\n";
    infos << "Actual: Gaël LANE LÉPINE\n";
    infos << "Former: Samuel BÉLAND, Olivier BÉLANGER, Vincent BERTHIAUME, Nicolas MASSON, Antoine MISSOUT\n\n";
    infos << "Assistants:\n";
    infos << "Mélanie FRISOLI, Nicola GIANNINI, David PIAZZA";

    mInfosLabel.setText(infos, juce::NotificationType::dontSendNotification);
    mInfosLabel.setJustificationType(juce::Justification::left);
    mInfosLabel.setBounds(5, 230, 390, 300);
    mInfosLabel.setFont(lookAndFeel.getFont());
    mInfosLabel.setLookAndFeel(&lookAndFeel);
    mInfosLabel.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mInfosLabel);

    mWebsiteHyperlink.setButtonText("GRIS Web Site");
    mWebsiteHyperlink.setURL(juce::URL{ "http://gris.musique.umontreal.ca/" });
    mWebsiteHyperlink.setColour(juce::HyperlinkButton::textColourId, lookAndFeel.getFontColour());
    mWebsiteHyperlink.setBounds(20, 520, 150, 22);
    mWebsiteHyperlink.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    mWebsiteHyperlink.setLookAndFeel(&lookAndFeel);
    mWebsiteHyperlink.addListener(this);
    addAndMakeVisible(mWebsiteHyperlink);

    mCloseButton.setButtonText("Close");
    mCloseButton.setBounds(250, 520, 100, 22);
    mCloseButton.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    mCloseButton.setLookAndFeel(&lookAndFeel);
    mCloseButton.addListener(this);
    addAndMakeVisible(mCloseButton);
}

//==============================================================================
AboutWindow::AboutWindow(juce::String const & name,
                         GrisLookAndFeel & lookAndFeel,
                         MainContentComponent & mainContentComponent)
    : DocumentWindow(name, lookAndFeel.getBackgroundColour(), closeButton)
    , mMainContentComponent(mainContentComponent)
{
    setContentOwned(new AboutComponent{ *this, lookAndFeel }, true);

    centreWithSize(getContentComponent()->getWidth(), getContentComponent()->getHeight());
    setUsingNativeTitleBar(true);
    setResizable(false, false);
    Component::setVisible(true);
}

//==============================================================================
void AboutWindow::closeButtonPressed()
{
    mMainContentComponent.closeAboutWindow();
}

} // namespace gris