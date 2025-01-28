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
* [libav](https://github.com/libav/libav) - Open source audio and video processing tools.
* [libcue](https://github.com/lipnitsk/libcue) - CUE sheet parser library.
* [tagparser](https://github.com/Martchus/tagparser/) - Tag Parser Library.
* [libopenmt](http://lib.openmpt.org/) - OpenMPT based module player library.
* [Qt opensource framework](http://www.qt.io/).

# Install

Download and execute the ![latest release](https://github.com/FelixdelasPozas/transcodertomp3/releases) installer.

# Screenshots
Configuration dialog.

![Configuration Dialog](https://user-images.githubusercontent.com/12167134/107889759-0c857000-6f15-11eb-9764-cb135c585c75.png)

Simple main dialog.

![Main dialog](https://cloud.githubusercontent.com/assets/12167134/7867872/e2fd4c28-0578-11e5-93bb-56c7ee8b26df.jpg)

Dialog shown while processing files.

![Process Dialog](https://cloud.githubusercontent.com/assets/12167134/7867873/e48c0714-0578-11e5-8de4-ba1b44b1b72f.jpg)

# Repository information
**Version**: 1.4.4

**Status**: finished

**License**: GNU General Public License 3

**cloc statistics**

| Language                     |files          |blank        |comment        |code  |
|:-----------------------------|--------------:|------------:|--------------:|-----:|
| C++                          |   11          |  595        |    358        |2515  |
| C/C++ Header                 |   10          |  248        |    710        | 546  |
| CMake                        |    1          |   25        |     14        |  99  |
| **Total**                    |   **22**      |  **868**    |   **1082**    |**3160**|
