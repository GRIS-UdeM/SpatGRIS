/*
 This file is part of SpatGRIS2.

 Developers: Olivier Belanger, Nicolas Masson

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

#include <algorithm>

#include "ServerGrisConstants.h"
#include "UiComponent.h"
#include "LevelComponent.h"
#include "MainComponent.h"
#include "Speaker.h"

static double GetFloatPrecision(double value, double precision) {
    return (floor((value * pow(10, precision) + 0.5)) / pow(10, precision));
}

// ====================================== BOX ======================================
Box::Box(GrisLookAndFeel *feel, String title, bool verticalScrollbar, bool horizontalScrollbar) {
    this->title = title;
    this->grisFeel = feel;
    this->bgColour = this->grisFeel->getBackgroundColour();

    this->content = new Component();
    this->viewport = new Viewport();
    this->viewport->setViewedComponent(this->content, false);
    this->viewport->setScrollBarsShown(verticalScrollbar, horizontalScrollbar);
    this->viewport->setScrollBarThickness(15);
    this->viewport->getVerticalScrollBar().setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    this->viewport->getHorizontalScrollBar().setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());

    this->viewport->setLookAndFeel(this->grisFeel);
    addAndMakeVisible(this->viewport);
}

Box::~Box() {
    this->content->deleteAllChildren();
    delete this->viewport;
    delete this->content;
}

Component * Box::getContent() {
    return this->content ? this->content : this;
}

void Box::resized() {
    if (this->viewport) {
        this->viewport->setSize(getWidth(), getHeight());
    }
}

void Box::correctSize(unsigned int width, unsigned int height) {
    if (this->title != "") {
        this->viewport->setTopLeftPosition(0, 20);
        this->viewport->setSize(getWidth(), getHeight() - 20);
        if (width < 80) {
            width = 80;
        }
    } else {
        this->viewport->setTopLeftPosition(0, 0);
    }
    this->getContent()->setSize(width, height);
}

void Box::paint(Graphics &g) {
    g.setColour(this->bgColour);
    g.fillRect(getLocalBounds());
    if (this->title != "") {
        g.setColour(this->grisFeel->getWinBackgroundColour());
        g.fillRect(0, 0, getWidth(), 18);
        g.setColour(this->grisFeel->getFontColour());
        g.drawText(title, 0, 0, this->content->getWidth(), 20, juce::Justification::left);
    }
}

// ====================================== BOX CLIENT ========================================
BoxClient::BoxClient(MainContentComponent *parent, GrisLookAndFeel *feel) {
    this->mainParent = parent;
    this->grisFeel = feel;

    tableListClient.setModel(this);

    tableListClient.setColour(ListBox::outlineColourId, this->grisFeel->getWinBackgroundColour());
    tableListClient.setColour(ListBox::backgroundColourId, this->grisFeel->getWinBackgroundColour());
    tableListClient.setOutlineThickness(1);

    tableListClient.getHeader().addColumn("Client",    1, 120, 70, 120, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("Start",     2, 60, 35, 70, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("End",       3, 60, 35, 70, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("On/Off",    4, 62, 35, 70, TableHeaderComponent::notSortable);

    tableListClient.setMultipleSelectionEnabled (false);

    numRows = 0;
    tableListClient.updateContent();

    this->addAndMakeVisible(tableListClient);
}

BoxClient::~BoxClient() {}

void BoxClient::buttonClicked(Button *button) {
    this->mainParent->getLockClients().lock();
    bool connectedCli = !this->mainParent->getListClientjack().at(button->getName().getIntValue()).connected;
    this->mainParent->connectionClientJack(this->mainParent->getListClientjack().at(button->getName().getIntValue()).name, connectedCli);
    updateContentCli();
    this->mainParent->getLockClients().unlock();
}

void BoxClient::setBounds(int x, int y, int width, int height) {
    this->juce::Component::setBounds(x, y, width, height);
    tableListClient.setSize(width, height);
}

void BoxClient::updateContentCli() {
    numRows = (unsigned int)this->mainParent->getListClientjack().size();
    tableListClient.updateContent();
    tableListClient.repaint();
}

void BoxClient::setValue(const int rowNumber, const int columnNumber, const int newRating) {
    this->mainParent->getLockClients().lock();
    if (this->mainParent->getListClientjack().size() > (unsigned int)rowNumber) {
        switch (columnNumber) {
            case 2:
                this->mainParent->getListClientjack().at(rowNumber).portStart = newRating;
                this->mainParent->getListClientjack().at(rowNumber).initialized = true;
                break;
            case 3:
                this->mainParent->getListClientjack().at(rowNumber).portEnd = newRating;
                this->mainParent->getListClientjack().at(rowNumber).initialized = true;
                break;
        }
    }
    bool connectedCli = this->mainParent->getListClientjack().at(rowNumber).connected;
    this->mainParent->connectionClientJack(this->mainParent->getListClientjack().at(rowNumber).name, connectedCli);
    this->mainParent->getLockClients().unlock();
}

int BoxClient::getValue(const int rowNumber, const int columnNumber) const {
    if ((unsigned int)rowNumber < this->mainParent->getListClientjack().size()) {
        switch (columnNumber) {
            case 2:
                return this->mainParent->getListClientjack().at(rowNumber).portStart;
            case 3:
                return this->mainParent->getListClientjack().at(rowNumber).portEnd;
        }
    }
    return -1;
}

String BoxClient::getText(const int columnNumber, const int rowNumber) const {
    String text = "?";
    if ((unsigned int)rowNumber < this->mainParent->getListClientjack().size()) {
        if (columnNumber == 1) {
            text = String(this->mainParent->getListClientjack().at(rowNumber).name);
        }
    }
    return text;
}

int BoxClient::getNumRows() {
    return numRows;
}

void BoxClient::paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) {
    if (rowNumber % 2) {
        g.fillAll(this->grisFeel->getBackgroundColour().withBrightness(0.6));
    } else {
        g.fillAll(this->grisFeel->getBackgroundColour().withBrightness(0.7));
    }
}

void BoxClient::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) {
    g.setColour(Colours::black);
    g.setFont(12.0f);
    if (this->mainParent->getLockClients().try_lock()) {
        if ((unsigned int)rowNumber < this->mainParent->getListClientjack().size()) {
            if (columnId == 1) {
                String text = getText(columnId, rowNumber);
                g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
            }
        }
        this->mainParent->getLockClients().unlock();
    }
    g.setColour(Colours::black.withAlpha (0.2f));
    g.fillRect(width - 1, 0, 1, height);
}

Component * BoxClient::refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/,
                                               Component *existingComponentToUpdate) {
    if (columnId == 1) {
        return existingComponentToUpdate;
    }

    if (columnId == 4) {
        TextButton *tbRemove = static_cast<TextButton*> (existingComponentToUpdate);
        if (tbRemove == nullptr) {
            tbRemove = new TextButton();
            tbRemove->setName(String(rowNumber));
            tbRemove->setBounds(4, 404, 88, 22);
            tbRemove->addListener(this);
            tbRemove->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
            tbRemove->setLookAndFeel(this->grisFeel);
        }

        if (this->mainParent->getListClientjack().at(rowNumber).connected) {
            tbRemove->setButtonText("<->");
        } else {
            tbRemove->setButtonText("<X>");
        }

        return tbRemove;
    }

    ListIntOutComp *textLabel = static_cast<ListIntOutComp*> (existingComponentToUpdate);

    if (textLabel == nullptr)
        textLabel = new ListIntOutComp(*this);

    textLabel->setRowAndColumn(rowNumber, columnId);

    return textLabel;
}

// ============================ WindowProperties ===========================
Label * WindowProperties::createPropLabel(String lab, Justification::Flags just, int ypos, int width) {
    Label *label = new Label();
    label->setText(lab, NotificationType::dontSendNotification);
    label->setJustificationType(just);
    label->setBounds(10, ypos, width, 22);
    label->setFont(this->grisFeel->getFont());
    label->setLookAndFeel(this->grisFeel);
    label->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->juce::Component::addAndMakeVisible(label);
    return label;
}

TextEditor * WindowProperties::createPropIntTextEditor(String tooltip, int ypos, int init) {
    TextEditor *editor = new TextEditor();
    editor->setTooltip(tooltip);
    editor->setTextToShowWhenEmpty("", this->grisFeel->getOffColour());
    editor->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    editor->setLookAndFeel(this->grisFeel);
    editor->setBounds(130, ypos, 120, 22);
    editor->setInputRestrictions(5, "0123456789");
    editor->setText(String(init));
    /* Implemented but not yet in current Juce release. */
    //this->tedOSCInPort->setJustification(Justification::right);
    this->juce::Component::addAndMakeVisible(editor);
    return editor;
}

