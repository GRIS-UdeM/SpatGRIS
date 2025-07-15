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
#include "Data/sg_LogicStrucs.hpp"
#include "Data/StrongTypes/sg_CartesianVector.hpp"

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

    setupCoordinateLabel(x, Position::Coordinate::x);
    setupCoordinateLabel(y, Position::Coordinate::y);
    setupCoordinateLabel(z, Position::Coordinate::z);
    setupCoordinateLabel(azim, Position::Coordinate::azimuth);
    setupCoordinateLabel(elev, Position::Coordinate::elevation);
    setupCoordinateLabel(distance, Position::Coordinate::radius);

    setupDeleteButton ();

    setupStringLabel(drag, juce::String("="));

    drag.setEditable(false);
    drag.setInterceptsMouseClicks(false, false);

    updateUiBasedOnSpatMode();
}

void SpeakerTreeComponent::setupDeleteButton()
{
    if (auto trashDrawable = juce::Drawable::createFromImageData(BinaryData::trash_svg, BinaryData::trash_svgSize))
        deleteButton.setImages(trashDrawable.get());
    else
        jassertfalse;

    deleteButton.setClickingTogglesState(false);

    deleteButton.onClick = [this]() {
        this->deleteButtonBehaviour();
    };

    addAndMakeVisible(deleteButton);
}

void SpeakerTreeComponent::resized()
{
    if (auto * window = findParentComponentOfClass<juce::DocumentWindow>()) {
        auto bounds = getLocalBounds();

        // position the ID colum so it is a fixed width of fixedLeftColWidth fromt the left of the document window
        const auto windowOriginInThis = window->getLocalPoint(this, juce::Point<int>{ 0, 0 });
        const auto idColWidth = fixedLeftColWidth - windowOriginInThis.x;
        id.setBounds(bounds.removeFromLeft(idColWidth - colGap));
        bounds.removeFromLeft (colGap);

        // then position the other components with a fixed width of otherColWidth
        std::vector<juce::Component*> components = { &x, &y, &z, &azim, &elev, &distance, &gain, &highpass, &direct, &deleteButton, &drag };
        for (auto* component : components)
        {
            component->setBounds (bounds.removeFromLeft (otherColWidth - colGap));
            bounds.removeFromLeft (colGap);
        }
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
        return juce::String (position.getPolar ().azimuth.get () / radians_t::RADIAN_PER_DEGREE, 1);
    case Position::Coordinate::elevation:
        return juce::String (position.getPolar ().elevation.get () / radians_t::RADIAN_PER_DEGREE, 1);
    case Position::Coordinate::radius:
        return juce::String (position.getPolar ().length, 3);
    default:
        jassertfalse;
        return juce::String ();
    };
}

void SpeakerTreeComponent::setPositionCoordinate(Position::Coordinate coordinate, float newValue)
{
    // get the current position and update the coordinate
    auto localPosition = [position{ getPosition() }, coordinate, newValue]() {
        switch (coordinate) {
        case Position::Coordinate::x:
            return position.withX(newValue);
        case Position::Coordinate::y:
            return position.withY(newValue);
        case Position::Coordinate::z:
            return position.withZ(newValue);
        case Position::Coordinate::azimuth:
            return Position{ position.getPolar().withAzimuth(radians_t(newValue * radians_t::RADIAN_PER_DEGREE)) };
        case Position::Coordinate::elevation:
            return Position{ position.getPolar().withElevation(radians_t(newValue * radians_t::RADIAN_PER_DEGREE)) };
        case Position::Coordinate::radius:
            return Position{ position.getPolar().withRadius(newValue) };
        default:
            jassertfalse;
            return position;
        }
    }();

#if ! ENABLE_GROUP_MOVEMENT_IN_DOME
    auto const spatMode{ getSpatMode().value_or(SpatMode::mbap) };
    localPosition = getLegalSpeakerPosition(localPosition, spatMode, speakerTreeVt[DIRECT_OUT_ONLY], coordinate);
    setPosition(localPosition);
#else
    // TODO: https://github.com/GRIS-UdeM/SpatGRIS/issues/486. For now group movement is just disabled in dome
    // clamp the position to a legal value and set it back
    auto const spatMode{ getSpatMode().value_or(SpatMode::mbap) };
    auto const isDirectOut{ speakerTreeVt[DIRECT_OUT_ONLY] };

    DBG(localPosition.toString());

    auto const parent{ speakerTreeVt.getParent() };
    jassert(parent.hasProperty(CARTESIAN_POSITION));
    auto const parentPosition{ juce::VariantConverter<Position>::fromVar(parent[CARTESIAN_POSITION]) };
    DBG(parentPosition.toString());

    // if this is a speaker we need to factor in the group position when clamping
    if (auto absolutePosition = SpeakerData::getAbsoluteSpeakerPosition(localPosition, parentPosition)) {
        DBG(absolutePosition->toString());

        absolutePosition = getLegalSpeakerPosition(*absolutePosition, spatMode, isDirectOut, coordinate);
        DBG(absolutePosition->toString());

        // get speaker position and offset it by the group center
        localPosition = { CartesianVector{ absolutePosition->getCartesian().x - parentPosition.getCartesian().x,
                                           absolutePosition->getCartesian().y - parentPosition.getCartesian().y,
                                           absolutePosition->getCartesian().z - parentPosition.getCartesian().z } };
        DBG(localPosition.toString());

        setPosition(localPosition);
    }
#endif
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
        if (! label.isEnabled())
            return;

        auto currentValue = label.getText().getFloatValue();
        auto const dragIncrement = (&label == &x || &label == &y || &label == &z || &label == &distance) ? 0.01f : 0.5f;
        auto newValue = currentValue - deltaY * dragIncrement;

        setPositionCoordinate(coordinate, newValue);
        updateAllPositionLabels ();
    };

    addAndMakeVisible(label);
}

