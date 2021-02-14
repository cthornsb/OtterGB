# To install pulse audio library
set(AUDIO_SOURCES AudioData.cpp
                  AudioMixer.cpp
                  AudioSampler.cpp
                  AudioUnit.cpp
                  FrequencySweep.cpp
                  LengthCounter.cpp
                  MidiFile.cpp
                  ShiftRegister.cpp
                  SimpleSynthesizers.cpp
                  SoundBuffer.cpp
                  SoundManager.cpp
                  SoundMixer.cpp
                  SquareWave.cpp
                  UnitTimer.cpp
                  VolumeEnvelope.cpp
                  WavFile.cpp
                  WaveTable.cpp)

# Generate the audio library
add_library(AUDIO_OBJECTS OBJECT ${AUDIO_SOURCES})
add_library(AUDIO_LIB STATIC $<TARGET_OBJECTS:AUDIO_OBJECTS>)
