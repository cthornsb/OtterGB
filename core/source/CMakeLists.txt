# Core components
set(CORE_SOURCES 
	ComponentTimer.cpp
	ConfigFile.cpp
	HighResTimer.cpp
	Opcode.cpp
	Support.cpp
	SystemComponent.cpp
	Register.cpp
	TextParser.cpp
)

if(NOT WIN32)
	set(CORE_SOURCES ${CORE_SOURCES} optionHandler.cpp)
endif(NOT WIN32)

# Generate the core library
add_library(CORE_OBJECTS OBJECT ${CORE_SOURCES})
add_library(CORE_LIB STATIC $<TARGET_OBJECTS:CORE_OBJECTS>)

