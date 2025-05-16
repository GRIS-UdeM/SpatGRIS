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
#include "SpeakerSetupLine.hpp"
#include <Data/StrongTypes/sg_CartesianVector.hpp>

namespace gris
{
SpeakerTreeComponent::SpeakerTreeComponent(SpeakerSetupLine * owner,
                                           const juce::ValueTree & v,
                                           juce::UndoManager & undoMan)
    : speakerTreeVt(v)
    , speakerSetupVt (v.getRoot())
    , speakerSetupLine(owner)
    , undoManager (undoMan)
{
    speakerSetupVt.addListener(this);
    setInterceptsMouseClicks(false, true);

    //TODO VB: anything special to do about this wrt to the OG window?
    //actually yep, look for `} else if (property == ID) {`
    setupEditorLabel (id, ID);

    setupCoordinateLabel(x, Position::Coordinate::x);
    setupCoordinateLabel(y, Position::Coordinate::y);
    setupCoordinateLabel(z, Position::Coordinate::z);
    setupCoordinateLabel(azim, Position::Coordinate::azimuth);
    setupCoordinateLabel(elev, Position::Coordinate::elevation);
    setupCoordinateLabel(radius, Position::Coordinate::radius);

    setupDeleteButton ();

    setupStringLabel(drag, juce::String("="));

    drag.setEditable(false);
    drag.setInterceptsMouseClicks(false, false);

    updateEnabledLabels();
}

void SpeakerTreeComponent::setupDeleteButton()
{
    if (auto trashDrawable = juce::Drawable::createFromImageData(BinaryData::trash_svg, BinaryData::trash_svgSize))
        deleteButton.setImages(trashDrawable.get());
    else
        jassertfalse;

    deleteButton.setClickingTogglesState(false);

    deleteButton.onClick = [this]() {
        auto parent = speakerTreeVt.getParent();

        if (speakerTreeVt.getType() == SPEAKER) {
            parent.removeChild(speakerTreeVt, &undoManager);
        } else if (speakerTreeVt.getType() == SPEAKER_GROUP) {
            SpeakerSetupLine::isDeletingGroup = true;

            auto const numSpeakers { speakerTreeVt.getNumChildren()};
            if (numSpeakers >= 1) {
                //remove all but the first child
                for (int i = numSpeakers - 1; i >= 1; --i)
                    speakerTreeVt.removeChild (i, &undoManager);

                SpeakerSetupLine::isDeletingGroup = false;

                //this last one will trigger a call to MainContentComponent::refreshSpeakers()
                speakerTreeVt.removeChild (0, &undoManager);

                //and finally delete the group
                parent.removeChild (speakerTreeVt, &undoManager);
            }
        }
    };

    addAndMakeVisible(deleteButton);
}

void SpeakerTreeComponent::paint(juce::Graphics & g)
{
    if (speakerSetupLine->isSelected())
        g.fillAll(lnf.mHlBgcolor);
    else if (speakerTreeVt.getType() == SPEAKER_GROUP)
        g.fillAll(lnf.mBackGroundAndFieldColour.darker(.5f));
    else if (speakerSetupLine->getIndexInParent() % 2 == 0)
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
        std::vector<juce::Component*> components = { &x, &y, &z, &azim, &elev, &radius, &gain, &highpass, &direct, &deleteButton, &drag };
        for (auto* component : components)
            component->setBounds (bounds.removeFromLeft (otherColWidth));
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

void SpeakerTreeComponent::setupCoordinateLabel(DraggableLabel & label, Position::Coordinate coordinate)
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

void SpeakerTreeComponent::setupStringLabel (juce::Label & label, juce::StringRef text)
{
    label.setText(text, juce::dontSendNotification);
    addAndMakeVisible(label);
}

void SpeakerTreeComponent::setupEditorLabel (juce::Label& label, juce::Identifier property)
{
    label.setEditable (true);
    label.getTextValue().referTo(speakerTreeVt.getPropertyAsValue(property, &undoManager));
    addAndMakeVisible (label);
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
    if ((valueTree == speakerSetupVt && property == SPAT_MODE)
        || (valueTree == speakerTreeVt && property == DIRECT_OUT_ONLY))
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
        radius.setEnabled (speakerTreeVt[DIRECT_OUT_ONLY]);
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
        radius.setEnabled (false);
    }
}

//==============================================================================

SpeakerGroupComponent::SpeakerGroupComponent(SpeakerSetupLine* owner,
                                             const juce::ValueTree & v,
                                             juce::UndoManager & undoMan)
    : SpeakerTreeComponent(owner, v, undoMan)
{
}

//==============================================================================

SpeakerComponent::SpeakerComponent(SpeakerSetupLine* owner,
                                   const juce::ValueTree & v,
                                   juce::UndoManager & undoMan)
    : SpeakerTreeComponent(owner, v, undoMan)
{
    setupGain();

    setupHighPass ();

    direct.getToggleStateValue ().referTo (speakerTreeVt.getPropertyAsValue (DIRECT_OUT_ONLY, &undoManager));
    addAndMakeVisible (direct);
}

void SpeakerComponent::setupGain ()
{
    gain.setEditable (true);
    gain.setText (speakerTreeVt[GAIN], juce::dontSendNotification);

    const auto getClampedGain = [](float gainValue) {
        static constexpr dbfs_t MIN_GAIN { -18.0f };
        static constexpr dbfs_t MAX_GAIN { 6.0f };
        return std::clamp (dbfs_t (gainValue), MIN_GAIN, MAX_GAIN);
        };

    gain.onTextChange = [this, getClampedGain] {
        auto const clampedValue = getClampedGain (gain.getText ().getFloatValue ()).get ();

        gain.setText (juce::String (clampedValue, 1), juce::dontSendNotification);
        speakerTreeVt.setProperty (GAIN, clampedValue, &undoManager);
        };

    gain.onMouseDragCallback = [this, getClampedGain](int deltaY) {
        auto const draggedValue { gain.getText ().getFloatValue () - deltaY * 0.05f };
        auto const clampedValue { getClampedGain (draggedValue).get () };

        gain.setText (juce::String (clampedValue, 1), juce::dontSendNotification);
        speakerTreeVt.setProperty (GAIN, clampedValue, &undoManager);
        };

    addAndMakeVisible (gain);
}

void SpeakerComponent::setupHighPass ()
{
    highpass.setEditable(true);
    highpass.setText(speakerTreeVt[FREQ], juce::dontSendNotification);

    const auto getClampedFrequency = [](float freqValue, bool isIncreasing = false) {
        static constexpr hz_t OFF_FREQ{ 0.0f };
        static constexpr hz_t MIN_FREQ{ 20.0f };
        static constexpr hz_t MAX_FREQ{ 150.0f };

        if (hz_t(freqValue) < MIN_FREQ)
            return isIncreasing ? MIN_FREQ : OFF_FREQ;

        return std::clamp(hz_t(freqValue), MIN_FREQ, MAX_FREQ);
    };

    highpass.onTextChange = [this, getClampedFrequency] {
        auto const clampedValue = getClampedFrequency(highpass.getText().getFloatValue()).get();

        highpass.setText(juce::String(clampedValue, 1), juce::dontSendNotification);
        speakerTreeVt.setProperty(FREQ, clampedValue, &undoManager);
    };

    highpass.onMouseDragCallback = [this, getClampedFrequency](int deltaY) {
        auto const draggedValue{ highpass.getText().getFloatValue() - deltaY * 0.5f };
        auto const clampedValue{ getClampedFrequency(draggedValue, deltaY < 0).get() };

        highpass.setText(juce::String(clampedValue, 1), juce::dontSendNotification);
        speakerTreeVt.setProperty(FREQ, clampedValue, &undoManager);
    };

    addAndMakeVisible(highpass);
}

} // namespace gris
