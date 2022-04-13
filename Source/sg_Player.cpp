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
//==============================================================================
Player::Player(MainContentComponent & parent)
    : mMainContentComponent(parent)
    , mManager{}
    , mWavFile{}
    , mWavFormat{ nullptr }
{
    // mManager.registerBasicFormats();
    //// audio file to map in memory
    // mWavFile = "C:/musik.wav";
    // jassert(mWavFile.existsAsFile());
    //// audio format to use
    // mWavFormat = mManager.findFormatForFileExtension(mWavFile.getFileExtension());
    //// make sure the format is valid and registered
    // jassert(mWavFormat);
    // mReader = mWavFormat->createMemoryMappedReader(mWavFile);
    //// make sure the reader is created
    // jassert(mReader);
    //// make sure the file is mono
    // jassert(mReader->getChannelLayout().size() == 1);

    //// create an audio source that takes ownership of the reader
    //// this audio source only knows how to get the next audio block
    // juce::AudioFormatReaderSource source{ mReader, true };
    //// create an other audio source that can do start, stop, but more importantly, resampling
    // juce::AudioTransportSource resampled_source{};
    // resampled_source.setSource(&source);
    //// inform the source about sample rate
    // auto const & data{ mMainContentComponent.getData() };
    // auto const sampleRate{ data.appData.audioSettings.sampleRate };
    // auto const bufferSize{ data.appData.audioSettings.bufferSize };
    // resampled_source.prepareToPlay(bufferSize, sampleRate);

    DBG("Player constructor.");
}

//==============================================================================
Player::~Player()
{
    AudioManager::getInstance().playerOff();
    DBG("Player destructor.");
}

//==============================================================================
bool Player::loadWavFilesAndSpeakerSetup(juce::File const & folder)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (validateWavFilesAndSpeakerSetup(folder)) {
        for (const auto & filenameThatWasFound :
            folder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.wav")) {
            mAudioFileSet.push_back(filenameThatWasFound);
        }
        DBG("Wav files loaded.");

        if (!AudioManager::getInstance().playerExists()) {
            mManager.registerBasicFormats();
        }
        // audio file to map in memory
        mWavFile = "C:/musik2.wav";
        jassert(mWavFile.existsAsFile());
        // audio format to use
        mWavFormat = mManager.findFormatForFileExtension(mWavFile.getFileExtension());
        // make sure the format is valid and registered
        jassert(mWavFormat);
        auto * reader = mWavFormat->createMemoryMappedReader(mWavFile);
        // make sure the reader is created
        jassert(reader);
        // make sure the file is mono
        jassert(reader->getChannelLayout().size() == 1);
        reader->mapEntireFile();

        // create an audio source that takes ownership of the reader
        // this audio source only knows how to get the next audio block
        //juce::AudioFormatReaderSource source{ reader, true };
        // 
        // create an other audio source that can do start, stop, but more importantly, resampling
        // juce::AudioTransportSource resampled_source{};
        // resampled_source.setSource(&source);
        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        mTransportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        mReaderSource.reset(newSource.release());

        // inform the source about sample rate
        //auto const & data{ mMainContentComponent.getData() };
        //auto const sampleRate{ data.appData.audioSettings.sampleRate };
        //auto const bufferSize{ data.appData.audioSettings.bufferSize };
        // resampled_source.prepareToPlay(bufferSize, sampleRate);
        //AudioManager::getInstance().setPlayerSource(&AFRSource, sampleRate, bufferSize);
        //AudioManager::getInstance().playerOn();
        //mTransportSource.prepareToPlay(bufferSize, sampleRate);
        //mTransportSource.setPosition(0.0);
        //mTransportSource.start();
        //AudioManager::getInstance().initPlayer(*mReaderSource, mTransportSource);

        return true;
    }

    return false;
}

void Player::playAudio()
{
    auto const & data{ mMainContentComponent.getData() };
    auto const sampleRate{ data.appData.audioSettings.sampleRate };
    auto const bufferSize{ data.appData.audioSettings.bufferSize };

    mTransportSource.prepareToPlay(bufferSize, sampleRate);
    mTransportSource.setPosition(0.0);
    //mTransportSource.start();
    AudioManager::getInstance().initPlayer(mTransportSource);
}

bool Player::validateWavFilesAndSpeakerSetup(juce::File const & folder)
{
    juce::StringArray wavFileList;
    juce::StringArray speakerList;
    tl::optional<SpeakerSetup> speakerSetup;
    juce::XmlElement xml("tmp");

    auto const displayError = [&](juce::String const & message) {
        juce::NativeMessageBox::show(juce::MessageBoxOptions{}
                                         .withTitle("Unable to open Speaker Setup and Audio Files folder")
                                         .withMessage(message)
                                         .withIconType(juce::MessageBoxIconType::WarningIcon));
    };

    for (const auto & filenameThatWasFound :
         folder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*")) {
        // filenameThatWasFound.get
        // DBG("Filename : " << filenameThatWasFound.getFileName());
        // DBG("File path : " << filenameThatWasFound.getFullPathName());
        // DBG("File extension : " << filenameThatWasFound.getFileExtension());

        jassert(filenameThatWasFound.existsAsFile());

        if (filenameThatWasFound.getFileExtension() == ".wav") {
            // wavFileList.add(filenameThatWasFound.getFileName());
            wavFileList.add(filenameThatWasFound.getFileNameWithoutExtension().substring(
                filenameThatWasFound.getFileNameWithoutExtension().lastIndexOfChar('-') + 1));
            // DBG("Found wav file : " << filenameThatWasFound.getFileName());

        } else if (filenameThatWasFound.getFileExtension() == ".xml") {
            speakerSetup
                = mMainContentComponent.playerExtractSpeakerSetup(juce::File(filenameThatWasFound.getFullPathName()));

            if (!speakerSetup) {
                return false;
            }

            juce::XmlDocument xmlDoc(juce::File(filenameThatWasFound.getFullPathName()));
            xml = *xmlDoc.getDocumentElement();

            // DBG("Found speaker setup file.");
        }
    }

    for (auto const * speaker : xml.getChildIterator()) {
        auto const tagName{ speaker->getTagName() };
        if (tagName.startsWith(SpeakerData::XmlTags::MAIN_TAG_PREFIX)) {
            speakerList.add(tagName.substring(juce::String(SpeakerData::XmlTags::MAIN_TAG_PREFIX).length()));
            // DBG("speaker number : " <<
            // tagName.substring(juce::String(SpeakerData::XmlTags::MAIN_TAG_PREFIX).length()));
        }
    }

    if (speakerList.size() != wavFileList.size()) {
        displayError("Audio files do not match Speaker Setup.\n" + juce::String(wavFileList.size()) + " Audio files\n"
                     + juce::String(speakerList.size()) + " Speakers");
        return false;
    }

    for (auto const elem : wavFileList) {
        if (!speakerList.contains(elem)) {
            displayError("Audio file list does not match Speaker Setup.");
            return false;
        }
    }

    return true;
}

} // namespace gris