void SpeakerTreeComponent::setupStringLabel (juce::Label & label, juce::StringRef text)
{
    label.setText(text, juce::dontSendNotification);
    addAndMakeVisible(label);
}

void SpeakerTreeComponent::labelTextChanged (juce::Label* label)
{
    if (label != &id)
        return;

    auto const currentId = id.getText ().getIntValue ();
    auto const clampedId { std::clamp (currentId, 1, MAX_NUM_SPEAKERS) };
    if (clampedId != currentId)
        id.setText (juce::String (clampedId), juce::dontSendNotification);

    // TODO: for now speaker ID edition isn't undoable; it's impossible to track the proper previous and next
    // output patch numbers when lining up multiple undos/redos. To do this properly, we should change
    // MainContentComponent::speakerOutputPatchChanged() and all related logic to use speaker UUIDs instead of
    // the previous ID, e.g., speakerOutputPatchChanged(speakerUuid, newOutputPatchId)
    speakerTreeVt.setProperty (NEXT_SPEAKER_PATCH_ID, clampedId, nullptr);
}

void SpeakerTreeComponent::updateAllPositionLabels ()
{
    x.setText(getPositionCoordinateTrimmedText(Position::Coordinate::x), juce::dontSendNotification);
    y.setText(getPositionCoordinateTrimmedText(Position::Coordinate::y), juce::dontSendNotification);
    z.setText(getPositionCoordinateTrimmedText(Position::Coordinate::z), juce::dontSendNotification);
    azim.setText(getPositionCoordinateTrimmedText(Position::Coordinate::azimuth), juce::dontSendNotification);
    elev.setText(getPositionCoordinateTrimmedText(Position::Coordinate::elevation), juce::dontSendNotification);
    distance.setText(getPositionCoordinateTrimmedText(Position::Coordinate::radius), juce::dontSendNotification);
}

tl::optional<SpatMode> SpeakerTreeComponent::getSpatMode() const
{
    return stringToSpatMode (speakerTreeVt.getRoot ().getProperty(SPAT_MODE).toString());
}

void SpeakerTreeComponent::valueTreePropertyChanged(juce::ValueTree & valueTree, const juce::Identifier & property)
{
    if (property == SPAT_MODE)
        updateUiBasedOnSpatMode ();

    if (valueTree != speakerTreeVt)
        return;

    if (property == SPEAKER_GROUP_NAME || property == SPEAKER_PATCH_ID) {
        auto const value = speakerTreeVt[property].toString ();
        id.setText(value, juce::dontSendNotification);
    }

    if (property == CARTESIAN_POSITION)
        updateAllPositionLabels();

    // only interested in this tree and property for now
    if (property == DIRECT_OUT_ONLY)
        updateUiBasedOnSpatMode();
}

void SpeakerTreeComponent::updateUiBasedOnSpatMode ()
{
    if (auto const spatMode{ getSpatMode() }; spatMode == SpatMode::vbap) {
        x.setEnabled(false);
        y.setEnabled(false);
        z.setEnabled(false);

        // the behaviour changes if we are a SpeakerComponent or a SpeakerGroupComponent
        setVbapSphericalCoordinateBehaviour();
    }
    else if (spatMode == SpatMode::mbap)
    {
        x.setEnabled (true);
        y.setEnabled (true);
        z.setEnabled (true);
        azim.setEnabled (false);
        elev.setEnabled (false);
        distance.setEnabled (false);
    }
    else
    {
        x.setEnabled (true);
        y.setEnabled (true);
        z.setEnabled (true);
        azim.setEnabled (true);
        elev.setEnabled (true);
        distance.setEnabled (false);
    }
}

//==============================================================================

