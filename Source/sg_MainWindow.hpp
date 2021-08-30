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

#include "sg_MainComponent.hpp"

//==============================================================================
/* This class implements the desktop window that contains an instance of
   our MainContentComponent class.
*/
class MainWindow final
    : public juce::DocumentWindow
    , public juce::TooltipWindow
{
public:
    enum CommandIds {
        // File menu
        newProjectId = 1000,
        openProjectId,
        saveProjectId,
        saveProjectAsId,

        openSpeakerSetupId,
        saveSpeakerSetupId,
        saveSpeakerSetupAsId,

        openSettingsWindowId,

        quitId,

        // View menu
        show2DViewId,
        showSpeakerEditId,
        showOscMonitorId,

        showNumbersId,
        showSpeakersId,
        showTripletsId,
        showSourceActivityId,
        showSpeakerActivityId,
        showSphereId,

        colorizeInputsId,
        resetInputPosId,
        resetMeterClipping,

        // Help menu
        aboutId,
        openManualId
    };

private:
    //==============================================================================
    juce::ApplicationCommandManager mApplicationCommandManager{};
    std::unique_ptr<MainContentComponent> mMainContentComponent{};

public:
    //==============================================================================
    MainWindow(juce::String const & name,
               GrisLookAndFeel & newLookAndFeel,
               SmallGrisLookAndFeel & smallGrisLookAndFeel);
    //==============================================================================
    MainWindow() = delete;
    ~MainWindow() override = default;
    //==============================================================================
    MainWindow(MainWindow const &) = delete;
    MainWindow(MainWindow &&) = delete;
    MainWindow & operator=(MainWindow const &) = delete;
    MainWindow & operator=(MainWindow &&) = delete;
    //==============================================================================
    [[nodiscard]] bool exitWinApp() const;
    [[nodiscard]] juce::ApplicationCommandManager & getApplicationCommandManager();
    //==============================================================================
    void closeButtonPressed() override;
    //==============================================================================
    [[nodiscard]] static MainWindow * getMainAppWindow();

private:
    //=============================================================
    JUCE_LEAK_DETECTOR(MainWindow)
};
