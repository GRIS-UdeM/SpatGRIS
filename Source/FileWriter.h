//
//  FileWriter.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-05-05.
//
//

#ifndef FileWriter_h
#define FileWriter_h


#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent;

class FileWriter :  public Thread
{
public:
    
    FileWriter(MainContentComponent * parent) ;
    ~FileWriter();
    
    //==============================================================================
    void recording (int numOutputs = 1, int sampleRate = 48000);
    bool isSavingRun(){ return this->inSave; }
    
    
    
private:
    
    //Tread=============
    void run() override;
    
    MainContentComponent * mainParent;
    int     numOutputs;
    float   sampleRate;
    bool    inSave = false;
    String  filePath ;
};


#endif /* FileWriter_h */


