# To install pulse audio library
set(SOUND_SOURCES AudioData.cpp
                  AudioSampler.cpp
                  SimpleSynthesizers.cpp
                  SoundManager.cpp
                  WavFile.cpp)

# Generate the audio library
add_library(SOUND_OBJECTS OBJECT ${SOUND_SOURCES})
add_library(SOUND_LIB STATIC $<TARGET_OBJECTS:SOUND_OBJECTS>)
