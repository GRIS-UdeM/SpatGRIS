/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

namespace gris
{
enum CommandId {
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
    showPlayerWindowId,
    showSpeakerEditId,
    showOscMonitorId,
    showSpeakerViewId,

    keepSpeakerViewOnTopId,

    showHallId,
    showSourceNumbersId,
    showSpeakerNumbersId,
    showSpeakersId,
    showTripletsId,
    showSourceActivityId,
    showSpeakerActivityId,
    showSphereId,

    colorizeInputsId,
    resetInputPosId,
    resetMeterClipping,
    muteAllSpeakers,

    // Help menu
    aboutId,
    openManualENId,
    openManualFRId,

    // Player control
    playerPlayStopId
};

}
