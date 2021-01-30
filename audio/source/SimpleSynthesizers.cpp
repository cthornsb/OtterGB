#include <cmath>

#include "SimpleSynthesizers.hpp"

constexpr float PI = 3.14159f;

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

float SimpleSynthesizers::SineWave::userSample(const float& dt){
	return std::sin(fPhase * fFrequency * 2 * PI);
}

float SimpleSynthesizers::TriangleWave::userSample(const float& dt){
	return ((4.f * fFrequency) * std::fabs(std::fmod(fPhase - fPeriod/4, fPeriod) - fPeriod/2) - 1.f);
}

float SimpleSynthesizers::SquareWave::userSample(const float& dt){
	const float coeff = 2 * PI * fFrequency * fPhase;
	float sum = 0.f;
	for(int i = 1; i <= nHarmonics; i++){
		sum += std::sin(coeff * (2.f * i - 1)) / (2.f * i - 1);
	}
	return (4.f * sum / PI);
}

float SimpleSynthesizers::SawtoothWave::userSample(const float& dt){
	const float coeff = 2 * PI * fFrequency * fPhase;
	float sum = 0.f;
	for(int i = 1; i <= nHarmonics; i++){
		sum += (i % 2 == 0 ? 1.f : -1.f) * std::sin(coeff * i) / i;
	}
	return (-2.f * sum / PI);
}