ComboBox * WindowProperties::createPropComboBox(const StringArray choices, int selected, int ypos) {
    ComboBox *combo = new ComboBox();
    combo->addItemList(choices, 1);
    combo->setSelectedItemIndex(selected);
    combo->setBounds(130, ypos, 120, 22);
    combo->setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(combo);
    return combo;
}

WindowProperties::WindowProperties( MainContentComponent *parent, GrisLookAndFeel *feel, Array<String> devices,
                                   String currentDevice, int indR, int indB, int indFF, int indFC, int indAttDB, int indAttHz, int oscPort)
{
    this->mainParent = parent;
    this->grisFeel = feel;

    int ypos = 20;

    this->generalLabel = this->createPropLabel("General Settings", Justification::left, ypos);
    ypos += 30;

    this->labOSCInPort = this->createPropLabel("OSC Input Port :", Justification::left, ypos);
    this->tedOSCInPort = this->createPropIntTextEditor("Port Socket OSC Input", ypos, oscPort);
    ypos += 40;

    this->jackSettingsLabel = this->createPropLabel("Jack Settings", Justification::left, ypos);
    ypos += 30;

    if (! devices.isEmpty()) {
        int deviceIndex = 0;
        if (devices.contains(currentDevice)) {
            deviceIndex = devices.indexOf(currentDevice);
        }
        this->labDevice = this->createPropLabel("Output Device :", Justification::left, ypos);
        this->cobDevice = this->createPropComboBox(devices, deviceIndex, ypos);
        ypos += 30;
    }

    this->labRate = this->createPropLabel("Sampling Rate (hz) :", Justification::left, ypos);
    this->cobRate = this->createPropComboBox(RateValues, indR, ypos);
    ypos += 30;

    this->labBuff = this->createPropLabel("Buffer Size (spls) :", Justification::left, ypos);
    this->cobBuffer = this->createPropComboBox(BufferSizes, indB, ypos);
    ypos += 40;

    this->recordingLabel = this->createPropLabel("Recording Settings", Justification::left, ypos);
    ypos += 30;

    this->labRecFormat = this->createPropLabel("File Format :", Justification::left, ypos);
    this->recordFormat = this->createPropComboBox(FileFormats, indFF, ypos);
    ypos += 30;

    this->labRecFileConfig = this->createPropLabel("Output Format :", Justification::left, ypos);
    this->recordFileConfig = this->createPropComboBox(FileConfigs, indFC, ypos);
    ypos += 40;

    this->cubeDistanceLabel = this->createPropLabel("CUBE Distance Settings", Justification::left, ypos, 250);
    ypos += 30;

    this->labDistanceDB = this->createPropLabel("Attenuation (dB) :", Justification::left, ypos);
    this->cobDistanceDB = this->createPropComboBox(AttenuationDBs, indAttDB, ypos);
    ypos += 30;

    this->labDistanceCutoff = this->createPropLabel("Attenuation (Hz) :", Justification::left, ypos);
    this->cobDistanceCutoff = this->createPropComboBox(AttenuationCutoffs, indAttHz, ypos);
    ypos += 40;

    this->butValidSettings = new TextButton();
    this->butValidSettings->setButtonText("Save");
    this->butValidSettings->setBounds(163, ypos, 88, 22);
    this->butValidSettings->addListener(this);
    this->butValidSettings->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->butValidSettings->setLookAndFeel(this->grisFeel);
    this->addAndMakeVisible(this->butValidSettings, false);
}

