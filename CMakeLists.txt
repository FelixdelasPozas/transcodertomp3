project(MusicTranscoder)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (MUSIC_TRANSCODER_VERSION_MAJOR 1)
set (MUSIC_TRANSCODER_VERSION_MINOR 1)
set (MUSIC_TRANSCODER_VERSION_PATCH 3)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Widgets)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if(DEFINED MINGW)
  configure_file("${PROJECT_SOURCE_DIR}/resources.rc.in" "${PROJECT_BINARY_DIR}/resources.rc")
  set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
  enable_language(RC)
endif(DEFINED MINGW)


# LAME library
set(LAME_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/lame-3.99.5/include")
set(LAME_LIBRARY     "D:/Desarrollo/Bibliotecas/lame-3.99.5/libmp3lame/.libs/libmp3lame.a")

# libav library
set(LIBAV_DIR "D:/Desarrollo/Bibliotecas/libav-11.3")
set(LIBAV_INCLUDE_DIR ${LIBAV_DIR})
set(LIBAV_LIBRARIES   ${LIBAV_DIR}/libavformat/libavformat.a 
                      ${LIBAV_DIR}/libavcodec/libavcodec.a 
                      ${LIBAV_DIR}/libavutil/libavutil.a 
                      ${LIBAV_DIR}/libavresample/libavresample.a)

# libcue library
set(LIBCUE_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/libcue-1.4.0/src")
set(LIBCUE_LIBRARY     "D:/Desarrollo/Bibliotecas/libcue-1.4.0/src/libcue/.libs/libcue.a")

# taglib library
set(TAGLIB_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/taglib-1.10/taglib")
set(TAGLIB_LIBRARY_DIR "D:/Desarrollo/Bibliotecas/taglib-build")
set(TAGLIB_LIBRARIES   ${TAGLIB_LIBRARY_DIR}/taglib/libtag.a)

# libopenmpt library
set(LIBOPENMPT_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/libopenmpt-0.2.4954-beta12-windows")
set(LIBOPENMPT_LIBRARY     "D:/Desarrollo/Bibliotecas/libopenmpt-0.2.4954-beta12-windows/bin/libopenmpt.a")

#include zlib
set(ZLIB_LIBRARY "D:/Desarrollo/Bibliotecas/zlib-build/libzlibstatic.a")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  ${LAME_INCLUDE_DIR}
  ${LIBAV_INCLUDE_DIR}
  ${LIBCUE_INCLUDE_DIR}
  ${LIBOPENMPT_INCLUDE_DIR}
  ${TAGLIB_INCLUDE_DIR}
  ${TAGLIB_LIBRARY_DIR}
  )

set(CMAKE_CXX_FLAGS " -Wall -Wno-deprecated -std=c++11 -mwindows -DTAGLIB_STATIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

# Add Qt Resource files
qt5_add_resources(RESOURCES
  rsc/resources.qrc
)

qt5_wrap_ui(CORE_UI
  # .ui for Qt
  MusicTranscoder.ui
  ProcessDialog.ui
  AboutDialog.ui
  ConfigurationDialog.ui
)
	
set (CORE_SOURCES
  # project files
  ${CORE_SOURCES}
  ${RESOURCES}
  ${CORE_UI}
  Main.cpp
  MusicTranscoder.cpp
  ProcessDialog.cpp
  Utils.cpp
  Worker.cpp
  AudioWorker.cpp
  MP3Worker.cpp
  ModuleWorker.cpp
  AboutDialog.cpp
  ConfigurationDialog.cpp
  PlaylistWorker.cpp
)

SET_SOURCE_FILES_PROPERTIES(${CORE_SOURCES} PROPERTIES OBJECT_DEPENDS "${CORE_UI}")

set(CORE_EXTERNAL_LIBS
  ${QT_LIBRARIES}
  ${LAME_LIBRARY}
  ${LIBAV_LIBRARIES}
  ${LIBCUE_LIBRARY}
  ${TAGLIB_LIBRARIES}
  ${LIBOPENMPT_LIBRARY}
  ${ZLIB_LIBRARY}
)
  
add_executable(MusicTranscoder ${CORE_SOURCES})
target_link_libraries (MusicTranscoder ${CORE_EXTERNAL_LIBS})
qt5_use_modules(MusicTranscoder Widgets)