#ifndef AUDIO_MIXER_HPP
#define AUDIO_MIXER_HPP

#include <vector>

#include "AudioData.hpp"

class SoundManager;

class AudioMixer{
public:
	AudioMixer() :
		nOutputChannels(0),
		nInputChannels(0),
		input(),
		manager(0x0)
	{
	}
	
	AudioMixer(const int& outputChannels, const int& inputChannels, SoundManager* parent);
	
	~AudioMixer() { }

	AudioMixer(const AudioMixer&) = delete;
	
	AudioMixer& operator = (const AudioMixer&) = delete;

	AudioData& operator [] (const size_t& index) { return (*input[index].first.get()); }
	
	AudioData* get(const size_t& index) { return input[index].first.get(); }

	int getNumberInputChannels() const { return nInputChannels; }
	
	int getNumberOutputChannels() const { return nOutputChannels; }

	void getSamples(float* arr, const unsigned int& nValues);

	/** Get reference to the left channel phase (assuming there is at least one channel)
	  */	
	AudioData& left() { return (*input[0].first.get()); }
	
	/** Get reference to the right channel phase (assuming there are at least two channels)
	  */
	AudioData& right() { return (*input[1].first.get()); }

private:
	typedef std::pair<std::unique_ptr<AudioData>, float> dataPair;

	int nOutputChannels;
	
	int nInputChannels;

	std::vector<dataPair> input;
	
	const SoundManager* manager;
};

#endif
