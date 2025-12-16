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

#include "sg_NumSlider.hpp"
#include "sg_GrisLookAndFeel.hpp"

namespace gris
{

//==============================================================================
NumSlider::NumSlider(GrisLookAndFeel & grisLookAndFeel,
                     float minVal,
                     float maxVal,
                     float inc,
                     int numDec,
                     float defaultReturnValue,
                     juce::String const & suffix,
                     juce::String const & tooltip)
    : mGrisLookAndFeel(grisLookAndFeel)
{
    setLookAndFeel(&grisLookAndFeel);
    setTitle("NumSlider");
    setRange(minVal, maxVal, inc);
    setTooltip(tooltip);
    setSliderStyle(juce::Slider::LinearBar);
    setSliderSnapsToMousePosition(false);
    mDefaultNumDecimalToDisplay = numDec;
    setNumDecimalPlacesToDisplay(numDec);
    mDefaultReturnValue = defaultReturnValue;
    setScrollWheelEnabled(true);
    setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, getWidth(), getHeight());
    setTextBoxIsEditable(false);
    mSuffix = suffix == "" ? suffix : juce::String(" " + suffix);
}

//==============================================================================
void NumSlider::mouseWheelMove(const juce::MouseEvent & /*event*/, const juce::MouseWheelDetails & wheel)
{
    mLastValue = getValue();
    juce::Time currentTime = juce::Time::getCurrentTime();
    auto timeDiff = static_cast<double>((currentTime - mLastTime).inMilliseconds());
    if (timeDiff <= 0.0001)
        timeDiff = 1.0;
    double valueDiff = wheel.deltaY * getInterval();
    double velocity = valueDiff / timeDiff * 1000;
    double newValue = mLastValue - velocity;
    newValue = std::clamp(newValue, getRange().getStart(), getRange().getEnd());

    setValue(newValue);

    mLastTime = currentTime;
    mLastValue = newValue;
}

//==============================================================================
void NumSlider::paint(juce::Graphics & g)
{
    auto rangeVals{ getRange() };
    const auto rangeStart{ rangeVals.getStart() };
    auto val{ getValue() };
    auto bounds{ getLocalBounds() };

    auto onColor{ mGrisLookAndFeel.getOnColour() };
    auto lightColor{ mGrisLookAndFeel.getLightColour() };

    if (!isEnabled()) {
        onColor = onColor.withBrightness(0.8f);
        lightColor = lightColor.withBrightness(0.8f);
    }

    if (val > rangeStart) {
        juce::Rectangle<float> drawRec;
        g.setColour(onColor);
        double xLimitProportion{};
        const auto rangeLength{ rangeVals.getLength() };
        const auto rangeEnd{ rangeVals.getEnd() };

        if (rangeStart < 0 && rangeEnd > 0) {
            xLimitProportion = (val + rangeLength / 2) / rangeLength;
        } else if (rangeStart > 0 || (rangeStart < 0 && rangeEnd <= 0)) {
            xLimitProportion = (val - rangeStart) / rangeLength;
        } else {
            xLimitProportion = val / rangeLength;
        }

        drawRec = juce::Rectangle<float>{ 0,
                                          0,
                                          static_cast<float>(bounds.getWidth() * xLimitProportion),
                                          static_cast<float>(bounds.getHeight()) };
        g.fillRect(drawRec);

        g.setColour(lightColor);
        drawRec = juce::Rectangle<float>{ static_cast<float>(bounds.getWidth() * xLimitProportion),
                                          0,
                                          static_cast<float>(bounds.getWidth()),
                                          static_cast<float>(bounds.getHeight()) };
        g.fillRect(drawRec);
    } else {
        g.setColour(lightColor);
        g.fillRect(bounds);
    }

    g.setColour(mGrisLookAndFeel.getDarkColour());
    g.setFont(mGrisLookAndFeel.getFont());
    g.drawText(juce::String(getTextFromValue(getValue()) + mSuffix),
               bounds.translated(0, 1),
               juce::Justification::centred);
}

//==============================================================================
void NumSlider::valueChanged()
{
    mLastValue = getValue();

    if (mLastValue >= 10000 && getNumDecimalPlacesToDisplay() > 0) {
        setNumDecimalPlacesToDisplay(0);
    } else if (mLastValue < 10000 && getNumDecimalPlacesToDisplay() == 0) {
        setNumDecimalPlacesToDisplay(mDefaultNumDecimalToDisplay);
    }
}

//==============================================================================
void NumSlider::mouseDown(const juce::MouseEvent & event)
{
    if (isEnabled()) {
        mMouseDragStartPos = event.getMouseDownPosition();
        mMouseDiffFromStartY = 0;
    }
}

