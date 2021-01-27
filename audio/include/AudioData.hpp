#ifndef AUDIO_DATA_HPP
#define AUDIO_DATA_HPP

#include <vector>
#include <memory>

class SoundManager;
class AudioSampler;

class AudioData{
public:
	AudioData();
	
	AudioData(const size_t& chan, const SoundManager* parent);
	
	AudioData(const AudioData&) = delete;
	
	AudioData& operator = (const AudioData&) = delete;

	~AudioData();
	
	AudioSampler* operator [] (const size_t& index) { return samplers[index].get(); }

	AudioSampler* get(const size_t& index) { return samplers[index].get(); }

	void getSamples(float* arr, const unsigned long& nValues);

	/** Get reference to the left channel phase (assuming there is at least one channel)
	  */	
	float& left();
	
	/** Get reference to the right channel phase (assuming there are at least two channels)
	  */
	float& right();
	
	/** Replace an audio channel with a new sampler
	  * The old sampler will be deleted
	  */
	void replace(const size_t& chan, AudioSampler* audio);
	
	/** Get the number of audio channels
	  */
	unsigned short getNumberOfChannels() const { return nChannels; }
	
private:
	unsigned short nChannels;
	
	std::vector<std::unique_ptr<AudioSampler> > samplers;
	
	const SoundManager *proc;

	float fSampleRate;	
	float fTimeStep;
};

#endif
