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
    SpeakerTreeComponent(juce::TreeViewItem * owner, const juce::ValueTree & v);
    ~SpeakerTreeComponent() { setLookAndFeel(nullptr); }

    void paint(juce::Graphics & g) override;

    void resized() override;

protected:
    void setupEditor(DraggableLabel & editor, juce::Value value);
    void setupEditor(juce::Label & editor, juce::StringRef text);

    DraggableLabel id, x, y, z, azim, elev, distance, gain, highpass, direct, del, drag;

    juce::ValueTree vt;

    GrisLookAndFeel lnf;
    juce::TreeViewItem * treeViewItem;

    class ValueToLabelListener : public juce::Value::Listener
    {
    public:
        ValueToLabelListener(juce::Value valueToWatch, juce::Label & targetLabel)
            : value(valueToWatch)
            , weakLabel(&targetLabel)
        {
            value.addListener(this);
        }

        ~ValueToLabelListener() override { value.removeListener(this); }

        void valueChanged(juce::Value & v) override
        {
            if (auto * l = weakLabel.getComponent())
                l->setText(juce::String((float)v.getValue(), 3), juce::dontSendNotification);
        }

    private:
        juce::Value value;
        juce::Component::SafePointer<juce::Label> weakLabel;
    };

    juce::OwnedArray<ValueToLabelListener> valueListeners;
};

//==============================================================================

class SpeakerGroupComponent : public SpeakerTreeComponent
{
public:
    SpeakerGroupComponent(juce::TreeViewItem * owner, const juce::ValueTree & v);
};

//==============================================================================

class SpeakerComponent : public SpeakerTreeComponent
{
public:
    SpeakerComponent(juce::TreeViewItem * owner, const juce::ValueTree & v);
};
} // namespace gris
