/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "AudioRenderer.h"

#include "JackClient.h"

//==============================================================================
AudioRenderer::AudioRenderer()
    : juce::ThreadWithProgressWindow("Merging recorded mono files into an interleaved multi-channel file...",
                                     true,
                                     false)
{
    setStatusMessage("Initializing...");
}

//==============================================================================
void AudioRenderer::prepareRecording(juce::File const & file,
                                     juce::Array<juce::File> const & fileNames,
                                     unsigned const sampleRate)
{
    mFileToRecord = file;
    mFiles = fileNames;
    mSampleRate = sampleRate;
}

//==============================================================================
void AudioRenderer::run()
{
    static auto constexpr BLOCK_SIZE{ 2048u };

    auto const factor{ std::pow(2.0f, 31.0f) };
    auto const numberOfChannels{ mFiles.size() };
    auto const extF{ mFiles[0].getFileExtension() };

    juce::AudioBuffer<float> buffer{ numberOfChannels, BLOCK_SIZE };

    mFormatManager.registerBasicFormats();
    juce::AudioFormatReader * readers[MAX_OUTPUTS];
    for (int i{}; i < numberOfChannels; ++i) {
        readers[i] = mFormatManager.createReaderFor(mFiles[i]);
    }

    auto const duration{ static_cast<unsigned int>(readers[0]->lengthInSamples) };
    auto const howManyPasses{ duration / BLOCK_SIZE };

    // Create an OutputStream to write to our destination file.
    mFileToRecord.deleteFile();
    auto fileStream{ mFileToRecord.createOutputStream() };

    std::unique_ptr<juce::AudioFormatWriter> writer{};

    if (fileStream != nullptr) {
        // Now create a writer object that writes to our output stream...
        if (extF == ".wav") {
            juce::WavAudioFormat wavFormat;
            writer.reset(wavFormat.createWriterFor(fileStream.get(), mSampleRate, numberOfChannels, 24, NULL, 0));
        } else {
            juce::AiffAudioFormat aiffFormat;
            writer.reset(aiffFormat.createWriterFor(fileStream.get(), mSampleRate, numberOfChannels, 24, NULL, 0));
        }

        if (writer) {
            fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now
            // using it)
        }
    }

    unsigned int numberOfPasses{};
    std::array<int, BLOCK_SIZE> data;
    auto * const dataStart{ data.data() };
    while ((numberOfPasses * BLOCK_SIZE) < duration) {
        // this will update the progress bar on the dialog box
        setProgress(static_cast<double>(numberOfPasses) / static_cast<double>(howManyPasses));

        setStatusMessage("Processing...");

        for (int i{}; i < numberOfChannels; ++i) {
            readers[i]->read(&dataStart, 1, numberOfPasses * BLOCK_SIZE, BLOCK_SIZE, false);
            for (unsigned j{}; j < BLOCK_SIZE; ++j) {
                buffer.setSample(i, j, data[j] / factor);
            }
        }
        writer->writeFromAudioSampleBuffer(buffer, 0, BLOCK_SIZE);
        ++numberOfPasses;
        wait(1); // TODO : why wait?
    }

    setProgress(-1.0); // setting a value beyond the range 0 -> 1 will show a spinning bar.
    setStatusMessage("Finishing the creation of the multi-channel file!");

    // Delete the monophonic files.
    for (auto const & it : mFiles) {
        it.deleteFile();
        wait(50); // TODO : why wait ?
    }
    wait(1000); // TODO : why wait ?
}

//==============================================================================
void AudioRenderer::threadComplete(bool /*userPressedCancel*/)
{
    // thread finished normally.
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                           "Multi-channel processing window",
                                           "Merging files finished!");

    // Clean up by deleting our thread object.
    delete this;
}
