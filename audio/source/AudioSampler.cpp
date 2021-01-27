#include "AudioSampler.hpp"

void AudioSampler::sample(const float& timeStep, float* arr, const unsigned long& N, const unsigned short& channels) { 
	for(unsigned long i = 0; i < N; i++) { // Over all frames
		arr[i * channels] = sample(timeStep);
	}
}

