#include <cmath>

#include "Synthesizers.hpp"

constexpr float PI = 3.14159f;

float Synthesizers::SineWave::userSample(const float& dt){
	return std::sin(fPhase * fFrequency * 2 * PI);
}

float Synthesizers::TriangleWave::userSample(const float& dt){
	return ((4.f * fFrequency) * std::fabs(std::fmod(fPhase - fPeriod/4, fPeriod) - fPeriod/2) - 1.f);
}

float Synthesizers::SquareWave::userSample(const float& dt){
	const float coeff = 2 * PI * fFrequency * fPhase;
	float sum = 0.f;
	for(int i = 1; i <= nHarmonics; i++){
		sum += std::sin(coeff * (2.f * i - 1)) / (2.f * i - 1);
	}
	return (4.f * sum / PI);
}

float Synthesizers::SawtoothWave::userSample(const float& dt){
	const float coeff = 2 * PI * fFrequency * fPhase;
	float sum = 0.f;
	for(int i = 1; i <= nHarmonics; i++){
		sum += (i % 2 == 0 ? 1.f : -1.f) * std::sin(coeff * i) / i;
	}
	return (-2.f * sum / PI);
}

