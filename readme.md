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

If you want to support this project you can do it on [Ko-fi](https://ko-fi.com/felixdelaspozas).

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

MusicTranscoder is available for Windows 10 onwards. You can download the lastest installer from the [releases page](https://github.com/FelixdelasPozas/transcodertomp3/releases). Neither the application or the installer are digitally signed so the system will ask for approval before running it the first time.

The last version compatible with Windows 7 & 8 is version 1.4.4, you can download it [here](https://github.com/FelixdelasPozas/transcodertomp3/releases/tag/1.4.4).

# Screenshots
Configuration dialog.

![Configuration Dialog](https://github.com/user-attachments/assets/458966f0-5ed9-41da-aeb7-20926d2eed5a)

Simple main dialog.

![Main dialog](https://github.com/user-attachments/assets/a2802058-fbf4-4b55-a767-36a164882379)

Dialog shown while processing files.

![Process Dialog](https://github.com/user-attachments/assets/f2f7c919-a791-4004-b96c-c9886028782b)

# Repository information
**Version**: 1.5.1

**Status**: finished

**License**: GNU General Public License 3

**cloc statistics**

| Language                     |files          |blank        |comment        |code     |
|:-----------------------------|--------------:|------------:|--------------:|--------:|
| C++                          |   12          |  632        |    403        | 2700    |
| C/C++ Header                 |   11          |  276        |    796        |  608    |
| CMake                        |    1          |   23        |     19        |  104    |
| **Total**                    |   **24**      |  **931**    |   **1211**    | **3412**|
