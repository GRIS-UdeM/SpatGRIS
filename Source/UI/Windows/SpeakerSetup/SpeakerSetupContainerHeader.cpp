#include "SpeakerSetupContainerHeader.hpp"
#include "SpeakerTreeComponent.hpp"
#include <array>

namespace gris
{

void SpeakerColumnHeader::paint(juce::Graphics & g)
{
    // start by painting the label
    juce::Label::paint(g);
    if (sortState == SortState::none) {
        return;
    }

    // paints the little arrow thingy if necessary.
    g.setColour(arrowColor);
    juce::Path sortTriangle;
    const auto middle = getLocalBounds().getHeight() / 2;
    constexpr auto triangleRightPad = 4.f;
    constexpr auto triangleWidth = 10.f;
    constexpr auto triangleHeight = 4.f;
    if (sortState == SortState::ascending)
        sortTriangle.addTriangle(width - triangleRightPad - triangleWidth,
                                 middle + triangleHeight,
                                 width - triangleRightPad,
                                 middle + triangleHeight,
                                 width - triangleRightPad - (triangleWidth / 2),
                                 middle - triangleHeight);
    else
        sortTriangle.addTriangle(width - triangleRightPad - triangleWidth,
                                 middle - triangleHeight,
                                 width - triangleRightPad,
                                 middle - triangleHeight,
                                 width - triangleRightPad - (triangleWidth / 2),
                                 middle + triangleHeight);
    g.fillPath(sortTriangle);
}

void SpeakerColumnHeader::mouseUp (const juce::MouseEvent&) {
    // Basically state transition (and repaint) + sort callback for the parent.
    switch (sortState) {
        case SortState::none:
            setState(SortState::ascending);
            break;
        case SortState::ascending:
            setState(SortState::descending);
            break;
        case SortState::descending:
            setState(SortState::ascending);
            break;
    }
    sortCallback(this, sortState);
}

void SpeakerColumnHeader::setState(const SortState newState) {
    sortState = newState;
    // repaint to show the changed sort arrow
    repaint();
}

void SpeakerColumnHeader::setSortCallback(std::function<void(const SpeakerColumnHeader*, const SortState)> callback) {
    sortCallback = callback;
}

SpeakerSetupContainerHeader::SpeakerSetupContainerHeader(GrisLookAndFeel& glaf)

    : grisLookAndFeel(glaf)
{
    auto setHeaderText = [this](juce::Label& label, const juce::String & text) {
        label.setColour (juce::Label::ColourIds::outlineColourId, grisLookAndFeel.mLightColour.withAlpha (.25f));
        label.setText(text, juce::dontSendNotification);
        addAndMakeVisible(label);
    };

    auto disableOtherHeaders = [this](const SpeakerColumnHeader* clickedHeader) {
        for (const auto header : {&id, &x, &y, &z, &azim, &elev, &distance}) {
            if (clickedHeader != header) {
                header->setState(SpeakerColumnHeader::SortState::none);
            }
        }
    };
    auto makeSortFunction = [this, disableOtherHeaders](SpeakerColumnHeader::ColumnID sortID) {
        return [this, sortID, disableOtherHeaders](const SpeakerColumnHeader* clickedHeader, const SpeakerColumnHeader::SortState sortState) {
            if (sortState == SpeakerColumnHeader::SortState::none) {
                return;
            }
            int sortDirection = sortState == SpeakerColumnHeader::SortState::ascending ? 1 : -1;
            sortFunc(sortID, sortDirection);
            disableOtherHeaders(clickedHeader);
        };
    };
    setHeaderText (id, "ID");
    constexpr auto realIdWidth = SpeakerTreeComponent::fixedLeftColWidth-28;
    id.width = realIdWidth;
    id.arrowColor = grisLookAndFeel.mOnColor;
    id.setSortCallback(makeSortFunction(SpeakerColumnHeader::ColumnID::ID));
    setHeaderText (x, "X");
    setHeaderText (y, "Y");
    setHeaderText (z, "Z");
    setHeaderText (azim, "Azimuth");
    setHeaderText (elev, "Elevation");
    setHeaderText (distance, "Distance");
    setHeaderText (gain, "Gain");
    setHeaderText (highpass, "Highpass");
    setHeaderText (direct, "Direct");
    setHeaderText (del, "Delete");
    constexpr std::array<SpeakerColumnHeader::ColumnID, 8> headerKeys = {
        SpeakerColumnHeader::ColumnID::X,
        SpeakerColumnHeader::ColumnID::Y,
        SpeakerColumnHeader::ColumnID::Z,
        SpeakerColumnHeader::ColumnID::Azimuth,
        SpeakerColumnHeader::ColumnID::Elevation,
        SpeakerColumnHeader::ColumnID::Distance,
        SpeakerColumnHeader::ColumnID::Gain,
        SpeakerColumnHeader::ColumnID::Highpass,
    };
    size_t i = 0;
    for (const auto header : { &x, &y, &z, &azim, &elev, &distance, &gain, &highpass}) {
        header->width = SpeakerTreeComponent::otherColWidth;
        header->arrowColor = grisLookAndFeel.mOnColor;
        header->setSortCallback(makeSortFunction(headerKeys[i]));
        i++;
    }
    setHeaderText (drag, "Drag");
}

void SpeakerSetupContainerHeader::resized() {
    auto height = getLocalBounds().getHeight();
    id.setBounds(0, 0, id.width, height);
    auto currentX = id.width;
    for (const auto header : { &x, &y, &z, &azim, &elev, &distance, &gain, &highpass }) {
        header->setBounds(currentX, 0, header->width, height);
        currentX += header->width;
    }
    for (const auto label : { &direct, &del }) {
        label->setBounds(currentX, 0, SpeakerTreeComponent::otherColWidth, height);
        currentX += SpeakerTreeComponent::otherColWidth;
    }
    constexpr auto dragColWidth{ 40 };
    drag.setBounds(currentX, 0, dragColWidth, height);
}

void SpeakerSetupContainerHeader::setSortFunc(std::function<void(SpeakerColumnHeader::ColumnID, int sortDirection)> sort) {
    sortFunc = sort;
}
}
