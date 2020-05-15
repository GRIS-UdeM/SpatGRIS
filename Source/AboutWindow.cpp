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
AboutWindow::AboutWindow(const String& name, Colour backgroundColour, int buttonsNeeded,
                         MainContentComponent *parent, GrisLookAndFeel *feel):
    DocumentWindow(name, backgroundColour, buttonsNeeded)
{
    this->mainParent = parent;
    this->grisFeel = feel;

    File fs = File(ServerGrisIconSmallFilePath);
    if (fs.exists()) {
        Image img = ImageFileFormat::loadFrom(fs);
        this->imageComponent = new ImageComponent("");
        this->imageComponent->setImage(img);
        this->imageComponent->setBounds(136, 5, 128, 127);
        this->juce::Component::addAndMakeVisible(this->imageComponent);
    }

    this->title = new Label("AboutBox_title");
    this->title->setText("SpatGRIS v2 - Sound Spatialization Tool\n\n",
                         NotificationType::dontSendNotification);
    this->title->setJustificationType(Justification::horizontallyCentred);
    this->title->setBounds(5, 150, 390, 50);
    this->title->setLookAndFeel(this->grisFeel);
    this->title->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->juce::Component::addAndMakeVisible(this->title);

    this->version = new Label("AboutBox_version");
    String version_num = STRING(JUCE_APP_VERSION);
    this->version->setText("Version " + version_num + "\n\n\n",
                           NotificationType::dontSendNotification);
    this->version->setJustificationType(Justification::horizontallyCentred);
    this->version->setBounds(5, 180, 390, 50);
    this->version->setLookAndFeel(this->grisFeel);
    this->version->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->juce::Component::addAndMakeVisible(this->version);

    String infos;
    infos << "Developed by the G.R.I.S. at Université de Montréal\n\n";
    infos << "(Groupe de Recherche en Immersion Spatiale)\n\n\n";
    infos << "Director:\n\n";
    infos << "Robert NORMANDEAU\n\n\n";
    infos << "Programmers:\n\n";
    infos << "Actual: Samuel Béland\n\n";
    infos << "Former: Olivier BÉLANGER, Vincent BERTHIAUME, Nicolas MASSON, Antoine MISSOUT\n\n\n";
    infos << "Assistants:\n\n";
    infos << "David LEDOUX, Christophe LENGELÉ, Nicola GIANNINI\n\n";

    this->label = new Label();
    this->label->setText(infos, NotificationType::dontSendNotification);
    this->label->setJustificationType(Justification::left);
    this->label->setBounds(5, 230, 390, 250);
    this->label->setFont(this->grisFeel->getFont());
    this->label->setLookAndFeel(this->grisFeel);
    this->label->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->juce::Component::addAndMakeVisible(this->label);

    this->website = new HyperlinkButton("GRIS Web Site", {"http://gris.musique.umontreal.ca/"});
    this->website->setColour(HyperlinkButton::textColourId, this->grisFeel->getFontColour());
    this->website->setBounds(20, 470, 150, 22);
    this->website->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->website->setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->website);

    this->close = new TextButton();
    this->close->setButtonText("Close");
    this->close->setBounds(250, 470, 100, 22);
    this->close->addListener(this);
    this->close->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->close->setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->close);
}

//==============================================================================
AboutWindow::~AboutWindow() {
    delete this->imageComponent;
    delete this->title;
    delete this->version;
    delete this->label;
    delete this->website;
    delete this->close;
    this->mainParent->closeAboutWindow();
}