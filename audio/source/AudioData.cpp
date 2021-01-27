#include "AudioData.hpp"
#include "AudioSampler.hpp"
#include "SoundManager.hpp"

AudioData::AudioData() :
	nChannels(0),
	samplers(),
	proc(0x0),
	fSampleRate(44100.f),
	fTimeStep(1.f / fSampleRate)
{
}

AudioData::AudioData(const size_t& chan, const SoundManager* parent) :
	nChannels(chan),
	samplers(),
	proc(parent),
	fSampleRate((float)parent->getSampleRate()),
	fTimeStep(1.f / fSampleRate)
{
	for(size_t i = 0; i < chan; i++)
		samplers.push_back(std::unique_ptr<AudioSampler>(new AudioSampler(proc)));
}

AudioData::~AudioData() { 
}

void AudioData::getSamples(float* arr, const unsigned long& nValues){
	for(unsigned short i = 0; i < nChannels; i++) { // Over all channels
		samplers[i]->sample(fTimeStep, &arr[i], nValues, nChannels);
	}
}

float& AudioData::left() { 
	return samplers[0]->phase; 
}

float& AudioData::right() { 
	return samplers[1]->phase; 
}

void AudioData::replace(const size_t& chan, AudioSampler* audio){
	samplers[chan].reset(audio);
}

