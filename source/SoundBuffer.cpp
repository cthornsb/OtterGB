#include <iostream> // TEMP

#include "SoundBuffer.hpp"

void SoundBuffer::pushSample(const float& l, const float& r){
	lock.lock(); // Writing to buffer
	push(std::make_pair(l, r));
	if(size() >= 1024) // Too simple, needs to be fixed CRT
		pop();
	lock.unlock();
}

bool SoundBuffer::getSample(float* output){
	lock.lock(); // Reading from buffer
	if(!empty()){ // Copy audio sample to the output array and to the previous sample 
		output[0] = left();
		output[1] = right();
		popSample();
		return true;
	}
	else{ // Sample buffer is empty D:
		output[0] = fEmptyLeft;
		output[1] = fEmptyRight;
	}
	lock.unlock();
	return false;
}

bool SoundBuffer::getSamples(float* output, const size_t& N){
	lock.lock(); // Reading from buffer
	//std::cout << " getting N=" << N << " samples with " << size() << " elements in buffer\n";
	bool retval = false;
	if(size() >= N){ // Buffer contains at least N samples
		for(size_t i = 0; i < N; i++){
			output[2 * i]     = left();
			output[2 * i + 1] = right();
			popSample();
		}
		retval = true;
	}
	else if(size() > 1){ // Not enough samples in the buffer, in-between values will be interpolated
		size_t delta = N - size(); // Number of missing samples
		float period = (float)N / (size() - 1); // Number of "in-between" samples per actual sample
		float counter = 0.f;
		float current[2] = {
			left(), 
			right()
		}; // Starting left / right volumes used for interpolation
		popSample(); // Pop the first sample
		float slopes[2] = {
			(left() - current[0]) / period, 
			(right() - current[1]) / period
		}; // Slopes for volume interpolation
		for(size_t i = 0; i < N - 1; i++){
			output[2 * i + 0] = current[0] + counter * slopes[0];
			output[2 * i + 1] = current[1] + counter * slopes[1];
			counter += 1.f;			
			if(counter >= period){ // Update interpolation values
				current[0] = left();
				current[1] = right();
				popSample();
				// Re-compute slopes
				slopes[0] = (left() - current[0]) / period;
				slopes[1] = (right() - current[1]) / period;
				counter = 0.f;
			}
		}
		// Final sample
		current[0] = left();
		current[1] = right();
		popSample();
	}
	else{// Sample buffer is empty, just fill it with our backup values
		if(!empty())
			popSample(); // Backup final sample and pop it off the queue
		for(size_t i = 0; i < N; i++){
			output[2 * i]     = fEmptyLeft;
			output[2 * i + 1] = fEmptyRight;
		}
	}
	lock.unlock();
	return retval;
}

float SoundBuffer::left() const {
	if(empty())
		return fEmptyLeft;
	return (this->front().first);
}

float SoundBuffer::right() const {
	if(empty())
		return fEmptyRight;
	return (this->front().second);
}

void SoundBuffer::popSample(){
	if(empty()){
		std::cout << " ERROR! Popping with zero size" << std::endl;
		return;
	}
	if(size() == 1){ // Sample buffer will be empty after pop, backup the remaining sample
		fEmptyLeft = left();
		fEmptyRight = right();
	}
	pop();
}
