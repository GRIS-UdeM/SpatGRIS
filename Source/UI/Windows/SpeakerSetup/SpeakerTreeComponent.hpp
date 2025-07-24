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
#include <memory>
#include "../../../sg_GrisLookAndFeel.hpp"
#include <Data/sg_Position.hpp>
#include <Data/sg_SpatMode.hpp>
#include <StructGRIS/ValueTreeUtilities.hpp>
#include "SpeakerGroupSettingsWindow.hpp"

#define ENABLE_GROUP_MOVEMENT_IN_DOME 0

namespace gris
{
class SpeakerSetupLine;

/**
 * @class DraggableLabel
 * @brief A juce::Label subclass that supports vertical mouse drag events.
 *
 * DraggableLabel allows the user to drag the label vertically, triggering the public onMouseDragCallback member
 * with the delta Y movement. This is useful for adjusting values interactively in the UI.
 */
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

/**
 * @class SpeakerTreeComponent
 * @brief UI component for displaying and editing speaker or speaker group properties in a tree structure.
 *
 * SpeakerTreeComponent provides a graphical interface for manipulating speaker positions and properties
 * within a speaker setup. It supports editing coordinates, gain, highpass, and other parameters via
 * draggable and editable labels. The component listens to changes in the underlying ValueTree and
 * updates the UI accordingly. It also provides support for undo/redo operations and integrates with
 * the application's look and feel.
 *
 * @see SpeakerGroupComponent, SpeakerComponent, SpeakerSetupLine
 */
class SpeakerTreeComponent
    : public juce::Component
    , public juce::ValueTree::Listener
{
public:
    static constexpr auto fixedLeftColWidth { 200 };
    static constexpr auto otherColWidth { 60 };
    static constexpr auto colGap { 15 };

    juce::ValueTree speakerTreeVt;

    SpeakerTreeComponent(SpeakerSetupLine * owner, const juce::ValueTree & v, juce::UndoManager & undoManager);

    ~SpeakerTreeComponent() { setLookAndFeel(nullptr); }

    void resized() override;

    void setPosition(Position position)
    {
        speakerTreeVt.setProperty(CARTESIAN_POSITION, juce::VariantConverter<Position>::toVar(position), &undoManager);
    }

    Position getPosition() { return juce::VariantConverter<Position>::fromVar(speakerTreeVt[CARTESIAN_POSITION]); }

    float getPositionCoordinate (Position::Coordinate coordinate);

    juce::String getPositionCoordinateTrimmedText(Position::Coordinate coordinate);
    juce::UndoManager& undoManager;
    void setPositionCoordinate (Position::Coordinate coordinate, float newValue);

protected:
    void setupCoordinateLabel(DraggableLabel & label, Position::Coordinate coordinate);
    void setupStringLabel(juce::Label & label, juce::StringRef text);
    void setupDeleteButton ();

    void updateAllPositionLabels();

    juce::Label id;

    DraggableLabel x, y, z, azim, elev, distance, gain, highpass, drag;

    juce::DrawableButton deleteButton { "TrashButton", juce::DrawableButton::ImageOnButtonBackground };

    juce::ToggleButton direct;
    juce::ValueTree speakerSetupVt;

    GrisLookAndFeel lnf;
    SpeakerSetupLine* speakerSetupLine;

    tl::optional<SpatMode> getSpatMode () const;
    /**
     * The delete button behaviour depends on wether or not we are a speaker group
     */
    virtual void deleteButtonBehaviour() = 0;
    /**
     * This is called in "updateUiBasedOnSpatMode". Speakers and groups of speakers
     * behave differently when the mode is vbap.
     */
    virtual void setVbapSphericalCoordinateBehaviour() = 0;

protected:
    void updateUiBasedOnSpatMode();

private:
    void valueTreePropertyChanged(juce::ValueTree & speakerTreeVt, const juce::Identifier & property) override;
};

class SpeakerGroupSettingsWindow;
//==============================================================================
/**
 * @class SpeakerGroupComponent
 * @brief Specialized SpeakerTreeComponent that represents a group of speakers.
 *
 * @see SpeakerTreeComponent
 */
class SpeakerGroupComponent : public SpeakerTreeComponent,
                              public juce::Button::Listener
{
public:
    SpeakerGroupComponent(SpeakerSetupLine* owner, const juce::ValueTree & v, juce::UndoManager & undoManager);
    void paint(juce::Graphics & g) override;
    void resized() override;
    void closeSettingsWindow();
protected:
  void deleteButtonBehaviour() override;
  void setVbapSphericalCoordinateBehaviour() override;
private:
    static constexpr uint32_t groupSettingsButtonWidth = 20;
    juce::ImageButton groupSettingsButton;
    std::unique_ptr<SpeakerGroupSettingsWindow> settingsWindow;
    void buttonClicked (juce::Button* button) override;

};
//==============================================================================

/**
 * @class SpeakerComponent
 * @brief Specialized SpeakerTreeComponent that represents an individual speaker.
 *
 * @see SpeakerTreeComponent
 */
class SpeakerComponent : public SpeakerTreeComponent,
                         public juce::Label::Listener
{
public:
    SpeakerComponent(SpeakerSetupLine* owner, const juce::ValueTree & v, juce::UndoManager & undoManager);
    void paint(juce::Graphics & g) override;
protected:
  void deleteButtonBehaviour() override;
  void setVbapSphericalCoordinateBehaviour() override;
private:
    void setupGain ();
    void setupHighPass ();
    void editorShown(juce::Label *, juce::TextEditor &) override;
    void labelTextChanged(juce::Label * labelThatHasChanged) override;
};
} // namespace gris