//==============================================================================
void NumSlider::mouseDrag(const juce::MouseEvent & event)
{
    if (isEnabled()) {
        const auto isShiftDown{ event.mods.isShiftDown() };
        const auto mouseDragCurrentPos{ event.getPosition() };
        const auto mouseDiffFromStartY{ mMouseDragStartPos.getY() - mouseDragCurrentPos.getY() };
        const auto mouseDiffDragY{ mouseDiffFromStartY - mMouseDiffFromStartY };
        const auto range{ getRange() };
        const auto smallestSliderVal{ std::pow(10, -1 * (getNumDecimalPlacesToDisplay())) };
        const auto increment{ isShiftDown ? smallestSliderVal * 0.1f * std::abs(mouseDiffDragY)
                                          : range.getLength() / 100 * std::abs(mouseDiffDragY) };

        if (!isShiftDown && mIsFineDragging) {
            stopFineClickDragging(event);
        } else if (isShiftDown && !mIsFineDragging) {
            startFineClickDragging(event);
        }

        if (mouseDiffDragY > 0) {
            if (increment < smallestSliderVal && mIncrementBuffer < smallestSliderVal) {
                mIncrementBuffer += increment;
                if (mIncrementBuffer >= smallestSliderVal) {
                    double newValue = getValue() + mIncrementBuffer;
                    setValue(juce::jlimit(getMinimum(), getMaximum(), newValue), juce::sendNotificationSync);
                    mIncrementBuffer = 0.0f;
                }
            } else {
                double newValue = getValue() + increment;
                setValue(juce::jlimit(getMinimum(), getMaximum(), newValue), juce::sendNotificationSync);
            }
            mMouseDiffFromStartY = mouseDiffFromStartY;
        } else if (mouseDiffDragY < 0) {
            if (increment < smallestSliderVal && mIncrementBuffer < smallestSliderVal) {
                mIncrementBuffer += increment;
                if (mIncrementBuffer >= smallestSliderVal) {
                    double newValue = getValue() - mIncrementBuffer;
                    setValue(juce::jlimit(getMinimum(), getMaximum(), newValue), juce::sendNotificationSync);
                    mIncrementBuffer = 0.0f;
                }
            } else {
                double newValue = getValue() - increment;
                setValue(juce::jlimit(getMinimum(), getMaximum(), newValue), juce::sendNotificationSync);
            }
            mMouseDiffFromStartY = mouseDiffFromStartY;
        }
    }
}

//==============================================================================
void NumSlider::mouseUp(const juce::MouseEvent & event)
{
    if (isEnabled()) {
        if (event.mods.isAltDown() && event.mods.isLeftButtonDown()) {
            setValue(mDefaultReturnValue);
        }

        if (event.mods.isShiftDown() && mIsFineDragging) {
            stopFineClickDragging(event);
        }
    }
}

//==============================================================================
void NumSlider::mouseDoubleClick(const juce::MouseEvent & /*event*/)
{
    if (isEnabled()) {
        auto sliderEditor{ std::make_unique<juce::TextEditor>("SliderEditor") };
        sliderEditor->setLookAndFeel(&mGrisLookAndFeel);
        sliderEditor->setJustification(juce::Justification::centred);
        sliderEditor->addListener(this);
        sliderEditor->setMultiLine(false);
        sliderEditor->setSize(60, 20);
        if (getRange().getStart() < 0) {
            sliderEditor->setInputRestrictions(5, "0123456789,.-");
        } else {
            sliderEditor->setInputRestrictions(5, "0123456789,.");
        }
        sliderEditor->setText(juce::String(getValue()), false);
        sliderEditor->selectAll();

        auto & box = juce::CallOutBox::launchAsynchronously(std::move(sliderEditor), getScreenBounds(), nullptr);
        box.setLookAndFeel(&mGrisLookAndFeel);
    }
}

//==============================================================================
void NumSlider::setDefaultNumDecimalPlacesToDisplay(int numDec)
{
    mDefaultNumDecimalToDisplay = numDec;
    setNumDecimalPlacesToDisplay(mDefaultNumDecimalToDisplay);
}

void NumSlider::setDefaultReturnValue(double value)
{
    mDefaultReturnValue = value;
}

//==============================================================================
void NumSlider::textEditorReturnKeyPressed(juce::TextEditor & ed)
{
    if (!ed.getText().isEmpty()) {
        auto val = ed.getText().replace(",", ".").getDoubleValue();
        mLastValue = val;
        setValue(val);
    }

    auto callOutBox = dynamic_cast<juce::CallOutBox *>(ed.getParentComponent());

    if (callOutBox != nullptr) {
        callOutBox->dismiss();
    }
}

//==============================================================================
void NumSlider::textEditorEscapeKeyPressed(juce::TextEditor & ed)
{
    auto callOutBox = dynamic_cast<juce::CallOutBox *>(ed.getParentComponent());

    if (callOutBox != nullptr) {
        callOutBox->dismiss();
    }
}

//==============================================================================
void NumSlider::startFineClickDragging(const juce::MouseEvent & event)
{
    setMouseCursor(juce::MouseCursor::NoCursor);
    event.source.enableUnboundedMouseMovement(true);
    mMouseScreenPos = event.source.getScreenPosition();
    mIsFineDragging = true;
}

//==============================================================================
void NumSlider::stopFineClickDragging(const juce::MouseEvent & event)
{
    juce::MouseInputSource inputSource = juce::MouseInputSource(event.source);
    inputSource.enableUnboundedMouseMovement(false);
    inputSource.setScreenPosition(mMouseScreenPos);
    setMouseCursor(juce::MouseCursor::NormalCursor);
    mIsFineDragging = false;
}

} // namespace gris
