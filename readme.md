Music Transcoder To MP3
=======================
Music Transcoder To MP3 is a cross-platform tool to transcode music files from multiple formats (music files, audio track of video files and module files) to MP3 format. 
Optionally:
* the output files could be renamed according to the input file metadata and the specified reformatting options.
* large audio files can be splitted into tracks if a CUE sheet is provided in the same forder of the audio file.
* IDev1/ID3v2 tags can be removed if the input file is already in MP3 format.
* can create M3U playlists in the input folders after the files have been converted.
* the cover picture of the input file, if present in the file metadata, can be extracted to disk. 

# Input file formats
The following file formats are detected and supported by the tool as input files:
* *Audio formats* : flac, ogg, ape, wav, wma, m4a, voc, wv and mp3.
* *Video formats* : mp4, avi, ogv and webm.
* *Module formats* : 669, amf, apun, dsm, far, gdm, it, imf, mod, med, mtm, okt, s3m, stm, stx, ult, uni, xt and xm.

# Requirements
## To build the tool depends on:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows or [gcc](http://gcc.gnu.org/) on Linux.

## External dependencies:
The following libraries are required to build the tool:
* [lame](http://lame.sourceforge.net/) - Lame Ain't An Mp3 Encoder.
* [libav](https://libav.org/) - Open source audio and video processing tools.
* [libcue](http://sourceforge.net/projects/libcue/) - CUE sheet parser library.
* [id3lib](http://id3lib.sourceforge.net/) - ID3 tagging library.
* [libopenmt](http://lib.openmpt.org/) - OpenMPT based module player library.
* [Qt opensource framework](http://www.qt.io/).

# Installing
The only current option is build from source as binaries are not provided. 

# Screenshots
Comming soon...