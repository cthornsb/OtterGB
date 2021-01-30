#ifndef AUDIO_SAMPLER_HPP
#define AUDIO_SAMPLER_HPP

class SoundManager;

class AudioSampler{
public:
	AudioSampler() :
		fPhase(0.f),
		fVolume(1.f),
		proc(0x0)
	{ 
	}

	AudioSampler(const SoundManager* parent) :
		fPhase(0.f),
		fVolume(1.f),
		proc(0x0)
	{ 
	}
	
	~AudioSampler() { }

	float getPhase() const { return fPhase; }

	float getVolume() const { return fVolume; }

	void setVolume(const float& volume) { fVolume = volume; }

	virtual float sample(const float&) { return 0.f; }
	
	virtual void sample(const float& timeStep, float* arr, const unsigned int& N);
	
protected:
	float fPhase;

	float fVolume;

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
