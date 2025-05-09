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


namespace gris
{
SpeakerTreeComponent::SpeakerTreeComponent(juce::TreeViewItem * owner,
                                           const juce::ValueTree & v,
                                           juce::UndoManager & undoMan)
    : vt(v)
    , treeViewItem(owner)
    , undoManager (undoMan)
{
    // setLookAndFeel(&lnf);
    setInterceptsMouseClicks(false, true);

    auto const position = juce::VariantConverter<Position>::fromVar (v[CARTESIAN_POSITION]);
    auto const & polar{ position.getPolar() };

    setupDraggableEditor(x, Position::Coordinate::x);
    setupDraggableEditor(y, Position::Coordinate::y);
    setupDraggableEditor(z, Position::Coordinate::z);

    // TODO VB: all these will need some different logic from the above setupEditor
    setupEditor(azim, juce::String(polar.azimuth.get(), 3));
    setupEditor(elev, juce::String(polar.elevation.get(), 3));
    setupEditor(distance, juce::String(polar.length, 3));
    setupEditor(del, juce::String("DEL"));
    setupEditor(drag, juce::String("="));

    drag.setEditable(false);
    drag.setInterceptsMouseClicks(false, false);
}

void SpeakerTreeComponent::paint(juce::Graphics & g)
{
    if (treeViewItem->isSelected())
        g.fillAll(lnf.mHlBgcolor);
    else if (vt.getType() == SPEAKER_GROUP)
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
        for (auto * component : { &x, &y, &z, &azim, &elev, &distance, &gain, &highpass, &direct, &del, &drag })
            component->setBounds(bounds.removeFromLeft(otherColWidth));
    }
}

//static void getLegalSpeakerPosition (const float& valueModified,
//                                     float& valueToAdjust,
//                                     float& valueToTryToKeepIntact,
//                                     SpatMode const spatMode,
//                                     bool const isDirectOutOnly,
//                                     juce::Identifier property)
//{
//    auto const modifiedFloat { static_cast<float> (vt[valueModified]) };
//    auto const clampedModifiedFloat { std::clamp (modifiedFloat, -1.0f, 1.0f) };
//    auto const adjustedFloat { static_cast<float> (vt[valueToAdjust]) };
//    auto const intactFloat { static_cast<float> (vt[valueToTryToKeepIntact]) };
//
//    if (spatMode == SpatMode::mbap || isDirectOutOnly) {
//        auto const clamped { std::clamp (modifiedFloat, -MBAP_EXTENDED_RADIUS, MBAP_EXTENDED_RADIUS) };
//        vt.setProperty (valueModified, clamped, undoManager);
//        return;
//    }
//
//    // TODO VB
//    // if (modifiedCol == AZIMUTH || modifiedCol == Col::ELEVATION) {
//    //     return position.normalized ();
//    // }
//
//    vt.setProperty (valueModified, clampedModifiedFloat, undoManager);
//    auto const valueModified2 { clampedModifiedFloat * clampedModifiedFloat };
//    auto const lengthWithoutValueToAdjust { valueModified2 + intactFloat * intactFloat };
//
//    if (lengthWithoutValueToAdjust > 1.0f) {
//        auto const sign { intactFloat < 0.0f ? -1.0f : 1.0f };
//        auto const length { std::sqrt (1.0f - valueModified2) };
//        vt.setProperty (valueToTryToKeepIntact, sign * length, undoManager);
//        vt.setProperty (valueToAdjust, 0.f, undoManager);
//        return;
//    }
//
//    auto const sign { adjustedFloat < 0.0f ? -1.0f : 1.0f };
//    auto const length { std::sqrt (1.0f - lengthWithoutValueToAdjust) };
//    vt.setProperty (valueToAdjust, sign * length, undoManager);
//}

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
    position = getLegalSpeakerPosition (position, spatMode, vt[DIRECT_OUT_ONLY], coordinate);
    setPosition (position);
}

void SpeakerTreeComponent::setupDraggableEditor(DraggableLabel & label, Position::Coordinate coordinate)
{
    label.setEditable(true);

    label.setText(getPositionCoordinateTrimmedText(coordinate), juce::dontSendNotification);

    // Create and hold the listener to sync value -> label
    //auto * listener = new ValueToLabelListener(vt.getPropertyAsValue(identifier, &undoManager), label);
    //valueListeners.add(listener);

    label.onTextChange = [this, &label, coordinate] {
        setPositionCoordinate(coordinate, label.getText().getFloatValue());
        label.setText(getPositionCoordinateTrimmedText (coordinate), juce::dontSendNotification);
    };

    label.onMouseDragCallback = [this, &label, coordinate](int deltaY) {
        auto currentValue = label.getText().getFloatValue();
        auto newValue = currentValue - deltaY * 0.01f;
        setPositionCoordinate (coordinate, newValue);
        label.setText (getPositionCoordinateTrimmedText (coordinate), juce::dontSendNotification);
    };

    addAndMakeVisible(label);
}

void SpeakerTreeComponent::setupEditor(juce::Label & editor, juce::StringRef text)
{
    editor.setText(text, juce::dontSendNotification);
    editor.setEditable(true);
    addAndMakeVisible(editor);
}

tl::optional<SpatMode> SpeakerTreeComponent::getSpatMode() const
{
    auto parentTree = vt;
    while (parentTree.getParent().isValid())
        parentTree = parentTree.getParent();

    return stringToSpatMode (parentTree[SPAT_MODE].toString());
}

//==============================================================================

SpeakerGroupComponent::SpeakerGroupComponent(juce::TreeViewItem * owner,
                                             const juce::ValueTree & v,
                                             juce::UndoManager & undoManager)
    : SpeakerTreeComponent(owner, v, undoManager)
{
    setupEditor(id, ID);
}

//==============================================================================

SpeakerComponent::SpeakerComponent(juce::TreeViewItem * owner,
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