SpeakerGroupComponent::SpeakerGroupComponent(SpeakerSetupLine* owner,
                                             const juce::ValueTree & v,
                                             juce::UndoManager & undoMan)
    : SpeakerTreeComponent(owner, v, undoMan)
{
  id.getTextValue().referTo(speakerTreeVt.getPropertyAsValue(SPEAKER_GROUP_NAME, &undoManager));
  id.setEditable (true);
  addAndMakeVisible(id);
}

// we have nothing particular to do with the labels when we are a group.
void SpeakerGroupComponent::labelTextChanged(juce::Label * labelThatHasChanged)
{
  return;
}

void SpeakerGroupComponent::deleteButtonBehaviour() {
    auto parent = speakerTreeVt.getParent();
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

void SpeakerGroupComponent::paint(juce::Graphics & g)
{
    if (speakerSetupLine->isSelected())
        g.fillAll(lnf.mHlBgcolor);
    else
        g.fillAll(lnf.mBackGroundAndFieldColour.darker(.5f));
}

void SpeakerGroupComponent::setVbapSphericalCoordinateBehaviour()
{
            // just disable all controls here
#if ENABLE_GROUP_MOVEMENT_IN_DOME
    azim.setEnabled(true);
    elev.setEnabled(true);
#else
    azim.setEnabled (false);
    elev.setEnabled (false);
#endif
    distance.setEnabled(false);

    // reset the group position and move its speakers to the dome
    setPosition(Position{ CartesianVector{ 0.f, 0.f, 0.f } });
    for (auto child : speakerTreeVt) {
        jassert(child.hasProperty(CARTESIAN_POSITION));
        auto curChildPosition = juce::VariantConverter<Position>::fromVar(child[CARTESIAN_POSITION]);
        child.setProperty(CARTESIAN_POSITION,
                          juce::VariantConverter<Position>::toVar(curChildPosition.normalized()),
                          &undoManager);
    }
}

//==============================================================================

SpeakerComponent::SpeakerComponent(SpeakerSetupLine* owner,
                                   const juce::ValueTree & v,
                                   juce::UndoManager & undoMan)
    : SpeakerTreeComponent(owner, v, undoMan)
{
    id.setEditable (true);
    id.addListener (this);
    setupStringLabel(id, speakerTreeVt[SPEAKER_PATCH_ID].toString());
    setupGain();

    setupHighPass ();

    direct.getToggleStateValue ().referTo (speakerTreeVt.getPropertyAsValue (DIRECT_OUT_ONLY, &undoManager));
    addAndMakeVisible (direct);
}

void SpeakerComponent::paint(juce::Graphics & g)
{
    if (speakerSetupLine->isSelected())
        g.fillAll(lnf.mHlBgcolor);
    else if(speakerSetupLine->getIndexInParent() % 2 == 0)
        g.fillAll(lnf.mGreyColour);
}

void SpeakerComponent::editorShown (juce::Label* label, juce::TextEditor& editor)
{
    if (label != &id)
        return;

    editor.setInputRestrictions (3, "0123456789");
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
        if (! gain.isEnabled ())
            return;

        auto const draggedValue { gain.getText ().getFloatValue () - deltaY * 0.05f };
        auto const clampedValue { getClampedGain (draggedValue).get () };

        gain.setText (juce::String (clampedValue, 1), juce::dontSendNotification);
        speakerTreeVt.setProperty (GAIN, clampedValue, &undoManager);
        };

    addAndMakeVisible (gain);
}

void SpeakerComponent::setVbapSphericalCoordinateBehaviour()
{
    azim.setEnabled (true);
    elev.setEnabled (true);
    distance.setEnabled (speakerTreeVt[DIRECT_OUT_ONLY]);
}

void SpeakerComponent::deleteButtonBehaviour()
{
    auto parent = speakerTreeVt.getParent();
    parent.removeChild(speakerTreeVt, &undoManager);
}

void SpeakerComponent::setupHighPass()
{
    highpass.setEditable(true);
    juce::String const highPass = speakerTreeVt.hasProperty(HIGHPASS_FREQ) ? speakerTreeVt[HIGHPASS_FREQ] : "0";
    highpass.setText(highPass, juce::dontSendNotification);

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
        speakerTreeVt.setProperty(HIGHPASS_FREQ, clampedValue, &undoManager);
    };

    highpass.onMouseDragCallback = [this, getClampedFrequency](int deltaY) {
        if (! highpass.isEnabled ())
            return;

        auto const draggedValue{ highpass.getText().getFloatValue() - deltaY * 0.5f };
        auto const clampedValue{ getClampedFrequency(draggedValue, deltaY < 0).get() };

        highpass.setText(juce::String(clampedValue, 1), juce::dontSendNotification);
        speakerTreeVt.setProperty(HIGHPASS_FREQ, clampedValue, &undoManager);
    };

    addAndMakeVisible(highpass);
}

} // namespace gris
