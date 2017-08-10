/*
 This file is part of ServerGris.
 
 Developers: Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FileWriter_h
#define FileWriter_h


#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent;

class FileWriter :  public Thread
{
public:
    
    FileWriter(MainContentComponent * parent) ;
    ~FileWriter();
    
    //===================================================================
    void recording (int numOutputs = 1, int sampleRate = 48000);
    bool isSavingRun(){ return this->inSave; }
    
    
private:
    
    //Tread=============
    void run() override;
    
    MainContentComponent * mainParent;
    String  filePath ;
    int     numOutputs;
    float   sampleRate;
    bool    inSave;
    
};


#endif /* FileWriter_h */


