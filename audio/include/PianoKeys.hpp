#ifndef PIANO_KEYS_HPP
#define PIANO_KEYS_HPP

#include <vector>
#include <string>

namespace PianoKeys {
	enum class Key {
		NONE,
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

	class Note{
	public:
		Key key;
		Modifier mod;
		int nOctave;
	
		Note() :
			key(Key::NONE),
			mod(Modifier::NONE),
			nOctave(4)
		{
		}
	};
	
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

#endif
