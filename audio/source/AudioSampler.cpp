#include "AudioSampler.hpp"

void AudioSampler::sample(const float& timeStep, float* arr, const unsigned int& N) { 
	for(unsigned long i = 0; i < N; i++) { // Over all frames
		arr[i] += fVolume * sample(timeStep);
	}
}

