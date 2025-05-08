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

#pragma once
#include "../../../sg_GrisLookAndFeel.hpp"
#include "../../ValueTreeUtilities.hpp"
#include <Data/sg_Position.hpp>
#include <Data/sg_SpatMode.hpp>

namespace gris
{

class DraggableLabel : public juce::Label
{
public:
    std::function<void(int)> onMouseDragCallback;

private:
    int lastMouseY = 0;

    void mouseDown(const juce::MouseEvent & event) override { lastMouseY = event.getPosition().getY(); }

    void mouseDrag(const juce::MouseEvent & event) override
    {
        auto const currentMouseY = event.getPosition().getY();
        auto const deltaY = currentMouseY - lastMouseY;
        lastMouseY = currentMouseY;

        if (onMouseDragCallback)
            onMouseDragCallback(deltaY);
    }
};

//==============================================================================

class SpeakerTreeComponent : public juce::Component
{
public:
    SpeakerTreeComponent(juce::TreeViewItem * owner, const juce::ValueTree & v, juce::UndoManager & undoManager);
    ~SpeakerTreeComponent() { setLookAndFeel(nullptr); }

    void paint(juce::Graphics & g) override;

    void resized() override;

    void setPosition(Position position)
    {
        vt.setProperty(CARTESIAN_POSITION, juce::VariantConverter<Position>::toVar(position), &undoManager);
    }

    Position getPosition() { return juce::VariantConverter<Position>::fromVar(vt[CARTESIAN_POSITION]); }

    float getPositionCoordinate (Position::Coordinate coordinate);

    juce::String getPositionCoordinateTrimmedText(Position::Coordinate coordinate);

    void setPositionCoordinate (Position::Coordinate coordinate, float newValue);

protected:
    void setupDraggableEditor(DraggableLabel & editor, Position::Coordinate coordinate);
    void setupEditor(juce::Label & editor, juce::StringRef text);

    DraggableLabel id, x, y, z, azim, elev, distance, gain, highpass, direct, del, drag;

    juce::ValueTree vt;

    GrisLookAndFeel lnf;
    juce::TreeViewItem * treeViewItem;

    SpatMode getSpatMode () const;

    ///** This is used to edit the label values back if they were clamped by getLegalSpeakerPosition() */
    //class ValueToLabelListener : public juce::Value::Listener
    //{
    //public:
    //    ValueToLabelListener(juce::Value valueToWatch, juce::Label & targetLabel)
    //        : value(valueToWatch)
    //        , weakLabel(&targetLabel)
    //    {
    //        value.addListener(this);
    //    }

    //    ~ValueToLabelListener() override { value.removeListener(this); }

    //    void valueChanged(juce::Value & v) override
    //    {
    //        if (auto * l = weakLabel.getComponent())
    //            l->setText(juce::String((float)v.getValue(), 3), juce::dontSendNotification);
    //    }

    //private:
    //    juce::Value value;
    //    juce::Component::SafePointer<juce::Label> weakLabel;
    //};
    //juce::OwnedArray<ValueToLabelListener> valueListeners;

    juce::UndoManager& undoManager;
};

//==============================================================================

class SpeakerGroupComponent : public SpeakerTreeComponent
{
public:
    SpeakerGroupComponent(juce::TreeViewItem * owner, const juce::ValueTree & v, juce::UndoManager & undoManager);
};

//==============================================================================

class SpeakerComponent : public SpeakerTreeComponent
{
public:
    SpeakerComponent(juce::TreeViewItem * owner, const juce::ValueTree & v, juce::UndoManager & undoManager);
};
} // namespace gris
