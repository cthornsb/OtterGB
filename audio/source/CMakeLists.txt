# Compile audio source files
add_library(
	AUDIO_OBJECTS 
	OBJECT
	AudioUnit.cpp
	FrequencySweep.cpp
	LengthCounter.cpp
	ShiftRegister.cpp
	SquareWave.cpp
	VolumeEnvelope.cpp
	WaveTable.cpp
)

target_include_directories(AUDIO_OBJECTS 
	PRIVATE 
	../include 
	${OTTER_INCLUDE_DIRS}
)

if(ENABLE_AUDIO)
	target_compile_definitions(AUDIO_OBJECTS 
		PRIVATE 
		AUDIO_ENABLED
	)
endif()

# Generate the audio library
add_library(AUDIO_LIB 
	STATIC 
	$<TARGET_OBJECTS:AUDIO_OBJECTS>
)
