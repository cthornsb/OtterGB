cmake_minimum_required (VERSION 3.1)

project (ottergb)

if(NOT WIN32) # Unix systems (Ubuntu)
	#Compile with c++11 or c++0x depending on the compiler version
	message(STATUS "Building for Linux")
	if (CMAKE_COMPILER_IS_GNUCXX)
		#Make compiler messages nice with colored tags.
		if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.9")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
		endif()
		# Compile flags
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic")
	endif()
else()
	message(STATUS "Building for Windows")
endif()

#if user does not specify prefix we assign it to the exec directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   message(STATUS "Install Prefix not specified.")
   file(MAKE_DIRECTORY install)
   get_filename_component(INSTALL_DIR ${CMAKE_SOURCE_DIR}/install REALPATH)
   set(CMAKE_INSTALL_PREFIX ${INSTALL_DIR} CACHE PATH "Install Prefix" FORCE)
endif()
message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")

#Define the default build type to be Release
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Build type, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
   message(STATUS "Build type not defined, using default: ${CMAKE_BUILD_TYPE}")
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#Add additional CXX flags if we use the Debug option
if(CMAKE_BUILD_TYPE MATCHES "Debug")
	if(NOT WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
	endif()
endif(CMAKE_BUILD_TYPE MATCHES "Debug")

#------------------------------------------------------------------------------

#Build options
option(BUILD_TOOLS     "Build and install emulator tools." OFF)
option(ENABLE_AUDIO    "Build with support for audio output (Requires PortAudio)." ON)
option(ENABLE_DEBUGGER "Build with support for gui debugger (Requires QT4)." OFF)
if(WIN32)
	option(INSTALL_DLLS "Install required DLLs when installing executable." ON)
endif(WIN32)

#------------------------------------------------------------------------------

#Set OtterEngine install directory.
if(NOT OTTER_DIRECTORY)
	set(OTTER_DIRECTORY "" CACHE STRING "OtterEngine install directory" FORCE)
endif(NOT OTTER_DIRECTORY)

#Find required packages (sourced from OtterEngine)
include("${OTTER_DIRECTORY}/OtterConfig.cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OTTER_MODULE_PATH})

#Include pre-requisite finder module
include(PreRequisites)

#Find all required packages
# Use OtterEngine 2d library
ott_use_core()

#Set the current working directory (needed by some files)
set(TOP_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_definitions(-DTOP_DIRECTORY="${TOP_DIRECTORY}")

#Compiler definitions
if(ENABLE_DEBUGGER)
	add_definitions(-DUSE_QT_DEBUGGER)
endif(ENABLE_DEBUGGER)

#Audio output (Port Audio)
#if(ENABLE_AUDIO)
	ott_use_audio()
#endif()

if(WIN32 AND INSTALL_DLLS)
	#List of external dlls
	set( INSTALL_DLL_LIST
		${GLFW3_DLL_PATH}
		${GLEW_DLL_PATH}
		${PORT_AUDIO_DLL_PATH}
	)
endif()

#Gui debugger (Qt4)
if(ENABLE_DEBUGGER)
	#Find qt install.
	#(Ubuntu) sudo apt-get install libqt4-dev
	#(Windows) Precompiled Qt4 libs from https://www.npcglib.org/~stathis/blog/precompiled-qt4-qt5/
	find_package(Qt4 REQUIRED) # Requires Qt4::QtCore Qt4::QtGui Qt4::QtOpenGL
	if(QT_USE_FILE)
		include(${QT_USE_FILE})
	endif()
endif()

#Set the DMG bootstrap ROM
if(NOT DMG_BOOTSTRAP_ROM)
	set(DMG_BOOTSTRAP_ROM "bootstraps/dmg_boot.bin" CACHE STRING "DMG bootstrap program" FORCE)
endif(NOT DMG_BOOTSTRAP_ROM)

#Set the CGB bootstrap ROM
if(NOT CGB_BOOTSTRAP_ROM)
	set(CGB_BOOTSTRAP_ROM "bootstraps/cgb_boot.bin" CACHE STRING "CGB bootstrap program" FORCE)
endif(NOT CGB_BOOTSTRAP_ROM)

#Add the graphics directory.
add_subdirectory(graphics/source)

#Add the sound directory.
add_subdirectory(audio/source)

#Add core component directory.
add_subdirectory(core/source)

#Add system component directory.
add_subdirectory(source)

if(BUILD_TOOLS)
	#Add the tools directory.
	add_subdirectory(tools)
endif(BUILD_TOOLS)
