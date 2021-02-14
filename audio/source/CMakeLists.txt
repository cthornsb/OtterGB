# Audio components
set(AUDIO_SOURCES 
	AudioUnit.cpp
	FrequencySweep.cpp
	LengthCounter.cpp
	MidiFile.cpp
	PianoKeys.cpp
	ShiftRegister.cpp
	SoundBuffer.cpp
	SoundManager.cpp
	SoundMixer.cpp
	SquareWave.cpp
	UnitTimer.cpp
	VolumeEnvelope.cpp
	WaveTable.cpp
)

# Unused source files
#  AudioData.cpp
#  AudioMixer.cpp
#  AudioSampler.cpp
#  Synthesizers.cpp
#  WavFile.cpp
	
# Generate the audio library
add_library(AUDIO_OBJECTS OBJECT ${AUDIO_SOURCES})
add_library(AUDIO_LIB STATIC $<TARGET_OBJECTS:AUDIO_OBJECTS>)
