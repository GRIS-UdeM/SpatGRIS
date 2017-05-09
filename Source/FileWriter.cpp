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
}

FileWriter::~FileWriter()
{
}

void FileWriter::recording(int numOutputs, int sampleRate)
{
    FileChooser fc ("Choose a file to save...",File::getCurrentWorkingDirectory(), "*.wav,*.aif", true);
    if (fc.browseForFileToSave (true))
    {
        this->filePath = fc.getResults().getReference(0).getFullPathName();
        this->numOutputs = numOutputs;
        this->sampleRate = sampleRate;
        this->inSave = true;
        this->startThread();
    }
}

void FileWriter::run()
{
    if(this->numOutputs < 1){
        this->inSave = false;
        return;
    }
    int size= this->mainParent->getBufferToRecord(0).size();
    AudioSampleBuffer* buffers=new AudioSampleBuffer(this->numOutputs,size);
    buffers->clear();
    
    for(int i  = 0; i < this->numOutputs; ++i)
    {
        buffers->addFrom(i, 0, &(this->mainParent->getBufferToRecord(i)[0]), size);
    }
    
    
    File fileS = File(this->filePath);
    fileS.deleteFile();
    FileOutputStream * outputTo = fileS.createOutputStream();
    AudioFormatWriter* writer;
    
    String extF = fileS.getFileExtension();
    if(extF == ".wav")
    {
        WavAudioFormat* waveAudioF = new WavAudioFormat();
        writer = waveAudioF->createWriterFor(outputTo, this->sampleRate, this->numOutputs, 24, NULL, 0);
        writer->writeFromAudioSampleBuffer(*buffers, 0, size);
        
        delete waveAudioF;
    }
    else if(extF == ".aif")
    {
        AiffAudioFormat* aiffAudioF = new AiffAudioFormat();
        writer = aiffAudioF->createWriterFor(outputTo,this->sampleRate, this->numOutputs, 24, NULL, 0);
        writer->writeFromAudioSampleBuffer(*buffers, 0, size);
        
        delete aiffAudioF;
    }
    delete writer;
    /*else if(extF == ".flac")
    {
        FlacAudioFormat* flacAudioF = new FlacAudioFormat();
        AudioFormatWriter* writer = flacAudioF->createWriterFor(outputTo, 48000, this->numOutputs, 24, NULL, 0);
        writer->writeFromAudioSampleBuffer(*buffers, 0, size);
        
        delete writer;
        delete flacAudioF;
    }*/
    
    
    
    this->inSave = false;
}