WindowProperties::~WindowProperties() {
    delete this->generalLabel;
    delete this->jackSettingsLabel;
    delete this->recordingLabel;
    delete this->labOSCInPort;
    if (this->cobDevice != nullptr) {
        delete this->labDevice;
        delete this->cobDevice;
    }
    delete this->labRate;
    delete this->labBuff;
    delete this->labRecFormat;
    delete this->labRecFileConfig;
    delete this->tedOSCInPort;
    delete this->cobRate;
    delete this->cobBuffer;
    delete this->recordFormat;
    delete this->recordFileConfig;
    delete this->butValidSettings;
    this->mainParent->destroyWindowProperties();
}

void WindowProperties::closeButtonPressed() {
    delete this;
}

void WindowProperties::buttonClicked(Button *button) {
    if (button == this->butValidSettings) {
        this->mainParent->saveProperties(this->cobDevice != nullptr ? this->cobDevice->getText() : String(),
                                         this->cobRate->getText().getIntValue(),
                                         this->cobBuffer->getText().getIntValue(),
                                         this->recordFormat->getSelectedItemIndex(),
                                         this->recordFileConfig->getSelectedItemIndex(),
                                         this->cobDistanceDB->getSelectedItemIndex(),
                                         this->cobDistanceCutoff->getSelectedItemIndex(),
                                         this->tedOSCInPort->getTextValue().toString().getIntValue());
        delete this;
    }
}


