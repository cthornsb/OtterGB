#include "AudioData.hpp"
#include "AudioSampler.hpp"
#include "SoundManager.hpp"

AudioData::AudioData() :
	nChannels(0),
	samplers(),
	manager(0x0),
	fSampleRate(44100.f),
	fTimeStep(1.f / fSampleRate),
	fTotalVolume(0.f)
{
}

AudioData::AudioData(const size_t& chan, const SoundManager* parent) :
	nChannels(0),
	samplers(),
	manager(parent),
	fSampleRate((float)parent->getSampleRate()),
	fTimeStep(1.f / fSampleRate),
	fTotalVolume(0.f)
{
	for(size_t i = 0; i < chan; i++)
		addInput(new AudioSampler(manager));
}

AudioData::~AudioData() { 
}

void AudioData::getSamples(float* arr, const unsigned int& nValues, const unsigned int& nOffset, const unsigned int& nSkip){
	std::vector<float> samples(nValues, 0.f);
	for(unsigned int i = 0; i < nChannels; i++) { // Over all channels
		samplers[i]->sample(fTimeStep, samples.data(), nValues);
	}
	for(unsigned int i = 0; i < nValues; i++){
		arr[nOffset + i * nSkip] = samples[i] / fTotalVolume;
	}
}

void AudioData::addInput(AudioSampler* audio){
	samplers.push_back(std::unique_ptr<AudioSampler>(new AudioSampler(manager)));
	fTotalVolume += audio->getVolume();
	nChannels++;
}

void AudioData::replaceInput(const size_t& chan, AudioSampler* audio){
	fTotalVolume -= samplers[chan]->getVolume();
	samplers[chan].reset(audio);
	fTotalVolume += audio->getVolume();
}

