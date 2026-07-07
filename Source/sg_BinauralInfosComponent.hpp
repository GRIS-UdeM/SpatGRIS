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

#pragma onec

#include "Data/sg_Macros.hpp"
#include <JuceHeader.h>

namespace gris
{
class GrisLookAndFeel;

//============================================================================
class BinauralInfosComponent final : public juce::Component, public juce::Timer
{
    static constexpr auto TITLE_HEIGHT = 18;

    GrisLookAndFeel & mLookAndFeel;
    juce::String mTitle{ "" };
    //int mSecondTitleXOffset{};
    double progressVal{ -1.0 };
    juce::ProgressBar mSpinningWheel{ progressVal };
    bool mShowSucceedCheck{ false };

    public:
    //==============================================================================
    BinauralInfosComponent(GrisLookAndFeel & lookAndFeel);
    ~BinauralInfosComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(BinauralInfosComponent)
    //==============================================================================
    void resized() override;
    void paint(juce::Graphics & g) override;
    void timerCallback() override;

    //void setBinauralTitle(juce::String title);
    void setBinauralFileName(juce::String fileName);
    void showSpinningWheel(bool showSpinningWheel);
    void showCheckSign();

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(BinauralInfosComponent)
};

} // namespace gris