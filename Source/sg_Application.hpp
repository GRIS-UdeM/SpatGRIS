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

#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainWindow.hpp"

namespace gris
{
//==============================================================================
class SpatGrisApplication final : public juce::JUCEApplication
{
    std::unique_ptr<MainWindow> mMainWindow{};
    GrisLookAndFeel mGrisFeel{};
    SmallGrisLookAndFeel mSmallLookAndFeel{};

public:
    //==============================================================================
    SpatGrisApplication() = default;
    ~SpatGrisApplication() override = default;
    SG_DELETE_COPY_AND_MOVE(SpatGrisApplication)
    //==============================================================================
    [[nodiscard]] const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    [[nodiscard]] const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    [[nodiscard]] bool moreThanOneInstanceAllowed() override { return true; }
    void initialise(const juce::String & /*commandLine*/) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String & /*commandLine*/) override {}

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatGrisApplication)
};

} // namespace gris