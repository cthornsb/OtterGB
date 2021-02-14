#include <cmath>

#include "PianoKeys.hpp"

float PianoKeys::getFrequency(const Key& key, const Modifier& mod/*=Modifier::NONE*/, const int& octave/*=4*/){
	const float twelfthRootOfTwo = std::pow(2.f, 1.f / 12.f);
	const float A0 = 27.5f; // Frequency of A0 key, in Hz

	// Octaves are counted from C
	// But the standard frequency is computed from A
	// So we subtract one from the octave to get the previous A key
	float An = A0 * std::pow(2.f, octave - 1);
	int deltaKey;
	
	// Compute how many steps target key is from the A key
	switch(key){
	case Key::A:
		deltaKey = 0;
		break;
	case Key::B:
		deltaKey = 2;
		break;
	case Key::C:
		deltaKey = 3;
		break;
	case Key::D:
		deltaKey = 5;
		break;
	case Key::E:
		deltaKey = 7;
		break;
	case Key::F:
		deltaKey = 8;
		break;
	case Key::G:
		deltaKey = 10;
		break;
	default:
		return 0.f;	
	}
	
	// Sharpen or flatten the key
	switch(mod){
	case Modifier::NONE:
		break;
	case Modifier::FLAT:
		deltaKey -= 1;
		break;
	case Modifier::SHARP:
		deltaKey += 1;
		break;
	default:
		return 0.f;
	}
	
	// Compute the frequency
	if(deltaKey > 0){ // Ascending (from A)
		return (An * std::pow(twelfthRootOfTwo, deltaKey));
	}
	else if(deltaKey < 0){ // Descending (from A)
		return (An / std::pow(twelfthRootOfTwo, -deltaKey));
	}
	return An; // A key
}

PianoKeys::Keyboard::Keyboard(){
	// A0 to C8 (standard 88 key keyboard)
	// One octave: A, A#, B, C, C#, D, D#, E, F, F#, G, G#
	const std::string octaves[9] = { "0", "1", "2", "3", "4", "5", "6", "7", "8" };
	for(int i = 0; i <= 8; i++){ // Over 9 octaves
		frequencies.push_back(std::make_pair("A"  + octaves[i], getFrequency(Key::A, Modifier::NONE,  i)));
		frequencies.push_back(std::make_pair("A#" + octaves[i], getFrequency(Key::A, Modifier::SHARP, i)));
		frequencies.push_back(std::make_pair("B"  + octaves[i], getFrequency(Key::B, Modifier::NONE,  i)));
		frequencies.push_back(std::make_pair("C"  + octaves[i], getFrequency(Key::C, Modifier::NONE,  i)));
		frequencies.push_back(std::make_pair("C#" + octaves[i], getFrequency(Key::C, Modifier::SHARP, i)));
		frequencies.push_back(std::make_pair("D"  + octaves[i], getFrequency(Key::D, Modifier::NONE,  i)));
		frequencies.push_back(std::make_pair("D#" + octaves[i], getFrequency(Key::D, Modifier::SHARP, i)));
		frequencies.push_back(std::make_pair("E"  + octaves[i], getFrequency(Key::E, Modifier::NONE,  i)));
		frequencies.push_back(std::make_pair("F"  + octaves[i], getFrequency(Key::F, Modifier::NONE,  i)));
		frequencies.push_back(std::make_pair("F#" + octaves[i], getFrequency(Key::F, Modifier::SHARP, i)));
		frequencies.push_back(std::make_pair("G"  + octaves[i], getFrequency(Key::G, Modifier::NONE,  i)));
		frequencies.push_back(std::make_pair("G#" + octaves[i], getFrequency(Key::G, Modifier::SHARP, i)));
	}
}

/** Get the ideal frequency for the key matching the input string
  * Input string should have the format "kmo" where 'k' is the key (a to g),
  * 'm' is the modifier (# or b or none if omitted), and 'o' is the octave (0 to 8, 4 assumed if omitted).
  */
float PianoKeys::Keyboard::get(const std::string& key){
	return 0.f;
}

/** Get the key string whose ideal frequency is closest to the input frequency
  */
std::string PianoKeys::Keyboard::getName(const float& freq){
	std::string retval;
	getName(freq, retval);
	return retval;
}

float PianoKeys::Keyboard::getName(const float& freq, std::string& key){
	size_t closestMatch = 0;
	float delta = 999.f; // Percentage difference between input frequency
	for(size_t i = 0; i != frequencies.size(); i++){
		if(frequencies[i].second == freq){ // Exact match is unlikely but possible
			key = frequencies[i].first;
			return 0.f;
		}
		float df = std::abs(2.f * std::abs(frequencies[i].second - freq) / (frequencies[i].second + freq));
		if(df < delta){
			closestMatch = i;
			delta = df;
		}
	}
	key = frequencies[closestMatch].first;
	return delta;
}

