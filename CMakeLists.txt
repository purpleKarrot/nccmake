# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.1)
project(nccmake VERSION 0.1 LANGUAGES CXX)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug"
  "Release"
  "RelWithDebInfo"
  "MinSizeRel"
  )

list(INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/CMake")

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
find_package(LibUV 1.0.0 REQUIRED)
find_package(JsonCpp REQUIRED)

set(KWSYS_NAMESPACE "cmsys")
set(KWSYS_HEADER_ROOT ${PROJECT_BINARY_DIR})
set(KWSYS_USE_Directory 1)
set(KWSYS_USE_FStream 1)
set(KWSYS_USE_RegularExpression 1)
set(KWSYS_USE_SystemTools 1)
add_subdirectory(KWSys)

add_executable(nccmake
  cmCursesOptionsWidget.cxx
  cmCursesBoolWidget.cxx
  cmCursesCacheEntryComposite.cxx
  cmCursesDummyWidget.cxx
  cmCursesFilePathWidget.cxx
  cmCursesForm.cxx
  cmCursesLabelWidget.cxx
  cmCursesLongMessageForm.cxx
  cmCursesMainForm.cxx
  cmCursesPathWidget.cxx
  cmCursesStringWidget.cxx
  cmCursesWidget.cxx
  cmDocumentation.cxx
  cmState.cxx
  cmSystemTools.cxx
  ccmake.cxx
  cmake.cxx
  )

set_target_properties(nccmake
  PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
  )

target_include_directories(nccmake
  PRIVATE
    ${CURSES_INCLUDE_DIRS}
    ${PROJECT_BINARY_DIR}
  )

target_link_libraries(nccmake
  PRIVATE
    ${CURSES_LIBRARIES}
    JsonCpp::JsonCpp
    LibUV::LibUV
    cmsys
  )

install(TARGETS nccmake
  DESTINATION bin
  )
