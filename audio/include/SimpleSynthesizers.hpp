#ifndef SIMPLE_SYNTHESIZERS_HPP
#define SIMPLE_SYNTHESIZERS_HPP

#include <vector>
#include <string>

#include "AudioSampler.hpp"

namespace PianoKeys {
	enum class Key {
		A,
		B,
		C,
		D,
		E,
		F,
		G,
	};
	
	enum class Modifier{
		NONE,
		FLAT, // Step down
		SHARP // Step up
	};

	float getFrequency(const Key& key, const Modifier& mod=Modifier::NONE, const int& octave=4);
	
	class Keyboard{
	public:
		/** Default constructor
		  */
		Keyboard();
		
		/** Get the ideal frequency for the key matching the input string
		  * Input string should have the format "kom" where 'k' is the key (a to g),
		  * 'o' is the octave (0 to 8, 4 assumed if omitted), and 'm' is the modifier (# or b or none if omitted).
		  */
		float get(const std::string& key);
		
		/** Get the key string whose ideal frequency is closest to the input frequency
		  */
		std::string getName(const float& freq);

		/** Get the key string whose ideal frequency is closest to the input frequency
		  * @return The absolute difference between the input frequency and the ideal frequency of the returned key
		  */
		float getName(const float& freq, std::string& key);
		
	private:
		std::vector<std::pair<std::string, float> > frequencies; ///< Standard keyboard has 88 keys, A0 to C8 (27.5 Hz ~ 4186 Hz)
	};
};

namespace SimpleSynthesizers{
	class Synthesizer : public AudioSampler {
	public:
		Synthesizer() : 
			AudioSampler(),
			fAmplitude(1.f),
			fFrequency(440.f),
			fPeriod(1.f / 440.f)
		{
		}
		
		Synthesizer(SoundManager* parent) : 
			AudioSampler(),
			fAmplitude(1.f),
			fFrequency(440.f),
			fPeriod(1.f / 440.f)
		{
		}
		
		~Synthesizer() { }
		
		void setAmplitude(const float& A){ fAmplitude = (A <= 1.f ? A : 1.f); }
		
		void setFrequency(const PianoKeys::Key& key, const int& octave=4){
			setFrequency(PianoKeys::getFrequency(key, PianoKeys::Modifier::NONE, octave));
		}
		
		void setFrequency(const PianoKeys::Key& key, const PianoKeys::Modifier& mod, const int& octave=4){
			setFrequency(PianoKeys::getFrequency(key, mod, octave));
		}
		
		void setFrequency(const float& freq){
			fFrequency = freq;
			fPeriod = 1.f / freq;
		}
		
		float getAmplitude() const { return fAmplitude; }
		
		/** Return the current frequency (in Hz)
		  */
		float getFrequency() const { return fFrequency; }
		
		/** Return the current period (in s)
		  */
		float getPeriod() const { return (1.f / fFrequency); }
		
		float sample(const float& dt){
			return (fAmplitude * clamp(userSample(fPhase += dt)));
		}

	protected:
		float fAmplitude;
		float fFrequency;
		float fPeriod;
		
		virtual float userSample(const float& dt) = 0;
	};
	
	class SineWave : public Synthesizer {
	public:
		SineWave() :
			Synthesizer()
		{
		}

	protected:
		/** Sample the sine wave at a specified phase
		  */
		virtual float userSample(const float& dt);
	};

	class TriangleWave : public Synthesizer {
	public:
		TriangleWave() :
			Synthesizer()
		{
		}
		
	protected:
		/** Sample the wave at a specified phase
		  */
		virtual float userSample(const float& dt);
	};	
	
	class SquareWave : public Synthesizer {
	public:
		SquareWave() :
			Synthesizer(),
			nHarmonics(10)
		{
		}

	protected:
		int nHarmonics; ///< Maximum harmonic number
		
		/** Sample the wave at a specified phase
		  */
		virtual float userSample(const float& dt);
	};
		
	class SawtoothWave : public Synthesizer {
	public:
		SawtoothWave() :
			Synthesizer(),
			nHarmonics(10)
		{
		}

	protected:
		int nHarmonics; ///< Maximum harmonic number
	
		/** Sample the wave at a specified phase
		  */
		virtual float userSample(const float& dt);
	};
};

#endif
