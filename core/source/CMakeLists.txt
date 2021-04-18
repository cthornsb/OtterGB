# Core components
if(NOT WIN32)
add_library(
	CORE_OBJECTS 
	OBJECT
	ComponentTimer.cpp
	ConfigFile.cpp
	HighResTimer.cpp
	Opcode.cpp
	optionHandler.cpp
	Support.cpp
	SystemComponent.cpp
	Register.cpp
)
else() # Windows
add_library(
	CORE_OBJECTS 
	OBJECT
	ComponentTimer.cpp
	ConfigFile.cpp
	HighResTimer.cpp
	Opcode.cpp
	Support.cpp
	SystemComponent.cpp
	Register.cpp
)
endif()

target_include_directories(CORE_OBJECTS PRIVATE ../include)

# Generate the core library
add_library(CORE_LIB STATIC $<TARGET_OBJECTS:CORE_OBJECTS>)

