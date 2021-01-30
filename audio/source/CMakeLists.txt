# To install pulse audio library
set(AUDIO_SOURCES AudioData.cpp
                  AudioMixer.cpp
                  AudioSampler.cpp
                  SimpleSynthesizers.cpp
                  SoundManager.cpp
                  WavFile.cpp)

# Generate the audio library
add_library(AUDIO_OBJECTS OBJECT ${AUDIO_SOURCES})
add_library(AUDIO_LIB STATIC $<TARGET_OBJECTS:AUDIO_OBJECTS>)
