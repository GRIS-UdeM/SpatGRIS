#pragma once

#include "../../../sg_GrisLookAndFeel.hpp"
#include <JuceHeader.h>

namespace gris
{

class SpeakerColumnHeader final : public juce::Label
{
public:
    /**
     * paints little sort arrows in the right places.
     */
    void paint(juce::Graphics & g) override;

    int width;

    void mouseUp(const juce::MouseEvent & e) override;
    juce::Colour arrowColor;
    /**
     * none sort state means no arrows are painted.
     *
     * the state transitions are
     *
     * none -> ascending <-> descending.
     *
     * none can only be reached at initialization and by a call to setState.
     */
    enum class SortState { none, ascending, descending }; // enum class

    enum class ColumnID { ID, X, Y, Z, Azimuth, Elevation, Distance, Gain, Highpass };

    void setState(SortState newState);

    void setSortCallback(std::function<void(const SpeakerColumnHeader *, const SortState)>);

private:
    SortState sortState{ SortState::none };
    /**
     * Sorting callback that is called with the column's state when it is clicked.
     */
    std::function<void(const SpeakerColumnHeader *, const SortState)> sortCallback;
};

class SpeakerSetupContainerHeader final : public juce::Component
{
public:
    SpeakerSetupContainerHeader(GrisLookAndFeel & grisLookAndFeel);

    void resized() override;
    void setSortFunc(std::function<void(SpeakerColumnHeader::ColumnID, int)>);

private:
    SpeakerColumnHeader id, x, y, z, azim, elev, distance, gain, highpass;
    juce::Label direct, del, drag;
    GrisLookAndFeel & grisLookAndFeel;
    std::function<void(SpeakerColumnHeader::ColumnID, int)> sortFunc;
};
} // namespace gris
