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

#include "sg_Player.hpp"

#include "sg_MainComponent.hpp"

namespace gris
{
Player::Player(MainContentComponent & parent)
    : mMainContentComponent(parent)
    , manager{}
    , file{}
    , wavFormat{ nullptr }
    , reader{ nullptr }
{
    manager.registerBasicFormats();
    // audio file to map in memory
    file = "C:/musik.wav";
    jassert(file.existsAsFile());
    // audio format to use
    wavFormat = manager.findFormatForFileExtension(file.getFileExtension());
    // make sure the format is valid and registered
    jassert(wavFormat);
    reader = wavFormat->createMemoryMappedReader(file);
    // make sure the reader is created
    jassert(reader);
    // make sure the file is mono
    jassert(reader->getChannelLayout().size() == 1);

    // create an audio source that takes ownership of the reader
    // this audio source only knows how to get the next audio block
    juce::AudioFormatReaderSource source{ reader, true };
    // create an other audio source that can do start, stop, but more importantly, resampling
    juce::AudioTransportSource resampled_source{};
    resampled_source.setSource(&source);
    // inform the source about sample rate
    // resampled_source.prepareToPlay(/*buffer size*/, /*sample rate*/);

    DBG("Player constructor.");
}

Player::~Player()
{
    DBG("Player destructor.");
}

void Player::loadWavFilesAndSpeakerSetupFolder()
{
}

bool Player::loadWavFilesAndSpeakerSetup(juce::File const & folder)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::StringArray fileList;

    for (const auto & filenameThatWasFound :
         folder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*")) {
        // filenameThatWasFound.get
        fileList.add(filenameThatWasFound.getFileName());
    }

    for (const auto & filename : fileList) {
        DBG("File in player folder : " << filename);
    }

    return false;
}

} // namespace gris
