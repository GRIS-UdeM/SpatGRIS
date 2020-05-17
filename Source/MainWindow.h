/*
 This file is part of SpatGRIS2.

 Developers: Olivier Belanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../JuceLibraryCode/JuceHeader.h"

#include "GrisLookAndFeel.h"
#include "MainComponent.h"

//==============================================================================
/* This class implements the desktop window that contains an instance of
   our MainContentComponent class.
*/
class MainWindow final : public DocumentWindow
{
public:
    enum CommandIDs
    {
        // File menu
        NewPresetID = 1000,
        OpenPresetID = 1001,
        SavePresetID = 1002,
        SaveAsPresetID = 1003,

        OpenSpeakerSetupID = 2000,
        ShowSpeakerEditID = 2003,

        PrefsID = 9998,
        QuitID = 9999,

        // View menu
        Show2DViewID = 3000,
        ShowNumbersID = 3001,
        ShowSpeakersID = 3002,
        ShowTripletsID = 3003,
        ShowSourceLevelID = 3004,
        ShowSpeakerLevelID = 3005,
        ShowSphereID = 3006,
        ColorizeInputsID = 3010,
        ResetInputPosID = 3011,
        ResetMeterClipping = 3012,
        ShowOscLogView = 3100,

        // Help menu
        AboutID = 4000,
        OpenManualID = 4001,
    };
    //==============================================================================
    MainWindow(String name);
    ~MainWindow() = default;
    //==============================================================================
    bool exitWinApp();

    // This is called when the user tries to close this window. Here, we'll just ask the
    // app to quit when this happens, but you can change this to do whatever you need.
    void closeButtonPressed() final { JUCEApplication::getInstance()->systemRequestedQuit(); }

    // returns the MainWindow if it exists.
    static MainWindow * getMainAppWindow()
    {
        for (int i = TopLevelWindow::getNumTopLevelWindows(); --i >= 0;) {
            if (auto * maw = dynamic_cast<MainWindow *>(TopLevelWindow::getTopLevelWindow(i)))
                return maw;
        }
        return nullptr;
    }

    // returns the command manager object used to dispatch command events.
    ApplicationCommandManager & getApplicationCommandManager();

private:
    //==============================================================================
    std::unique_ptr<MainContentComponent> mcc{};
    juce::ApplicationCommandManager       applicationCommandManager{};
    //=============================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

#endif // MAINWINDOW_H
