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

		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -fPIC")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
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
if(WIN32)
	option(INSTALL_DLLS "Install required DLLs when installing executable." ON)
endif(WIN32)

#------------------------------------------------------------------------------

#Find required packages.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(TOP_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_definitions(-DTOP_DIRECTORY="${TOP_DIRECTORY}")

#Set the graphics library
if(NOT GRAPHICS_LIBRARY)
	set(GRAPHICS_LIBRARY OpenGL CACHE STRING "Graphics library, options are: SDL OpenGL" FORCE)
endif()

if(GRAPHICS_LIBRARY MATCHES "OpenGL")
	if(NOT WIN32)
		# To install OpenGL and freeglut:
		#(Ubuntu) sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
		set(GRAPHICS_LIBS -lglut -lGLU -lGL)
	else()
		#Find OpenGL install on Windows (eventually make a CMake module for this)
		#Precompiled freeglut libs from https://www.transmissionzero.co.uk/software/freeglut-devel/
		if(NOT FREEGLUT_DIRECTORY)
			set(FREEGLUT_DIRECTORY "" CACHE STRING "Freeglut install directory" FORCE)
		endif(NOT FREEGLUT_DIRECTORY)
		set(FREEGLUT_INCLUDE ${FREEGLUT_DIRECTORY}/include)
		if(CMAKE_CL_64)
			set(FREEGLUT_LIBRARY ${FREEGLUT_DIRECTORY}/lib/x64/freeglut.lib)
			set(FREEGLUT_DLL_LIST ${FREEGLUT_DIRECTORY}/bin/x64/freeglut.dll)
		else()
			set(FREEGLUT_LIBRARY ${FREEGLUT_DIRECTORY}/lib/freeglut.lib)
			set(FREEGLUT_DLL_LIST ${FREEGLUT_DIRECTORY}/bin/freeglut.dll)
		endif(CMAKE_CL_64)
		include_directories(${FREEGLUT_INCLUDE})
		set(GRAPHICS_LIBS ${FREEGLUT_LIBRARY} opengl32.lib)
		#It is necessary to copy freeglut.dll to the build directory containing the executable.
		#Alternatively it may be copied to the Windows system directory.
		#This is necessary so that the freeglut headers point to the correct library
		add_definitions(-DNDEBUG)
	endif()
	add_definitions(-DUSE_OPENGL)
elseif(GRAPHICS_LIBRARY MATCHES "SDL")
	#Fix this at some point for Windows!
	set(GRAPHICS_LIBS -lSDL2)
endif()

#Add the include directories.
include_directories(include)
include_directories(graphics/include)

#Add the graphics directory.
add_subdirectory(graphics/source)

#Add the source directory.
add_subdirectory(source)

#Build/install the miscellaneous stuff
#add_subdirectory(share)
