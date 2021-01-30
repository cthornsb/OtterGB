#ifndef AUDIO_DATA_HPP
#define AUDIO_DATA_HPP

#include <vector>
#include <memory>
//#include <pair>

class SoundManager;
class AudioSampler;

class AudioData{
public:
	AudioData();
	
	AudioData(const size_t& chan, const SoundManager* parent);
	
	AudioData(const AudioData&) = delete;
	
	AudioData& operator = (const AudioData&) = delete;

	~AudioData();
	
	AudioSampler& operator [] (const size_t& index) { return (*samplers[index].get()); }

	AudioSampler* get(const size_t& index) { return samplers[index].get(); }

	void getSamples(float* arr, const unsigned int& nValues, const unsigned int& nOffset, const unsigned int& nSkip);

	void addInput(AudioSampler* audio);

	/** Replace an audio channel with a new sampler
	  * The old sampler will be deleted
	  */
	void replaceInput(const size_t& chan, AudioSampler* audio);
	
	/** Get the number of audio channels
	  */
	unsigned short getNumberOfChannels() const { return nChannels; }
	
private:
	unsigned short nChannels;
	
	std::vector<std::unique_ptr<AudioSampler> > samplers;
	
	const SoundManager* manager;

	float fSampleRate;	
	float fTimeStep;
	float fTotalVolume;
};

#endif
