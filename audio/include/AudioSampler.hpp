#ifndef AUDIO_SAMPLER_HPP
#define AUDIO_SAMPLER_HPP

class SoundManager;

class AudioSampler{
public:
	float phase;

	AudioSampler() :
		phase(0.f),
		proc(0x0)
	{ 
	}

	AudioSampler(const SoundManager* parent) :
		phase(0.f),
		proc(0x0)
	{ 
	}
	
	~AudioSampler() { }

	virtual float sample(const float&) { return 0.f; }
	
	virtual void sample(const float& timeStep, float* arr, const unsigned long& N, const unsigned short& channels);
	
protected:
	const SoundManager *proc;
	
	float clamp(const float& input){
		if(input < -1.f)
			return -1.f;
		else if(input > 1.f)
			return 1.f;
		return input;
	}
};

#endif
