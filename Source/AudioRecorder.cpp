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

#include "AudioRecorder.h"

//==============================================================================
AudioRecorder::AudioRecorder() : backgroundThread("Audio Recorder Thread")
{
}

//==============================================================================
void AudioRecorder::startRecording(juce::File const & file, unsigned const sampleRate, juce::String const & extF)
{
    stop();

    backgroundThread.startThread();

    // Create an OutputStream to write to our destination file.
    file.deleteFile();
    std::unique_ptr<juce::FileOutputStream> fileStream(file.createOutputStream());

    juce::AudioFormatWriter * writer;

    if (fileStream != nullptr) {
        // Now create a writer object that writes to our output stream...
        if (extF == ".wav") {
            juce::WavAudioFormat wavFormat;
            writer = wavFormat.createWriterFor(fileStream.get(), sampleRate, 1, 24, NULL, 0);
        } else {
            juce::AiffAudioFormat aiffFormat;
            writer = aiffFormat.createWriterFor(fileStream.get(), sampleRate, 1, 24, NULL, 0);
        }

        if (writer != nullptr) {
            fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now
            // using it)

            // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
            // write the data to disk on our background thread.
            mThreadedWriter.reset(new juce::AudioFormatWriter::ThreadedWriter(writer, backgroundThread, 32768));

            // And now, swap over our active writer pointer so that the audio callback will start using it..
            const juce::ScopedLock sl(mWriterLock);
            mActiveWriter = mThreadedWriter.get();
        }
    }
}

//==============================================================================
void AudioRecorder::stop()
{
    if (mActiveWriter == nullptr) {
        return;
    }

    // First, clear this pointer to stop the audio callback from using our writer object.
    {
        const juce::ScopedLock sl(mWriterLock);
        mActiveWriter = nullptr;
    }

    // Now we can delete the writer object. It's done in this order because the deletion could
    // take a little time while remaining data gets flushed to disk, so it's best to avoid blocking
    // the audio callback while this happens.
    mThreadedWriter.reset();

    // Stop the background thread.
    backgroundThread.stopThread(100);
}

//==============================================================================
void AudioRecorder::recordSamples(float ** samples, int numSamples) const
{
    const juce::ScopedLock sl(mWriterLock);
    mActiveWriter.load()->write(samples, numSamples);
}