//======================================= About Window ===========================
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
    infos << "Actual: Olivier BÉLANGER\n\n";
    infos << "Former: Vincent BERTHIAUME, Nicolas MASSON, Antoine MISSOUT\n\n\n";
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

AboutWindow::~AboutWindow() {
    delete this->imageComponent;
    delete this->title;
    delete this->version;
    delete this->label;
    delete this->website;
    delete this->close;
    this->mainParent->destroyAboutWindow();
}

void AboutWindow::closeButtonPressed() {
    delete this;
}

void AboutWindow::buttonClicked(Button *button) {
    delete this;
}

//======================================= OSC Log Window ===========================
OscLogWindow::OscLogWindow(const String& name, Colour backgroundColour, int buttonsNeeded,
                           MainContentComponent *parent, GrisLookAndFeel *feel):
    DocumentWindow(name, backgroundColour, buttonsNeeded), logger (codeDocument, 0)
{
    this->mainParent = parent;
    this->grisFeel = feel;

    this->index = 0;
    this->activated = true;

    this->logger.setFont(this->logger.getFont().withPointHeight(this->logger.getFont().getHeightInPoints() + 3));
    this->logger.setBounds(5, 5, 490, 450);
    this->logger.setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->logger);

    this->stop.setButtonText("Stop");
    this->stop.setClickingTogglesState(true);
    this->stop.setToggleState(true, NotificationType::dontSendNotification);
    this->stop.setBounds(100, 470, 100, 22);
    this->stop.addListener(this);
    this->stop.setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->stop.setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->stop);

    this->close.setButtonText("Close");
    this->close.setBounds(300, 470, 100, 22);
    this->close.addListener(this);
    this->close.setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->close.setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->close);
}

OscLogWindow::~OscLogWindow() {
    this->mainParent->destroyOscLogWindow();
}

void OscLogWindow::addToLog(String msg) {
    if (this->activated) {
        this->index++;

        const MessageManagerLock mmLock;

        this->logger.insertTextAtCaret(msg);

        if (this->index == 500) {
            this->index = 0;
            this->logger.loadContent(String(""));
        }
    }
}

void OscLogWindow::closeButtonPressed() {
    this->stop.setButtonText("Start");
    this->activated = false;
    this->mainParent->destroyOscLogWindow();
    delete this;
}

void OscLogWindow::buttonClicked(Button *button) {
    if (button == &this->stop) {
        if (button->getToggleState()) {
            this->stop.setButtonText("Stop");
            this->activated = true;
        } else {
            this->stop.setButtonText("Start");
            this->activated = false;
        }
    } else {
        this->closeButtonPressed();
    }
}
