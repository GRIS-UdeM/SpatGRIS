/*
 This file is part of SpatGRIS.

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

#include "SpeakerTreeComponent.hpp"
#include <Data/StrongTypes/sg_CartesianVector.hpp>
//#include "SpeakerSetupLine.hpp"

namespace gris
{
SpeakerTreeComponent::SpeakerTreeComponent(juce::TreeViewItem* owner, const juce::ValueTree& v, juce::UndoManager& undoMan)
    : speakerTreeVt(v)
    , speakerSetupVt (v.getRoot())
    , treeViewItem(owner)
    , undoManager (undoMan)
{
    speakerSetupVt.addListener(this);
    setInterceptsMouseClicks(false, true);

    auto const position = juce::VariantConverter<Position>::fromVar (v[CARTESIAN_POSITION]);
    auto const & polar{ position.getPolar() };

    setupDraggableEditor(x, Position::Coordinate::x);
    setupDraggableEditor(y, Position::Coordinate::y);
    setupDraggableEditor(z, Position::Coordinate::z);

    // TODO VB: all these will need some different logic from the above setupEditor
    setupDraggableEditor (azim, Position::Coordinate::azimuth);
    setupDraggableEditor (elev, Position::Coordinate::elevation);
    setupDraggableEditor (radius, Position::Coordinate::radius);

    setupEditor(del, juce::String("DEL"));
    setupEditor(drag, juce::String("="));

    drag.setEditable(false);
    drag.setInterceptsMouseClicks(false, false);

    updateEnabledLabels();
}

void SpeakerTreeComponent::paint(juce::Graphics & g)
{
    if (treeViewItem->isSelected())
        g.fillAll(lnf.mHlBgcolor);
    else if (speakerTreeVt.getType() == SPEAKER_GROUP)
        g.fillAll(lnf.mBackGroundAndFieldColour.darker(.5f));
    else if (treeViewItem->getIndexInParent() % 2 == 0)
        g.fillAll(lnf.mGreyColour);
}

void SpeakerTreeComponent::resized()
{
    constexpr auto fixedLeftColWidth{ 200 };
    constexpr auto otherColWidth{ 60 };

    if (auto * window = findParentComponentOfClass<juce::DocumentWindow>()) {
        auto bounds = getLocalBounds();

        // position the ID colum so it is a fixed width of fixedLeftColWidth fromt the left of the document window
        const auto windowOriginInThis = window->getLocalPoint(this, juce::Point<int>{ 0, 0 });
        const auto idColWidth = fixedLeftColWidth - windowOriginInThis.x;
        id.setBounds(bounds.removeFromLeft(idColWidth));

        // then position the other components with a fixed width of otherColWidth
        for (auto * component : { &x, &y, &z, &azim, &elev, &radius, &gain, &highpass, &direct, &del, &drag })
            component->setBounds(bounds.removeFromLeft(otherColWidth));
    }
}

static Position getLegalSpeakerPosition(Position const & position,
                                        SpatMode const spatMode,
                                        bool const isDirectOutOnly,
                                        Position::Coordinate const coordinate)
{
    if (spatMode == SpatMode::mbap || isDirectOutOnly) {
        return Position{ position.getCartesian().clampedToFarField() };
    }

    if (coordinate == Position::Coordinate::azimuth || coordinate == Position::Coordinate::elevation) {
        return position.normalized();
    }

    static auto const clampCartesianPosition
        = [](float const& valueModified, float& valueToAdjust, float& valueToTryToKeepIntact) {
        auto const valueModified2 { valueModified * valueModified };
        auto const lengthWithoutValueToAdjust { valueModified2 + valueToTryToKeepIntact * valueToTryToKeepIntact };

        if (lengthWithoutValueToAdjust > 1.0f) {
            auto const sign { valueToTryToKeepIntact < 0.0f ? -1.0f : 1.0f };
            auto const length { std::sqrt (1.0f - valueModified2) };
            valueToTryToKeepIntact = sign * length;
            valueToAdjust = 0.0f;
            return;
        }

        auto const sign { valueToAdjust < 0.0f ? -1.0f : 1.0f };
        auto const length { std::sqrt (1.0f - lengthWithoutValueToAdjust) };
        valueToAdjust = sign * length;
        };

    auto newPosition{ position.getCartesian() };

    auto & x{ newPosition.x };
    auto & y{ newPosition.y };
    auto & z{ newPosition.z };
    if (coordinate == Position::Coordinate::x) {
        x = std::clamp(x, -1.0f, 1.0f);
        clampCartesianPosition(x, y, z);
    } else if (coordinate == Position::Coordinate::y) {
        y = std::clamp(y, -1.0f, 1.0f);
        clampCartesianPosition(y, x, z);
    } else {
        jassert(coordinate == Position::Coordinate::z);
        z = std::clamp(z, -1.0f, 1.0f);
        clampCartesianPosition(z, x, y);
    }
    return Position{ newPosition };
}


float SpeakerTreeComponent::getPositionCoordinate (Position::Coordinate coordinate)
{
    auto const position { getPosition () };

    switch (coordinate) {
    case Position::Coordinate::x:
        return position.getCartesian ().x;
    case Position::Coordinate::y:
        return position.getCartesian ().y;
    case Position::Coordinate::z:
        return position.getCartesian ().z;
    case Position::Coordinate::azimuth:
        return position.getPolar ().azimuth.get();
    case Position::Coordinate::elevation:
        return position.getPolar ().elevation.get ();
    case Position::Coordinate::radius:
        return position.getPolar ().length;
    default:
        jassertfalse;
        return {};
    };
}

juce::String SpeakerTreeComponent::getPositionCoordinateTrimmedText (Position::Coordinate coordinate)
{
    auto const position { getPosition () };

    switch (coordinate) {
    case Position::Coordinate::x:
        return juce::String (position.getCartesian ().x, 3);
    case Position::Coordinate::y:
        return juce::String (position.getCartesian ().y, 3);
    case Position::Coordinate::z:
        return juce::String (position.getCartesian ().z, 3);
    case Position::Coordinate::azimuth:
        return juce::String (position.getPolar ().azimuth.get (), 3);
    case Position::Coordinate::elevation:
        return juce::String (position.getPolar ().elevation.get (), 3);
    case Position::Coordinate::radius:
        return juce::String (position.getPolar ().length, 3);
    default:
        jassertfalse;
        return juce::String ();
    };
}

void SpeakerTreeComponent::setPositionCoordinate (Position::Coordinate coordinate, float newValue)
{
    // get the current position and update the coordinate
    auto position = [position{ getPosition() }, coordinate, newValue](){
        switch (coordinate) {
        case Position::Coordinate::x:
            return position.withX(newValue);
        case Position::Coordinate::y:
            return position.withY(newValue);
        case Position::Coordinate::z:
            return position.withZ(newValue);
        case Position::Coordinate::azimuth:
            return Position {position.getPolar ().withAzimuth (radians_t (newValue))};
        case Position::Coordinate::elevation:
            return  Position { position.getPolar ().withElevation (radians_t (newValue)) };
        case Position::Coordinate::radius:
            return Position{ position.getPolar().withRadius(newValue) };
        default:
            jassertfalse;
            return position;
        }
    }();

    // clamp the position to a legal value and set it back
    auto const spatMode {getSpatMode().value_or(SpatMode::mbap)};
    position = getLegalSpeakerPosition (position, spatMode, speakerTreeVt[DIRECT_OUT_ONLY], coordinate);
    setPosition (position);
}

void SpeakerTreeComponent::setupDraggableEditor(DraggableLabel & label, Position::Coordinate coordinate)
{
    label.setEditable(true);
    label.setText(getPositionCoordinateTrimmedText(coordinate), juce::dontSendNotification);

    label.onTextChange = [this, &label, coordinate] {
        setPositionCoordinate(coordinate, label.getText().getFloatValue());
        updateAllPositionLabels ();
    };

    label.onMouseDragCallback = [this, &label, coordinate](int deltaY) {
        auto currentValue = label.getText().getFloatValue();
        auto newValue = currentValue - deltaY * 0.01f;
        setPositionCoordinate (coordinate, newValue);
        updateAllPositionLabels ();
    };

    addAndMakeVisible(label);
}

void SpeakerTreeComponent::setupEditor(juce::Label & editor, juce::StringRef text)
{
    editor.setText(text, juce::dontSendNotification);
    editor.setEditable(true);
    addAndMakeVisible(editor);
}

void SpeakerTreeComponent::updateAllPositionLabels ()
{
    x.setText(getPositionCoordinateTrimmedText(Position::Coordinate::x), juce::dontSendNotification);
    y.setText(getPositionCoordinateTrimmedText(Position::Coordinate::y), juce::dontSendNotification);
    z.setText(getPositionCoordinateTrimmedText(Position::Coordinate::z), juce::dontSendNotification);
    azim.setText(getPositionCoordinateTrimmedText(Position::Coordinate::azimuth), juce::dontSendNotification);
    elev.setText(getPositionCoordinateTrimmedText(Position::Coordinate::elevation), juce::dontSendNotification);
    radius.setText(getPositionCoordinateTrimmedText(Position::Coordinate::radius), juce::dontSendNotification);
}

tl::optional<SpatMode> SpeakerTreeComponent::getSpatMode() const
{
    return stringToSpatMode (speakerTreeVt.getRoot ().getProperty(SPAT_MODE).toString());
}

void SpeakerTreeComponent::valueTreePropertyChanged (juce::ValueTree& valueTree, const juce::Identifier& property)
{
    //only interested in this tree and property for now
    if (valueTree == speakerSetupVt && property == SPAT_MODE)
        updateEnabledLabels();
}

void SpeakerTreeComponent::updateEnabledLabels ()
{
    if (auto const spatMode {getSpatMode()}; spatMode == SpatMode::vbap)
    {
        x.setEnabled (false);
        y.setEnabled (false);
        z.setEnabled (false);
        azim.setEnabled (true);
        elev.setEnabled (true);
        radius.setEnabled (true);
    }
    else if (spatMode == SpatMode::mbap)
    {
        x.setEnabled (true);
        y.setEnabled (true);
        z.setEnabled (true);
        azim.setEnabled (false);
        elev.setEnabled (false);
        radius.setEnabled (false);
    }
    else
    {
        x.setEnabled (true);
        y.setEnabled (true);
        z.setEnabled (true);
        azim.setEnabled (true);
        elev.setEnabled (true);
        radius.setEnabled (true);
    }
}

//==============================================================================

SpeakerGroupComponent::SpeakerGroupComponent(juce::TreeViewItem* owner,
                                             const juce::ValueTree & v,
                                             juce::UndoManager & undoManager)
    : SpeakerTreeComponent(owner, v, undoManager)
{
    setupEditor(id, ID);
}

//==============================================================================

SpeakerComponent::SpeakerComponent(juce::TreeViewItem* owner,
                                   const juce::ValueTree & v,
                                   juce::UndoManager & undoManager)
    : SpeakerTreeComponent(owner, v, undoManager)
{
    // TODO VB: this is super weird because all these components belong to the parent. We probably need some virtual
    // function to call in resized where we get the list of components that are in children
    setupEditor(id, ID);
    setupEditor(gain, GAIN);
    setupEditor(highpass, FREQ);
    setupEditor(direct, DIRECT_OUT_ONLY);
}
} // namespace gris