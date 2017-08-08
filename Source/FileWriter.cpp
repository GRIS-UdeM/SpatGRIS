//
//  FileWriter.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-05-05.
//
//

#include "FileWriter.h"
#include "MainComponent.h"


FileWriter::FileWriter(MainContentComponent * parent) : Thread ("FileWriter")
{
    this->mainParent = parent;
    this->inSave     = false;
}

FileWriter::~FileWriter()
{
    
}

void FileWriter::recording(int numOutputs, int sampleRate) {
    String dir = this->mainParent->applicationProperties.getUserSettings()->getValue("lastRecordingDirectory");
    if (! File(dir).isDirectory()) {
        dir = File("~").getFullPathName();
    }
    FileChooser fc ("Choose a file to save...", dir + "/recording.wav" , "*.wav,*.aif", true);
    if (fc.browseForFileToSave (true)) {
        this->filePath = fc.getResults().getReference(0).getFullPathName();
        this->mainParent->applicationProperties.getUserSettings()->setValue("lastRecordingDirectory", 
                                                                            File(this->filePath).getParentDirectory().getFullPathName());
        this->numOutputs = numOutputs;
        this->sampleRate = sampleRate;
        this->inSave = true;
        this->startThread();
    }
}

void FileWriter::run()
{
    if (this->numOutputs < 1) {
        this->inSave = false;
        return;
    }

    unsigned int size = this->mainParent->getBufferCurentIndex();

    String channelName;
    File fileS = File(this->filePath);
    String fname = fileS.getFileNameWithoutExtension();
    String extF = fileS.getFileExtension();
    String parent = fileS.getParentDirectory().getFullPathName();

    if (extF == ".wav") {
        for (int i  = 0; i < this->numOutputs; ++i) {
            channelName = parent + "/" + fname + "_" + String(i+1).paddedLeft('0', 3) + extF;
            File fileC = File(channelName);
            fileC.deleteFile();
            FileOutputStream * outputTo = fileC.createOutputStream();
            AudioFormatWriter* writer;
            WavAudioFormat* waveAudioF = new WavAudioFormat();
            writer = waveAudioF->createWriterFor(outputTo, this->sampleRate, 1, 24, NULL, 0);
            AudioSampleBuffer* buffer = new AudioSampleBuffer(1, size);
            buffer->clear();
            buffer->addFrom(0, 0, &(this->mainParent->getBufferToRecord(i)[0]), size);
            writer->writeFromAudioSampleBuffer(*buffer, 0, size);
            delete waveAudioF;
            delete writer;
        }
    }
    else if (extF == ".aif") {
        AudioSampleBuffer* buffers = new AudioSampleBuffer(this->numOutputs, size);
        buffers->clear();

        for (int i  = 0; i < this->numOutputs; ++i) {
            buffers->addFrom(i, 0, &(this->mainParent->getBufferToRecord(i)[0]), size);
        }
        fileS.deleteFile();
        FileOutputStream * outputTo = fileS.createOutputStream();
        AudioFormatWriter* writer;
        AiffAudioFormat* aiffAudioF = new AiffAudioFormat();
        writer = aiffAudioF->createWriterFor(outputTo, this->sampleRate, this->numOutputs, 24, NULL, 0);
        writer->writeFromAudioSampleBuffer(*buffers, 0, size);
        
        delete aiffAudioF;
        delete writer;
    }

    this->inSave = false;
}

/*
void FileWriter::run_backup()
{
    if (this->numOutputs < 1) {
        this->inSave = false;
        return;
    }
    
    unsigned int size = this->mainParent->getBufferCurentIndex();
    AudioSampleBuffer* buffers=new AudioSampleBuffer(this->numOutputs,size);
    buffers->clear();
    
    for(int i  = 0; i < this->numOutputs; ++i) {
        buffers->addFrom(i, 0, &(this->mainParent->getBufferToRecord(i)[0]), size);
    }
    
    
    File fileS = File(this->filePath);
    fileS.deleteFile();
    FileOutputStream * outputTo = fileS.createOutputStream();
    
    String extF = fileS.getFileExtension();
    if (extF == ".wav") {
        AudioFormatWriter* writer;
        WavAudioFormat* waveAudioF = new WavAudioFormat();
        writer = waveAudioF->createWriterFor(outputTo, this->sampleRate, this->numOutputs, 24, NULL, 0);
        writer->writeFromAudioSampleBuffer(*buffers, 0, size);
        
        delete waveAudioF;
        delete writer;
    }
    else if (extF == ".aif") {
        AudioFormatWriter* writer;
        AiffAudioFormat* aiffAudioF = new AiffAudioFormat();
        writer = aiffAudioF->createWriterFor(outputTo,this->sampleRate, this->numOutputs, 24, NULL, 0);
        writer->writeFromAudioSampleBuffer(*buffers, 0, size);
        
        delete aiffAudioF;
        delete writer;
    }

    else if(extF == ".flac")
     {
     FlacAudioFormat* flacAudioF = new FlacAudioFormat();
     AudioFormatWriter* writer = flacAudioF->createWriterFor(outputTo, 48000, this->numOutputs, 24, NULL, 0);
     writer->writeFromAudioSampleBuffer(*buffers, 0, size);

     delete writer;
     delete flacAudioF;
     }

    this->inSave = false;
}
*/
