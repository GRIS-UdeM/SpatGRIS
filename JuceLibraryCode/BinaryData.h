/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   JAS_jpil;
    const int            JAS_jpilSize = 175;

    extern const char*   ServerGRIS_1_0_Manual_pdf;
    const int            ServerGRIS_1_0_Manual_pdfSize = 14198125;

    extern const char*   ServerGRIS_icon_doc_icns;
    const int            ServerGRIS_icon_doc_icnsSize = 481666;

    extern const char*   ServerGRIS_icon_doc_png;
    const int            ServerGRIS_icon_doc_pngSize = 403691;

    extern const char*   ServerGRIS_icon_small_png;
    const int            ServerGRIS_icon_small_pngSize = 24399;

    extern const char*   splash_screen_png;
    const int            splash_screen_pngSize = 440164;

    extern const char*   SinkinSans400Regular_otf;
    const int            SinkinSans400Regular_otfSize = 35872;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 7;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
