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

#include "sg_MinSizedComponent.hpp"

namespace gris
{
class GrisLookAndFeel;

//==============================================================================
class SpatTextEditor final
    : public MinSizedComponent
    , public juce::TextEditor::Listener
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
        virtual void textEditorChanged(juce::String const & value, SpatTextEditor * editor) = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    Listener & mListener;
    juce::Label mLabel{};
    juce::TextEditor mEditor{};

public:
    //==============================================================================
    SpatTextEditor(juce::String const & label,
                   juce::String const & tooltip,
                   Listener & listener,
                   GrisLookAndFeel & lookAndFeel);
    ~SpatTextEditor() override = default;
    SG_DELETE_COPY_AND_MOVE(SpatTextEditor)
    //==============================================================================
    void setText(juce::String const & text);
    //==============================================================================
    void resized() override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    void textEditorFocusLost(juce::TextEditor & editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor & /* editor*/) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatTextEditor)
};

} // namespace gris