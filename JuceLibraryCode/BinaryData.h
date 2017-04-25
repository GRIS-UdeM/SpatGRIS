/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#ifndef BINARYDATA_H_7363894_INCLUDED
#define BINARYDATA_H_7363894_INCLUDED

namespace BinaryData
{
    extern const char*   logo512_png;
    const int            logo512_pngSize = 229225;

    extern const char*   SinkinSans400Regular_otf;
    const int            SinkinSans400Regular_otfSize = 35872;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Number of elements in the namedResourceList array.
    const int namedResourceListSize = 2;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();
}

#endif
