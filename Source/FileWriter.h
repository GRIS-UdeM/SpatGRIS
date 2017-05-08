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
    void recording (int numOutputs = 1);
    bool isSavingRun(){ return this->inSave; }
    
    //Tread=============
    void run() override;
    
private:
    
    MainContentComponent * mainParent;
    int numOutputs = 0;
    bool inSave = false;
    String filePath ;
};


#endif /* FileWriter_h */


