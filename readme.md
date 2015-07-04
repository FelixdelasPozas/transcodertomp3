Music Transcoder To MP3
=======================

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
Music Transcoder To MP3 is a cross-platform tool to transcode music files from multiple formats (music files, audio track of video files and module files) to MP3 format. 

## Options
The tool can be configured:
* the output files can be renamed according to the input file metadata and the specified reformatting options.
* large audio files can be splitted into tracks if a CUE sheet is provided in the same folder of the audio file.
* ID3v1/ID3v2 tags can be removed if the input file is already in MP3 format.
* can create M3U playlists in the input folders after the files have been converted.
* the cover picture of the input file, if present in the file metadata, can be extracted to disk. 

## Input file formats
The following file formats are detected and supported by the tool as input files:
* **Audio formats**: flac, ogg, ape, wav, wma, m4a, voc, wv and mp3.
* **Video formats**: mp4, avi, ogv and webm.
* **Module formats**: 669, amf, apun, dsm, far, gdm, it, imf, mod, med, mtm, okt, s3m, stm, stx, ult, uni, xt and xm.

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows or [gcc](http://gcc.gnu.org/) on Linux.

## External dependencies:
The following libraries are required:
* [lame](http://lame.sourceforge.net/) - Lame Ain't An Mp3 Encoder.
* [libav](https://libav.org/) - Open source audio and video processing tools.
* [libcue](http://sourceforge.net/projects/libcue/) - CUE sheet parser library.
* [id3lib](http://id3lib.sourceforge.net/) - ID3 tagging library.
* [libopenmt](http://lib.openmpt.org/) - OpenMPT based module player library.
* [Qt opensource framework](http://www.qt.io/).

# Install
The only current option is build from source as binaries are not provided. 

# Screenshots
Configuration dialog.

![Configuration Dialog](https://cloud.githubusercontent.com/assets/12167134/7867871/e2fcceba-0578-11e5-83bd-e806f5f95672.jpg)

Simple main dialog.

![Main dialog](https://cloud.githubusercontent.com/assets/12167134/7867872/e2fd4c28-0578-11e5-93bb-56c7ee8b26df.jpg)

Dialog shown while processing files.

![Process Dialog](https://cloud.githubusercontent.com/assets/12167134/7867873/e48c0714-0578-11e5-8de4-ba1b44b1b72f.jpg)

# Repository information
**Version**: 1.0.0

**Status**: finished

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |   11          |  563        |    364           |2245  |
| C/C++ Header                 |   10          |  242        |    628           | 443  |
| CMake                        |    1          |   18        |     10           |  87  |
| **Total**                    |   **22**      |  **823**    |   **1002**       |**2775**|
