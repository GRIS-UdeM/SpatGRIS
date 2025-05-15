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
class SpeakerSetupLine;

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

class SpeakerTreeComponent : public juce::Component, public juce::ValueTree::Listener
{
public:
    SpeakerTreeComponent(SpeakerSetupLine* owner, const juce::ValueTree & v, juce::UndoManager & undoManager);
    ~SpeakerTreeComponent() { setLookAndFeel(nullptr); }

    void paint(juce::Graphics & g) override;

    void resized() override;

    void setPosition(Position position)
    {
        speakerTreeVt.setProperty(CARTESIAN_POSITION, juce::VariantConverter<Position>::toVar(position), &undoManager);
    }

    Position getPosition() { return juce::VariantConverter<Position>::fromVar(speakerTreeVt[CARTESIAN_POSITION]); }

    float getPositionCoordinate (Position::Coordinate coordinate);

    juce::String getPositionCoordinateTrimmedText(Position::Coordinate coordinate);

    void setPositionCoordinate (Position::Coordinate coordinate, float newValue);

protected:
    void setupCoordinateLabel(DraggableLabel & label, Position::Coordinate coordinate);
    void setupStringLabel(juce::Label & label, juce::StringRef text);
    void setupEditorLabel (juce::Label& label, juce::Identifier property);

    void updateAllPositionLabels();

    DraggableLabel id, x, y, z, azim, elev, radius, gain, highpass, drag;

    juce::DrawableButton deleteButton { "TrashButton", juce::DrawableButton::ImageFitted };

    juce::ToggleButton direct;

    juce::ValueTree speakerTreeVt;
    juce::ValueTree speakerSetupVt;

    GrisLookAndFeel lnf;
    SpeakerSetupLine* treeViewItem;

    tl::optional<SpatMode> getSpatMode () const;

    juce::UndoManager& undoManager;

private:
    void valueTreePropertyChanged(juce::ValueTree & speakerTreeVt, const juce::Identifier & property) override;
    void updateEnabledLabels();
};

//==============================================================================

class SpeakerGroupComponent : public SpeakerTreeComponent
{
public:
    SpeakerGroupComponent(SpeakerSetupLine* owner, const juce::ValueTree & v, juce::UndoManager & undoManager);
};

//==============================================================================

class SpeakerComponent : public SpeakerTreeComponent
{
public:
    SpeakerComponent(SpeakerSetupLine* owner, const juce::ValueTree & v, juce::UndoManager & undoManager);
private:
    void setupGain ();
    void setupHighPass ();
};
} // namespace gris
