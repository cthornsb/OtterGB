cmake_minimum_required (VERSION 2.8)

project (gbc)

if(NOT WIN32) # Unix systems (Ubuntu)
	#Compile with c++11 or c++0x depending on the compiler version
	message(STATUS "Building for Linux")
	if (CMAKE_COMPILER_IS_GNUCXX)
		if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.3" AND
				${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "4.7")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
		elseif(${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.6")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
		endif(${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.3" AND
				${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "4.7")

		#Make compiler messages nice with colored tags.
		if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.9")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
		endif()
		# Compile flags
		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -fPIC")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
		# Linker flags
		set(CMAKE_EXE_LINKER_FLAGS -pthread)
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

#Install options
option(BUILD_TOOLS "Build and install tools." OFF)
option(BUILD_DEBUG "Build with emulator debug window support (Requires QT)." OFF)
if(WIN32)
	option(INSTALL_DLLS "Install required DLLs when installing executable." ON)
endif(WIN32)

#------------------------------------------------------------------------------

#Find required packages.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(TOP_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

#Compiler definitions
if(BUILD_DEBUG)
	add_definitions(-DUSE_QT_DEBUGGER)
endif()

add_definitions(-DTOP_DIRECTORY="${TOP_DIRECTORY}")

#Set the graphics library
if(NOT GRAPHICS_LIBRARY)
	set(GRAPHICS_LIBRARY OpenGL CACHE STRING "Graphics library, options are: SDL OpenGL" FORCE)
endif()

if(GRAPHICS_LIBRARY MATCHES "OpenGL")
	if(NOT WIN32)
		# To install OpenGL and freeglut:
		#(Ubuntu) sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
		set(EXTERNAL_GRAPHICS_LIBS -lglut -lGLU -lGL)
		set(EXTERNAL_AUDIO_LIBS -lportaudio)
	else()
		#Find OpenGL install on Windows (eventually make a CMake module for this)
		#Precompiled freeglut libs from https://www.transmissionzero.co.uk/software/freeglut-devel/
		if(NOT FREEGLUT_DIRECTORY)
			set(FREEGLUT_DIRECTORY "" CACHE STRING "Freeglut install directory" FORCE)
		endif(NOT FREEGLUT_DIRECTORY)
		set(FREEGLUT_INCLUDE ${FREEGLUT_DIRECTORY}/include)
		if(CMAKE_CL_64)
			set(FREEGLUT_LIBRARY ${FREEGLUT_DIRECTORY}/lib/x64/freeglut.lib)
			set(INSTALL_DLL_LIST ${FREEGLUT_DIRECTORY}/bin/x64/freeglut.dll)
		else()
			set(FREEGLUT_LIBRARY ${FREEGLUT_DIRECTORY}/lib/freeglut.lib)
			set(INSTALL_DLL_LIST ${FREEGLUT_DIRECTORY}/bin/freeglut.dll)
		endif(CMAKE_CL_64)
		include_directories(${FREEGLUT_INCLUDE})
		set(EXTERNAL_GRAPHICS_LIBS ${FREEGLUT_LIBRARY} opengl32.lib)
		#It is necessary to copy freeglut.dll to the build directory containing the executable.
		#Alternatively it may be copied to the Windows system directory.
		#This is necessary so that the freeglut headers point to the correct library
		add_definitions(-DNDEBUG)

		#Audio (Port Audio)
		if(NOT PORT_AUDIO_DIRECTORY)
			set(PORT_AUDIO_DIRECTORY "" CACHE STRING "Port Audio install directory" FORCE)
		endif(NOT PORT_AUDIO_DIRECTORY)
		set(PORT_AUDIO_INCLUDE ${PORT_AUDIO_DIRECTORY}/include)
		set(PORT_AUDIO_LIBRARY ${PORT_AUDIO_DIRECTORY}/lib/x64/portaudio_x64.lib)
		set(INSTALL_DLL_LIST ${INSTALL_DLL_LIST} ${PORT_AUDIO_DIRECTORY}/bin/x64/portaudio_x64.dll)
		include_directories(${PORT_AUDIO_INCLUDE})
		set(EXTERNAL_AUDIO_LIBS ${PORT_AUDIO_LIBRARY})
	endif()
	
elseif(GRAPHICS_LIBRARY MATCHES "SDL")
	#Fix this at some point for Windows!
	set(EXTERNAL_GRAPHICS_LIBS -lSDL2)
	add_definitions(-DUSE_SDL_RENDERER)
endif()

#Add the gbc project pre-processor definition
add_definitions(-DPROJECT_GBC)

#Add the include directories.
include_directories(include)
include_directories(audio/include)
include_directories(graphics/include)

if(BUILD_DEBUG)
	if(NOT WIN32)
		#Find qt install.
		#(Ubuntu) sudo apt-get install libqt4-dev
		find_package (QT REQUIRED)
	else()
		#Find Qt install on Windows (eventually make a CMake module for this)
		#Precompiled Qt4 libs from https://www.npcglib.org/~stathis/blog/precompiled-qt4-qt5/
		if(NOT QT_TOP_DIRECTORY)
			set(QT_TOP_DIRECTORY "" CACHE STRING "Qt4+ install directory" FORCE)
		endif(NOT QT_TOP_DIRECTORY)
		#Set paths for Qt headers
		set(QT_INC_DIR ${QT_TOP_DIRECTORY}/include)
		set(QT_CORE_INC_DIR ${QT_INC_DIR}/QtCore)
		set(QT_GUI_INC_DIR ${QT_INC_DIR}/QtGui)
		#Set paths for Qt libraries
		set(QT_LIB_DIR ${QT_TOP_DIRECTORY}/lib)
		set(QT_GUI_LIB ${QT_LIB_DIR}/QtGui4.lib)
		set(QT_CORE_LIB ${QT_LIB_DIR}/QtCore4.lib)
		set(QT_OPENGL_LIB ${QT_LIB_DIR}/QtOpenGL4.lib)
		#Set Qt executable path
		set(MOC_EXECUTABLE ${QT_TOP_DIRECTORY}/bin/moc)
		set(UIC_EXECUTABLE ${QT_TOP_DIRECTORY}/bin/uic)
		#Set a list of Qt dlls
		set(INSTALL_DLL_LIST ${INSTALL_DLL_LIST} ${QT_LIB_DIR}/QtGui4.dll ${QT_LIB_DIR}/QtCore4.dll ${QT_LIB_DIR}/QtOpenGL4.dll)
	endif()
	include_directories(qtfiles ${QT_INC_DIR} ${QT_CORE_INC_DIR} ${QT_GUI_INC_DIR})
	add_subdirectory(qtfiles)
endif()

#Set the Gameboy boot ROM
if(NOT GB_BOOT_ROM)
	set(GB_BOOT_ROM "" CACHE STRING "Gameboy boot program" FORCE)
endif(NOT GB_BOOT_ROM)

#Set the Gameboy Color boot ROM
if(NOT GBC_BOOT_ROM)
	set(GBC_BOOT_ROM "" CACHE STRING "Gameboy Color boot program" FORCE)
endif(NOT GBC_BOOT_ROM)

#Add the graphics directory.
add_subdirectory(graphics/source)

#Add the sound directory.
add_subdirectory(audio/source)

#Add the source directory.
add_subdirectory(source)

if(BUILD_TOOLS)
	#Add the tools directory.
	add_subdirectory(tools)
endif(BUILD_TOOLS)

#Build/install the miscellaneous stuff
#add_subdirectory(share)
