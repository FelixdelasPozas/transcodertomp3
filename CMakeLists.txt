project(MusicConverter)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (MUSIC_CONVERTER_VERSION_MAJOR 0)
set (MUSIC_CONVERTER_VERSION_MINOR 1)
set (MUSIC_CONVERTER_VERSION_PATCH 0)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if(DEFINED MUSIC_CONVERTER_VERSION_MAJOR)
  configure_file("${PROJECT_SOURCE_DIR}/resources.rc.in" "${PROJECT_BINARY_DIR}/resources.rc")
  set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
  enable_language(RC)
endif(DEFINED MUSIC_CONVERTER_VERSION_MAJOR)

find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

# LAME library
set(LAME_DIR "D:/Desarrollo/Bibliotecas/lame-3.99.5")
set(LAME_INCLUDE_DIR ${LAME_DIR}/include)
set(LAME_LIBRARY     ${LAME_DIR}/libmp3lame/.libs/libmp3lame.a)

# libav library
set(LIBAV_DIR "D:/Desarrollo/Bibliotecas/libav-11.3")
set(LIBAV_INCLUDE_DIR ${LIBAV_DIR})
set(LIBAV_LIBRARIES   ${LIBAV_DIR}/libavformat/libavformat.a 
                      ${LIBAV_DIR}/libavcodec/libavcodec.a 
                      ${LIBAV_DIR}/libavutil/libavutil.a 
                      ${LIBAV_DIR}/libavresample/libavresample.a)

# libcue library
set(LIBCUE_DIR "D:/Desarrollo/Bibliotecas/libcue-1.4.0/src")
set(LIBCUE_INCLUDE_DIR ${LIBCUE_DIR})
set(LIBCUE_LIBRARY     ${LIBCUE_DIR}/libcue/.libs/libcue.a)

# libmikmod library
set(LIBMIKMOD_DIR "D:/Desarrollo/Bibliotecas/libmikmod-3.3.7")
set(LIBMIKMOD_INCLUDE_DIR ${LIBMIKMOD_DIR}/include)
set(LIBMIKMOD_LIBRARY     ${LIBMIKMOD_DIR}/../libmikmod-build/libmikmod.a)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  ${LAME_INCLUDE_DIR}
  ${LIBAV_INCLUDE_DIR}
  ${LIBCUE_INCLUDE_DIR}
  ${LIBMIKMOD_INCLUDE_DIR}
  )

set(CMAKE_CXX_FLAGS " -Wall -Wno-deprecated -std=c++11 -fpermissive -mwindows")

# Add Qt Resource files
qt4_add_resources(RESOURCES
	rsc/resources.qrc
	)

qt4_wrap_ui(CORE_UI
  # .ui for Qt
  MusicConverter.ui
  ProcessDialog.ui
  )
	
qt4_wrap_cpp(CORE_MOC
  # .h for Qt
  MusicConverter.h
  ProcessDialog.h
  ConverterThread.h
  )

set (CORE_SOURCES
  # project files
  ${CORE_SOURCES}
  ${RESOURCES}
  ${CORE_MOC}
  ${CORE_UI}
  Main.cpp
  MusicConverter.cpp
  ProcessDialog.cpp
  Utils.cpp
  ConverterThread.cpp
)

set(CORE_EXTERNAL_LIBS
  ${QT_LIBRARIES}
  ${LAME_LIBRARY}
  ${LIBAV_LIBRARIES}
  ${LIBCUE_LIBRARY}
  ${LIBMIKMOD_LIBRARY}
)

add_executable(MusicConverter ${CORE_SOURCES})
target_link_libraries (MusicConverter ${CORE_EXTERNAL_LIBS})