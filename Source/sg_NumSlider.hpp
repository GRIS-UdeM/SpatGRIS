/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine

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

#include <JuceHeader.h>

namespace gris
{
class GrisLookAndFeel;

//==============================================================================
class NumSlider
    : public juce::Slider
    , private juce::TextEditor::Listener
{
public:
    //==============================================================================
    NumSlider(GrisLookAndFeel & grisLookAndFeel,
              float minVal = 0.0f,
              float maxVal = 1.0f,
              float inc = 0.001f,
              int numDec = 3,
              float defaultReturnValue = 0.0f,
              juce::String const & suffix = "",
              juce::String const & tooltip = "");

    void mouseWheelMove(const juce::MouseEvent & event, const juce::MouseWheelDetails & wheel) override;
    void paint(juce::Graphics & g) override;
    void valueChanged() override;
    void mouseDown(const juce::MouseEvent & event) override;
    void mouseDrag(const juce::MouseEvent & event) override;
    void mouseUp(const juce::MouseEvent & event) override;
    void mouseDoubleClick(const juce::MouseEvent & event) override;

    void setDefaultNumDecimalPlacesToDisplay(int numDec);
    void setDefaultReturnValue(double value);

private:
    //==============================================================================
    void textEditorReturnKeyPressed(juce::TextEditor & ed) override;
    void textEditorEscapeKeyPressed(juce::TextEditor & ed) override;

    void startFineClickDragging(const juce::MouseEvent & event);
    void stopFineClickDragging(const juce::MouseEvent & event);

    GrisLookAndFeel & mGrisLookAndFeel;

    //==============================================================================
    juce::Time mLastTime{ 0 };
    double mLastValue{ 0 };
    int mDefaultNumDecimalToDisplay{ 1 };
    double mDefaultReturnValue{};
    juce::Point<int> mMouseDragStartPos;
    juce::Point<float> mMouseScreenPos;
    bool mIsFineDragging{};
    int mMouseDiffFromStartY{};
    double mIncrementBuffer{};
    juce::String mSuffix;

    //==============================================================================
    JUCE_LEAK_DETECTOR(NumSlider)
};
} // namespace gris
