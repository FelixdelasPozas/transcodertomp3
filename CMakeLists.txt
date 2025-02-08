cmake_minimum_required (VERSION 3.5.0)

project(MusicTranscoder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Version Number
set (MUSIC_TRANSCODER_VERSION_MAJOR 1)
set (MUSIC_TRANSCODER_VERSION_MINOR 5)
set (MUSIC_TRANSCODER_VERSION_PATCH 0)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Widgets)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

string(TIMESTAMP CURRENT_YEAR "%Y")

if(DEFINED MINGW)
  configure_file("${PROJECT_SOURCE_DIR}/resources.rc.in" "${PROJECT_BINARY_DIR}/resources.rc")
  configure_file("${PROJECT_SOURCE_DIR}/installer/script.iss.in" "${PROJECT_BINARY_DIR}/script.iss")
  set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
  enable_language(RC)
endif(DEFINED MINGW)


# LAME library 3.100.1
set(LAME_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/lame-3.100.1/include")
set(LAME_LIBRARY     "D:/Desarrollo/Bibliotecas/lame-3.100.1/libmp3lame/.libs/libmp3lame.a")

# libav library 13
set(LIBAV_DIR "D:/Desarrollo/Bibliotecas/libav-13")
set(LIBAV_INCLUDE_DIR ${LIBAV_DIR})
set(LIBAV_LIBRARIES   ${LIBAV_DIR}/libavformat/libavformat.a 
                      ${LIBAV_DIR}/libavcodec/libavcodec.a 
                      ${LIBAV_DIR}/libavutil/libavutil.a 
                      ${LIBAV_DIR}/libavresample/libavresample.a)
					  
# libvpx library 1.13.1
set(LIBVPX_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/libvpx")
set(LIBVPX_LIBRARY ${LIBVPX_INCLUDE_DIR}/libvpx.a)					  

# libcue library 2.3.0
set(LIBCUE_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/libcue-2.3.0")
set(LIBCUE_LIBRARY     "D:/Desarrollo/Bibliotecas/libcue-build/libcue.a")

# tagparser library 12.1.1
set(TAGPARSER_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/tag-parser/source"
                          "D:/Desarrollo/Bibliotecas/tag-parser/build")
set(TAGPARSER_LIBRARY_DIR "D:/Desarrollo/Bibliotecas/tag-parser/build")
set(TAGPARSER_LIBRARIES   ${TAGPARSER_LIBRARY_DIR}/tag-parser/libtag_parser.a
                          "D:/Desarrollo/Bibliotecas/tag-parser/build/c++utilities/libc++utilities.a")

# # libopenmpt library 0.7.3
# set(LIBOPENMPT_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/libopenmpt-0.7.3+release")
# set(LIBOPENMPT_LIBRARY     "D:/Desarrollo/Bibliotecas/libopenmpt-0.7.3+release/.libs/libopenmpt.a")

#include zlib 1.3
set(ZLIB_LIBRARY "D:/Desarrollo/Bibliotecas/zlib-build/libzlibstatic.a")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -m64 -Wall -mwindows -Wno-deprecated")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt6Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt6Core_EXECUTABLE_COMPILE_FLAGS}")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  ${LAME_INCLUDE_DIR}
  ${LIBAV_INCLUDE_DIR}
  ${LIBCUE_INCLUDE_DIR}
#  ${LIBOPENMPT_INCLUDE_DIR}
  ${TAGPARSER_INCLUDE_DIR}
  ${LIBVPX_INCLUDE_DIR}
  ${Qt6Widgets_INCLUDE_DIRS}
  ${Qt6Core_INCLUDE_DIRS}
  )

# Add Qt Resource files
qt6_add_resources(RESOURCES
  rsc/resources.qrc
)

qt6_wrap_ui(CORE_UI
  MusicTranscoder.ui
  ProcessDialog.ui
  AboutDialog.ui
  ConfigurationDialog.ui
)
	
set (CORE_SOURCES
  ${RESOURCES}
  ${CORE_UI}
  ${CORE_SOURCES}
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
  external/QTaskBarButton.cpp
)

set(CORE_EXTERNAL_LIBS
  ${QT_LIBRARIES}
  ${LAME_LIBRARY}
  ${LIBAV_LIBRARIES}
  ${LIBCUE_LIBRARY}
  ${TAGPARSER_LIBRARIES}
  ${LIBOPENMPT_LIBRARY}
  ${ZLIB_LIBRARY}
  ${LIBVPX_LIBRARY}
  openmpt
  Qt6::Core
  Qt6::Widgets
  ws2_32.a
  wsock32.a
  iconv.a
  pthread.a
  bcrypt.a
)
  
add_executable(MusicTranscoder ${CORE_SOURCES})
target_link_libraries (MusicTranscoder ${CORE_EXTERNAL_LIBS})

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
