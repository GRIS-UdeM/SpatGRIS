/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_SmallToggleButton.hpp"
#include "sg_SourceIndex.hpp"
#include "sg_constants.hpp"

namespace gris
{
class SmallGrisLookAndFeel;

//==============================================================================
class SourceIdButton final
    : public MinSizedComponent
    , private SmallToggleButton::Listener
    , private juce::ChangeListener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        SG_DEFAULT_COPY_AND_MOVE(Listener)
        //==============================================================================
        virtual void sourceIdButtonColorChanged(SourceIdButton * button, juce::Colour color) = 0;
        virtual void sourceIdButtonCopyColorToNextSource(SourceIdButton * button, juce::Colour color) = 0;
    };

private:
    //==============================================================================
    Listener & mListener;

    SmallToggleButton mButton;

public:
    //==============================================================================
    SourceIdButton(source_index_t sourceIndex,
                   juce::Colour color,
                   Listener & listener,
                   SmallGrisLookAndFeel & lookAndFeel);
    ~SourceIdButton() override = default;
    SG_DELETE_COPY_AND_MOVE(SourceIdButton)
    //==============================================================================
    void setColor(juce::Colour const & color);
    //==============================================================================
    [[nodiscard]] int getMinWidth() const noexcept override { return SLICES_WIDTH; }
    [[nodiscard]] int getMinHeight() const noexcept override { return SLICES_ID_BUTTON_HEIGHT; }
    void resized() override;

private:
    //==============================================================================
    void changeListenerCallback(juce::ChangeBroadcaster * source) override;
    void smallButtonClicked(SmallToggleButton * button, bool state, bool isLeftMouseButton) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(SourceIdButton)
};

} // namespace gris