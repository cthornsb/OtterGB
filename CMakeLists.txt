cmake_minimum_required (VERSION 2.8)

project (gbc)

#Compile with c++11 or c++0x depending on the compiler version
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

	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -fPIC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif(CMAKE_BUILD_TYPE MATCHES "Debug")

#------------------------------------------------------------------------------

#Install options
option(BUILD_TOOLS "Build and install tools." OFF)
option(BUILD_DEBUG "Build with emulator debug window support (Requires QT)." OFF)

#------------------------------------------------------------------------------

#Find required packages.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(TOP_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

#Add the include directories.
include_directories(include)
include_directories(graphics/include)

if(BUILD_DEBUG)
	#Find qt install.
	#sudo apt-get install libqt4-dev
	find_package (QT REQUIRED)
	include_directories(qtfiles ${QT_INC_DIR} ${QT_CORE_INC_DIR} ${QT_GUI_INC_DIR})
	add_subdirectory(qtfiles)
endif()

#Set the graphics library
if(NOT GRAPHICS_LIBRARY)
	# To install OpenGL:
	#sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
	set(GRAPHICS_LIBRARY OpenGL CACHE STRING "Graphics library, options are: SDL OpenGL" FORCE)
endif()

#Set the Gameboy boot ROM
if(NOT GB_BOOT_ROM)
	set(GB_BOOT_ROM "" CACHE STRING "Gameboy boot program" FORCE)
endif(NOT GB_BOOT_ROM)

#Set the Gameboy Color boot ROM
if(NOT GBC_BOOT_ROM)
	set(GBC_BOOT_ROM "" CACHE STRING "Gameboy Color boot program" FORCE)
endif(NOT GBC_BOOT_ROM)

#Definitions
if(BUILD_DEBUG)
	add_definitions(-DUSE_QT_DEBUGGER)
endif()

add_definitions(-DTOP_DIRECTORY="${TOP_DIRECTORY}")

if(GRAPHICS_LIBRARY MATCHES "OpenGL")
	add_definitions(-DUSE_OPENGL)
endif()

#Add the graphics directory.
add_subdirectory(graphics/source)

#Add the source directory.
add_subdirectory(source)

if(BUILD_TOOLS)
	#Add the tools directory.
	add_subdirectory(tools)
endif(BUILD_TOOLS)

#Build/install the miscellaneous stuff
#add_subdirectory(share)
