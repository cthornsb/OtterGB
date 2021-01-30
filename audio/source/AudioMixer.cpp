#include <iostream>

#include "AudioMixer.hpp"

AudioMixer::AudioMixer(const int& outputChannels, const int& inputChannels, SoundManager* parent) :
	nOutputChannels(outputChannels),
	nInputChannels(inputChannels),
	input(),
	manager(parent)
{
	for(int i = 0; i < nOutputChannels; i++)
		input.push_back(dataPair(std::unique_ptr<AudioData>(new AudioData(inputChannels, parent)), 1.f));
}

void AudioMixer::getSamples(float* arr, const unsigned int& nValues){
	for(int i = 0; i < nOutputChannels; i++){
		input[i].first->getSamples(arr, nValues, i, nOutputChannels);
	}
}
